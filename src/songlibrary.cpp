#include "songlibrary.h"
#include "songimporter.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

SongLibrary::SongLibrary(const QString &dbPath, QObject *parent) : QAbstractListModel(parent) {
    const QString path = dbPath.isEmpty() ? SongDatabase::defaultPath() : dbPath;
    m_db = std::make_unique<SongDatabase>(path);
    if (!m_db->isOpen()) {                      // the file cannot be opened: work in memory, and say that nothing is saved
        setError(m_db->error().isEmpty() ? QStringLiteral("De liederen kunnen niet worden opgeslagen.") : m_db->error());
        m_db = std::make_unique<SongDatabase>(":memory:");
    }
    if (m_db->claimMeta("initialized"))        // the first program that sees this database fills it
        firstRun(path == ":memory:" ? QString() : QFileInfo(path).absolutePath() + "/songs.json");
    if (path != ":memory:") m_db->backupIfDue(QFileInfo(path).absolutePath() + "/backups");   // one copy per day
    reload();
    m_poll.setInterval(1500);
    connect(&m_poll, &QTimer::timeout, this, &SongLibrary::checkNow);
    m_poll.start();
}

void SongLibrary::setError(const QString &e) {
    if (e == m_storageError) return;
    m_storageError = e;
    emit storageErrorChanged();
}

SongDatabase::Row SongLibrary::toRow(const Song &s) {
    SongDatabase::Row r;
    r.id = s.id; r.title = s.title; r.number = s.number; r.lyrics = s.lyrics; r.author = s.author; r.ccli = s.ccli;
    r.copyright = s.copyright; r.order = s.order; r.category = s.category; r.tags = s.tags; r.rev = s.rev;
    return r;
}

SongLibrary::Song SongLibrary::fromRow(const SongDatabase::Row &r) {
    return {r.title, r.number, r.lyrics, r.author, r.ccli, r.copyright, r.order, r.category, r.tags, r.id, r.rev};
}

// The first time the database is used: take over the songs of the older version (songs.json), or put in one example.
void SongLibrary::firstRun(const QString &jsonPath) {
    if (m_db->count() > 0) return;
    QFile f(jsonPath);
    if (!jsonPath.isEmpty() && f.open(QIODevice::ReadOnly)) {
        const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
        f.close();
        m_db->begin();
        for (const auto &v : arr) {
            const QJsonObject o = v.toObject();
            SongDatabase::Row r;
            r.title = o["title"].toString(); r.number = o["number"].toString(); r.lyrics = o["lyrics"].toString();
            r.author = o["author"].toString(); r.ccli = o["ccli"].toString(); r.copyright = o["copyright"].toString();
            r.order = o["order"].toString(); r.category = o["category"].toString().simplified();
            for (const auto &x : o["tags"].toArray()) r.tags << x.toString();
            if (!r.title.isEmpty()) m_db->insert(r);
        }
        m_db->commit();
        QFile::remove(jsonPath + ".migrated");
        QFile::rename(jsonPath, jsonPath + ".migrated");       // kept as a copy, but not used any more
        return;
    }
    // first run: one public-domain example (Psalm 23, Statenvertaling)
    SongDatabase::Row r;
    r.title = "Psalm 23 (voorbeeld)";
    r.lyrics = "De HEERE is mijn Herder,\nmij zal niets ontbreken.\n\n"
               "Hij doet mij nederliggen\nin grazige weiden;\n\n"
               "Hij voert mij zachtjes aan\nzeer stille wateren.\n\n"
               "Hij verkwikt mijn ziel;\nHij leidt mij in het spoor der gerechtigheid\nom Zijns Naams wil.";
    m_db->insert(r);
}

void SongLibrary::reload() {
    QList<Song> fresh;
    for (const auto &r : m_db->all()) fresh << fromRow(r);
    beginResetModel();
    m_songs = fresh;
    endResetModel();
    emit metaChanged();
}

void SongLibrary::checkNow() {
    if (!m_db->changedByOthers()) return;
    reload();
    emit externallyChanged();
}

int SongLibrary::rowOfId(qint64 id) const {
    for (int i = 0; i < m_songs.size(); ++i) if (m_songs[i].id == id) return i;
    return -1;
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

qint64 SongLibrary::add(const QString &title, const QString &number, const QString &lyrics, const QString &order,
                        const QString &category, const QStringList &tags) {
    Song s;
    s.title = title; s.number = number; s.lyrics = lyrics; s.order = order.trimmed();
    s.category = canonicalCategory(category); s.tags = canonicalTags(tags);
    s.id = m_db->insert(toRow(s));
    if (s.id == 0) { setError("Het lied kon niet worden opgeslagen."); return 0; }
    beginInsertRows({}, m_songs.size(), m_songs.size());
    m_songs.append(s);
    endInsertRows();
    emit metaChanged();
    return s.id;
}

bool SongLibrary::update(int i, const QString &title, const QString &number, const QString &lyrics, const QString &order,
                         const QString &category, const QStringList &tags, qint64 expectedRev) {
    if (i < 0 || i >= m_songs.size()) return false;
    Song s = m_songs[i];            // the credits (author, CCLI number) are not edited here and stay
    s.title = title; s.number = number; s.lyrics = lyrics; s.order = order.trimmed();
    s.category = canonicalCategory(category); s.tags = canonicalTags(tags);
    qint64 newRev = 0;
    switch (m_db->update(toRow(s), expectedRev, &newRev)) {
    case SongDatabase::Updated: break;
    case SongDatabase::Conflict: return false;                         // someone else changed this song: the caller decides
    case SongDatabase::Gone: checkNow(); return false;
    default: setError("Het lied kon niet worden opgeslagen."); return false;
    }
    s.rev = newRev;
    m_songs[i] = s;
    emit dataChanged(index(i), index(i));
    emit metaChanged();
    return true;
}

QVariantMap SongLibrary::get(int i) const {
    if (i < 0 || i >= m_songs.size()) return {};
    const Song &s = m_songs[i];
    return {{"id", s.id}, {"rev", s.rev}, {"title", s.title}, {"number", s.number}, {"lyrics", s.lyrics}, {"order", s.order},
            {"category", s.category}, {"tags", s.tags}, {"author", s.author}, {"ccli", s.ccli}, {"copyright", s.copyright}};
}

void SongLibrary::setCredits(int i, const QString &author, const QString &ccli) {
    if (i < 0 || i >= m_songs.size()) return;
    Song s = m_songs[i];
    const QString a = author.simplified(), c = QString(ccli).remove(QRegularExpression("\\D"));   // a CCLI number is digits
    if (s.author == a && s.ccli == c) return;
    s.author = a; s.ccli = c;
    qint64 newRev = 0;
    if (m_db->update(toRow(s), -1, &newRev) != SongDatabase::Updated) { setError("Het lied kon niet worden opgeslagen."); return; }
    s.rev = newRev;
    m_songs[i] = s;
    emit dataChanged(index(i), index(i), {AuthorRole, CcliRole});
}

void SongLibrary::setCategory(const QList<int> &rows, const QString &category) {
    const QString cat = canonicalCategory(category);
    m_db->begin();
    for (int r : rows) {
        if (r < 0 || r >= m_songs.size()) continue;
        Song s = m_songs[r];
        s.category = cat;
        qint64 newRev = 0;
        if (m_db->update(toRow(s), -1, &newRev) != SongDatabase::Updated) continue;
        s.rev = newRev;
        m_songs[r] = s;
        emit dataChanged(index(r), index(r), {CategoryRole});
    }
    m_db->commit();
    emit metaChanged();
}

void SongLibrary::addTag(const QList<int> &rows, const QString &tag) {
    const QStringList t = canonicalTags({tag});
    if (t.isEmpty()) return;
    m_db->begin();
    for (int r : rows) {
        if (r < 0 || r >= m_songs.size()) continue;
        Song s = m_songs[r];
        s.tags = canonicalTags(s.tags + t);
        qint64 newRev = 0;
        if (m_db->update(toRow(s), -1, &newRev) != SongDatabase::Updated) continue;
        s.rev = newRev;
        m_songs[r] = s;
        emit dataChanged(index(r), index(r), {TagsRole});
    }
    m_db->commit();
    emit metaChanged();
}

void SongLibrary::remove(int i) {
    if (i < 0 || i >= m_songs.size()) return;
    if (!m_db->remove(m_songs[i].id)) { setError("Het lied kon niet worden verwijderd."); return; }
    beginRemoveRows({}, i, i);
    m_songs.removeAt(i);
    endRemoveRows();
    emit metaChanged();
}

QVariantMap SongLibrary::importFiles(const QList<QUrl> &files) {
    int added = 0, existing = 0, failed = 0;
    QStringList titles;
    checkNow();                                  // songs that the other program added just now count as "already there"
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
            Song s;
            s.title = in.title; s.lyrics = in.lyrics; s.author = in.author; s.ccli = in.ccli; s.copyright = in.copyright;
            s.id = m_db->insert(toRow(s));
            if (s.id == 0) { ++failed; continue; }
            beginInsertRows({}, m_songs.size(), m_songs.size());
            m_songs.append(s);
            endInsertRows();
            titles << in.title;
            ++added;
        }
    }
    if (added) emit metaChanged();
    return {{"added", added}, {"existing", existing}, {"failed", failed}, {"titles", titles}};
}
