#include "songlibrary.h"
#include "songimporter.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
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
    case CategoryRole: return s.category;
    case TagsRole: return s.tags;
    }
    return {};
}

QHash<int, QByteArray> SongLibrary::roleNames() const {
    return {{TitleRole, "title"}, {NumberRole, "number"}, {LyricsRole, "lyrics"},
            {AuthorRole, "author"}, {CcliRole, "ccli"}, {CopyrightRole, "copyright"}, {OrderRole, "order"},
            {CategoryRole, "category"}, {TagsRole, "tags"}};
}

static QString foldKey(const QString &s) { return s.simplified().toCaseFolded(); }

QString SongLibrary::canonicalCategory(const QString &c) const {
    const QString t = c.simplified();
    if (t.isEmpty()) return t;
    for (const Song &s : m_songs) if (foldKey(s.category) == foldKey(t)) return s.category;   // the spelling that was used first
    return t;
}

QStringList SongLibrary::canonicalTags(const QStringList &in) const {
    QStringList out;
    for (QString t : in) {
        t = t.simplified();
        while (t.startsWith('#')) t = t.mid(1).simplified();
        if (t.isEmpty()) continue;
        QString use = t;
        for (const Song &s : m_songs) {
            for (const QString &e : s.tags) if (foldKey(e) == foldKey(t)) { use = e; break; }
            if (use != t) break;
        }
        bool dup = false;
        for (const QString &o : out) if (foldKey(o) == foldKey(use)) dup = true;
        if (!dup) out << use;
    }
    return out;
}

QStringList SongLibrary::parseTags(const QString &text) {
    QStringList out;
    for (const QString &p : text.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts)) if (!p.simplified().isEmpty()) out << p.simplified();
    return out;
}

static QStringList uniqueSorted(const QStringList &in) {
    QStringList out;
    for (const QString &s : in) {
        bool dup = false;
        for (const QString &o : out) if (foldKey(o) == foldKey(s)) dup = true;
        if (!dup && !s.isEmpty()) out << s;
    }
    std::sort(out.begin(), out.end(), [](const QString &a, const QString &b) { return foldKey(a) < foldKey(b); });
    return out;
}

QStringList SongLibrary::categories() const {
    QStringList all;
    for (const Song &s : m_songs) all << s.category;
    return uniqueSorted(all);
}

QStringList SongLibrary::tags() const {
    QStringList all;
    for (const Song &s : m_songs) all << s.tags;
    return uniqueSorted(all);
}

void SongLibrary::add(const QString &title, const QString &number, const QString &lyrics, const QString &order,
                      const QString &category, const QStringList &tags) {
    const QString cat = canonicalCategory(category);
    const QStringList tg = canonicalTags(tags);
    beginInsertRows({}, m_songs.size(), m_songs.size());
    m_songs.append({title, number, lyrics, QString(), QString(), QString(), order.trimmed(), cat, tg});
    endInsertRows();
    save();
    emit metaChanged();
}

void SongLibrary::update(int i, const QString &title, const QString &number, const QString &lyrics, const QString &order,
                         const QString &category, const QStringList &tags) {
    if (i < 0 || i >= m_songs.size()) return;
    const QString cat = canonicalCategory(category);
    const QStringList tg = canonicalTags(tags);
    Song &s = m_songs[i];            // the credits (author, CCLI number) are not edited here and stay
    s.title = title; s.number = number; s.lyrics = lyrics; s.order = order.trimmed(); s.category = cat; s.tags = tg;
    emit dataChanged(index(i), index(i));
    save();
    emit metaChanged();
}

QVariantMap SongLibrary::get(int i) const {
    if (i < 0 || i >= m_songs.size()) return {};
    const Song &s = m_songs[i];
    return {{"title", s.title}, {"number", s.number}, {"lyrics", s.lyrics}, {"order", s.order}, {"category", s.category},
            {"tags", s.tags}, {"author", s.author}, {"ccli", s.ccli}, {"copyright", s.copyright}};
}

void SongLibrary::setCredits(int i, const QString &author, const QString &ccli) {
    if (i < 0 || i >= m_songs.size()) return;
    Song &s = m_songs[i];
    const QString a = author.simplified(), c = QString(ccli).remove(QRegularExpression("\\D"));   // a CCLI number is digits
    if (s.author == a && s.ccli == c) return;
    s.author = a; s.ccli = c;
    emit dataChanged(index(i), index(i), {AuthorRole, CcliRole});
    save();
}

void SongLibrary::setCategory(const QList<int> &rows, const QString &category) {
    const QString cat = canonicalCategory(category);
    for (int r : rows) {
        if (r < 0 || r >= m_songs.size()) continue;
        m_songs[r].category = cat;
        emit dataChanged(index(r), index(r), {CategoryRole});
    }
    save();
    emit metaChanged();
}

void SongLibrary::addTag(const QList<int> &rows, const QString &tag) {
    const QStringList t = canonicalTags({tag});
    if (t.isEmpty()) return;
    for (int r : rows) {
        if (r < 0 || r >= m_songs.size()) continue;
        m_songs[r].tags = canonicalTags(m_songs[r].tags + t);
        emit dataChanged(index(r), index(r), {TagsRole});
    }
    save();
    emit metaChanged();
}

void SongLibrary::remove(int i) {
    if (i < 0 || i >= m_songs.size()) return;
    beginRemoveRows({}, i, i);
    m_songs.removeAt(i);
    endRemoveRows();
    save();
    emit metaChanged();
}

void SongLibrary::load() {
    QFile f(path());
    if (f.open(QIODevice::ReadOnly)) {
        for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
            const QJsonObject o = v.toObject();
            m_songs.append({o["title"].toString(), o["number"].toString(), o["lyrics"].toString(),
                            o["author"].toString(), o["ccli"].toString(), o["copyright"].toString(), o["order"].toString(),
                            o["category"].toString().simplified(), [&] { QStringList t; for (const auto &x : o["tags"].toArray()) t << x.toString(); return t; }()});
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
                               {"author", s.author}, {"ccli", s.ccli}, {"copyright", s.copyright}, {"order", s.order}, {"category", s.category}, {"tags", QJsonArray::fromStringList(s.tags)}});
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
            m_songs.append({in.title, QString(), in.lyrics, in.author, in.ccli, in.copyright, QString(), QString(), {}});
            endInsertRows();
            titles << in.title;
            ++added;
        }
    }
    if (added) { save(); emit metaChanged(); }
    return {{"added", added}, {"existing", existing}, {"failed", failed}, {"titles", titles}};
}
