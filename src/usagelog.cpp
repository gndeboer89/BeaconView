#include "usagelog.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMap>
#include <QStandardPaths>
#include "songfilter.h"

UsageLog::UsageLog(QObject *parent) : QObject(parent) { load(); }

QString UsageLog::path() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/usage.json";
}

void UsageLog::record(const QString &title, const QString &ccli, const QString &author, const QString &service, const QDateTime &when) {
    if (title.trimmed().isEmpty()) return;
    m_entries.append({when, title.trimmed(), ccli.trimmed(), author.trimmed(), service.trimmed()});
    save();
    emit changed();
}

void UsageLog::clear() {
    if (m_entries.isEmpty()) return;
    m_entries.clear();
    save();
    emit changed();
}

void UsageLog::range(const QString &period, const QDate &today, QDate *from, QDate *to) {
    *from = QDate(); *to = QDate();
    if (period == "month") { *from = QDate(today.year(), today.month(), 1); *to = from->addMonths(1).addDays(-1); }
    else if (period == "lastmonth") { *from = QDate(today.year(), today.month(), 1).addMonths(-1); *to = from->addMonths(1).addDays(-1); }
    else if (period == "year") { *from = QDate(today.year(), 1, 1); *to = QDate(today.year(), 12, 31); }
}

QVariantList UsageLog::summary(const QDate &from, const QDate &to) const {
    struct Row { QString title, ccli, author; int count = 0; QDateTime last; };
    QMap<QString, Row> rows;
    for (const Entry &e : m_entries) {
        const QDate d = e.when.date();
        if ((from.isValid() && d < from) || (to.isValid() && d > to)) continue;
        // the same song is found by its CCLI number, else by its title (without capitals and accents)
        const QString key = e.ccli.isEmpty() ? "t:" + SongFilter::fold(e.title) : "c:" + e.ccli;
        Row &r = rows[key];
        if (r.count == 0 || e.when >= r.last) { r.title = e.title; r.author = e.author; r.last = e.when; }
        if (r.ccli.isEmpty()) r.ccli = e.ccli;
        ++r.count;
    }
    QList<Row> list = rows.values();
    std::sort(list.begin(), list.end(), [](const Row &a, const Row &b) {
        return a.count != b.count ? a.count > b.count : a.title.compare(b.title, Qt::CaseInsensitive) < 0;
    });
    QVariantList out;
    for (const Row &r : list)
        out << QVariantMap{{"title", r.title}, {"ccli", r.ccli}, {"author", r.author}, {"count", r.count},
                           {"last", r.last.date().toString("yyyy-MM-dd")}};
    return out;
}

QVariantList UsageLog::summary(const QString &period) const {
    QDate from, to;
    range(period, QDate::currentDate(), &from, &to);
    return summary(from, to);
}

int UsageLog::uses(const QString &period) const {
    int n = 0;
    for (const QVariant &v : summary(period)) n += v.toMap()["count"].toInt();
    return n;
}

QString UsageLog::periodText(const QString &period) const {
    QDate from, to;
    range(period, QDate::currentDate(), &from, &to);
    if (period == "year") return QString::number(from.year());
    if (!from.isValid()) return QStringLiteral("alles");
    return QLocale(QLocale::Dutch).toString(from, "MMMM yyyy");
}

static QString csvCell(QString s) {
    if (!s.isEmpty() && QStringLiteral("=+-@").contains(s[0])) s.prepend('\'');   // a spreadsheet must not read a title as a formula
    if (s.contains(';') || s.contains('"') || s.contains('\n')) s = '"' + s.replace('"', "\"\"") + '"';
    return s;
}

QString UsageLog::csv(const QDate &from, const QDate &to) const {
    QString out = "Titel;CCLI-nummer;Auteur;Aantal keer;Laatst gebruikt\r\n";
    for (const QVariant &v : summary(from, to)) {
        const QVariantMap m = v.toMap();
        out += csvCell(m["title"].toString()) + ';' + csvCell(m["ccli"].toString()) + ';' + csvCell(m["author"].toString()) + ';'
             + QString::number(m["count"].toInt()) + ';' + m["last"].toString() + "\r\n";
    }
    return out;
}

bool UsageLog::exportCsv(const QUrl &file, const QString &period) const {
    QFile f(file.toLocalFile());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QDate from, to;
    range(period, QDate::currentDate(), &from, &to);
    f.write("\xEF\xBB\xBF");                       // so that Excel reads the accents
    f.write(csv(from, to).toUtf8());
    return true;
}

void UsageLog::load() {
    QFile f(path());
    if (!f.open(QIODevice::ReadOnly)) return;
    for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
        const QJsonObject o = v.toObject();
        const QDateTime when = QDateTime::fromString(o["when"].toString(), Qt::ISODate);
        if (when.isValid()) m_entries.append({when, o["title"].toString(), o["ccli"].toString(), o["author"].toString(), o["service"].toString()});
    }
}

void UsageLog::save() const {
    QJsonArray arr;
    for (const Entry &e : m_entries)
        arr.append(QJsonObject{{"when", e.when.toString(Qt::ISODate)}, {"title", e.title}, {"ccli", e.ccli},
                               {"author", e.author}, {"service", e.service}});
    QFile f(path());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
