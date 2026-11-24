#include "songlibrary.h"
#include "songimporter.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

SongLibrary::SongLibrary(QObject *parent) : QAbstractListModel(parent) { load(); }

QString SongLibrary::path() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/songs.json";
}

QVariant SongLibrary::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_songs.size()) return {};
    const Song &s = m_songs[index.row()];
    switch (role) {
    case TitleRole: return s.title;
    case NumberRole: return s.number;
    case LyricsRole: return s.lyrics;
    case AuthorRole: return s.author;
    case CcliRole: return s.ccli;
    case CopyrightRole: return s.copyright;
    case OrderRole: return s.order;
    }
    return {};
}

QHash<int, QByteArray> SongLibrary::roleNames() const {
    return {{TitleRole, "title"}, {NumberRole, "number"}, {LyricsRole, "lyrics"},
            {AuthorRole, "author"}, {CcliRole, "ccli"}, {CopyrightRole, "copyright"}, {OrderRole, "order"}};
}

void SongLibrary::add(const QString &title, const QString &number, const QString &lyrics, const QString &order) {
    beginInsertRows({}, m_songs.size(), m_songs.size());
    m_songs.append({title, number, lyrics, QString(), QString(), QString(), order.trimmed()});
    endInsertRows();
    save();
}

void SongLibrary::update(int i, const QString &title, const QString &number, const QString &lyrics, const QString &order) {
    if (i < 0 || i >= m_songs.size()) return;
    Song &s = m_songs[i];            // the credits (author, CCLI number) are not edited here and stay
    s.title = title; s.number = number; s.lyrics = lyrics; s.order = order.trimmed();
    emit dataChanged(index(i), index(i));
    save();
}

void SongLibrary::remove(int i) {
    if (i < 0 || i >= m_songs.size()) return;
    beginRemoveRows({}, i, i);
    m_songs.removeAt(i);
    endRemoveRows();
    save();
}

void SongLibrary::load() {
    QFile f(path());
    if (f.open(QIODevice::ReadOnly)) {
        for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
            const QJsonObject o = v.toObject();
            m_songs.append({o["title"].toString(), o["number"].toString(), o["lyrics"].toString(),
                            o["author"].toString(), o["ccli"].toString(), o["copyright"].toString(), o["order"].toString()});
        }
        return;
    }
    // First run: one public-domain example (Psalm 23, Statenvertaling).
    m_songs.append({"Psalm 23 (voorbeeld)", "",
        "De HEERE is mijn Herder,\nmij zal niets ontbreken.\n\n"
        "Hij doet mij nederliggen\nin grazige weiden;\n\n"
        "Hij voert mij zachtjes aan\nzeer stille wateren.\n\n"
        "Hij verkwikt mijn ziel;\nHij leidt mij in het spoor der gerechtigheid\nom Zijns Naams wil."});
    save();
}

void SongLibrary::save() const {
    QJsonArray arr;
    for (const Song &s : m_songs)
        arr.append(QJsonObject{{"title", s.title}, {"number", s.number}, {"lyrics", s.lyrics},
                               {"author", s.author}, {"ccli", s.ccli}, {"copyright", s.copyright}, {"order", s.order}});
    QFile f(path());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(QJsonDocument(arr).toJson());
}

QVariantMap SongLibrary::importFiles(const QList<QUrl> &files) {
    int added = 0, existing = 0, failed = 0;
    QStringList titles;
    for (const QUrl &url : files) {
        QFile f(url.toLocalFile());
        QList<ImportedSong> found;
        if (f.open(QIODevice::ReadOnly)) found = SongImporter::parse(f.readAll(), url.fileName());
        if (found.isEmpty()) { ++failed; continue; }
        for (const ImportedSong &in : found) {
            bool dup = false;
            for (const Song &s : std::as_const(m_songs)) {
                if ((!in.ccli.isEmpty() && s.ccli == in.ccli)
                    || (s.title.compare(in.title, Qt::CaseInsensitive) == 0 && s.lyrics.trimmed() == in.lyrics.trimmed())) { dup = true; break; }
            }
            if (dup) { ++existing; continue; }
            beginInsertRows({}, m_songs.size(), m_songs.size());
            m_songs.append({in.title, QString(), in.lyrics, in.author, in.ccli, in.copyright, QString()});
            endInsertRows();
            titles << in.title;
            ++added;
        }
    }
    if (added) save();
    return {{"added", added}, {"existing", existing}, {"failed", failed}, {"titles", titles}};
}
