#include "songdatabase.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <atomic>

static std::atomic<int> g_connectionCounter{0};

// An empty QString() is a null string, which would go into the database as NULL: store it as an empty text.
static QVariant text(const QString &v) { return v.isNull() ? QString("") : v; }

SongDatabase::SongDatabase(const QString &path) : m_path(path) {
    if (path != ":memory:") QDir().mkpath(QFileInfo(path).absolutePath());
    m_connection = QStringLiteral("beaconview-songs-%1").arg(++g_connectionCounter);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connection);
    db.setDatabaseName(path);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");       // wait for the other program when it is writing
    if (!db.open()) { m_error = db.lastError().text(); return; }
    m_open = true;
    exec("PRAGMA foreign_keys=ON");
    exec("PRAGMA journal_mode=WAL");                          // reading and writing at the same time by two programs
    exec("PRAGMA synchronous=NORMAL");
    exec("CREATE TABLE IF NOT EXISTS meta(key TEXT PRIMARY KEY, value TEXT NOT NULL)");
    exec("CREATE TABLE IF NOT EXISTS songs("
         "id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL, number TEXT NOT NULL DEFAULT '', lyrics TEXT NOT NULL DEFAULT '', "
         "author TEXT NOT NULL DEFAULT '', ccli TEXT NOT NULL DEFAULT '', copyright TEXT NOT NULL DEFAULT '', "
         "order_text TEXT NOT NULL DEFAULT '', category TEXT NOT NULL DEFAULT '', rev INTEGER NOT NULL DEFAULT 1, updated TEXT NOT NULL DEFAULT '')");
    exec("CREATE TABLE IF NOT EXISTS song_tags("
         "song_id INTEGER NOT NULL REFERENCES songs(id) ON DELETE CASCADE, pos INTEGER NOT NULL, tag TEXT NOT NULL, PRIMARY KEY(song_id, pos))");
    exec("CREATE INDEX IF NOT EXISTS idx_song_tags_song ON song_tags(song_id)");
    {   // only the first program writes the version: a second program starting up must not look like a change
        QSqlQuery v(db);
        if (v.exec("PRAGMA user_version") && v.next() && v.value(0).toInt() == 0) exec("PRAGMA user_version=1");
    }
    changedByOthers();                                        // takes the starting value
}

SongDatabase::~SongDatabase() {
    if (m_open) QSqlDatabase::database(m_connection, false).close();
    QSqlDatabase::removeDatabase(m_connection);
}

bool SongDatabase::exec(const QString &sql) const {
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    return q.exec(sql);
}

QString SongDatabase::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/songs.db";
}

QList<SongDatabase::Row> SongDatabase::all() const {
    QList<Row> rows;
    if (!m_open) return rows;
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    QSqlQuery q(db);
    if (q.exec("SELECT id, title, number, lyrics, author, ccli, copyright, order_text, category, rev FROM songs ORDER BY id")) {
        while (q.next()) {
            Row r;
            r.id = q.value(0).toLongLong(); r.title = q.value(1).toString(); r.number = q.value(2).toString();
            r.lyrics = q.value(3).toString(); r.author = q.value(4).toString(); r.ccli = q.value(5).toString();
            r.copyright = q.value(6).toString(); r.order = q.value(7).toString(); r.category = q.value(8).toString();
            r.rev = q.value(9).toLongLong();
            rows << r;
        }
    }
    QHash<qint64, QStringList> tags;
    QSqlQuery t(db);
    if (t.exec("SELECT song_id, tag FROM song_tags ORDER BY song_id, pos")) while (t.next()) tags[t.value(0).toLongLong()] << t.value(1).toString();
    for (Row &r : rows) r.tags = tags.value(r.id);
    return rows;
}

int SongDatabase::count() const {
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    return (q.exec("SELECT COUNT(*) FROM songs") && q.next()) ? q.value(0).toInt() : 0;
}

static bool writeTags(QSqlDatabase db, qint64 id, const QStringList &tags) {
    QSqlQuery del(db);
    del.prepare("DELETE FROM song_tags WHERE song_id=?");
    del.addBindValue(id);
    if (!del.exec()) return false;
    for (int i = 0; i < tags.size(); ++i) {
        QSqlQuery ins(db);
        ins.prepare("INSERT INTO song_tags(song_id, pos, tag) VALUES(?,?,?)");
        ins.addBindValue(id); ins.addBindValue(i); ins.addBindValue(text(tags[i]));
        if (!ins.exec()) return false;
    }
    return true;
}

qint64 SongDatabase::insert(const Row &r) {
    if (!m_open) return 0;
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    const bool own = db.transaction();                       // all or nothing (a no-op inside a larger begin())
    QSqlQuery q(db);
    q.prepare("INSERT INTO songs(title, number, lyrics, author, ccli, copyright, order_text, category, rev, updated) VALUES(?,?,?,?,?,?,?,?,1,?)");
    for (const QString &v : {r.title, r.number, r.lyrics, r.author, r.ccli, r.copyright, r.order, r.category}) q.addBindValue(text(v));
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) { if (own) db.rollback(); return 0; }
    const qint64 id = q.lastInsertId().toLongLong();
    if (!writeTags(db, id, r.tags)) { if (own) db.rollback(); return 0; }
    if (own) db.commit();
    return id;
}

SongDatabase::UpdateResult SongDatabase::update(const Row &r, qint64 expectedRev, qint64 *newRev) {
    if (!m_open) return Failed;
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    const bool own = db.transaction();
    const qint64 cur = revisionOf(r.id);
    if (cur == 0) { if (own) db.rollback(); return Gone; }
    if (expectedRev >= 0 && cur != expectedRev) { if (own) db.rollback(); return Conflict; }
    QSqlQuery q(db);
    q.prepare("UPDATE songs SET title=?, number=?, lyrics=?, author=?, ccli=?, copyright=?, order_text=?, category=?, rev=rev+1, updated=? WHERE id=?");
    for (const QString &v : {r.title, r.number, r.lyrics, r.author, r.ccli, r.copyright, r.order, r.category}) q.addBindValue(text(v));
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.addBindValue(r.id);
    if (!q.exec() || !writeTags(db, r.id, r.tags)) { if (own) db.rollback(); return Failed; }
    if (own) db.commit();
    if (newRev) *newRev = cur + 1;
    return Updated;
}

bool SongDatabase::remove(qint64 id) {
    if (!m_open) return false;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare("DELETE FROM songs WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

qint64 SongDatabase::revisionOf(qint64 id) const {
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare("SELECT rev FROM songs WHERE id=?");
    q.addBindValue(id);
    return (q.exec() && q.next()) ? q.value(0).toLongLong() : 0;
}

bool SongDatabase::begin() { return m_open && QSqlDatabase::database(m_connection, false).transaction(); }
bool SongDatabase::commit() { return m_open && QSqlDatabase::database(m_connection, false).commit(); }
void SongDatabase::rollback() { if (m_open) QSqlDatabase::database(m_connection, false).rollback(); }

bool SongDatabase::changedByOthers() {
    if (!m_open) return false;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    if (!q.exec("PRAGMA data_version") || !q.next()) return false;
    const qint64 v = q.value(0).toLongLong();
    const bool changed = m_dataVersion >= 0 && v != m_dataVersion;
    m_dataVersion = v;
    return changed;
}

QString SongDatabase::meta(const QString &key) const {
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare("SELECT value FROM meta WHERE key=?");
    q.addBindValue(key);
    return (q.exec() && q.next()) ? q.value(0).toString() : QString();
}

bool SongDatabase::claimMeta(const QString &key, const QString &value) {
    if (!m_open) return false;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare("INSERT OR IGNORE INTO meta(key, value) VALUES(?,?)");     // the database lets only one program insert it
    q.addBindValue(key); q.addBindValue(value);
    return q.exec() && q.numRowsAffected() == 1;
}

bool SongDatabase::backupTo(const QString &file) const {
    if (!m_open || QFile::exists(file)) return false;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare("VACUUM INTO ?");
    q.addBindValue(file);
    return q.exec();
}

void SongDatabase::backupIfDue(const QString &dir, int keep) const {
    if (!m_open || m_path == ":memory:") return;
    QDir().mkpath(dir);
    const QString today = dir + "/songs-" + QDate::currentDate().toString("yyyy-MM-dd") + ".db";
    if (!QFile::exists(today)) backupTo(today);
    QDir d(dir);
    const QStringList files = d.entryList({"songs-*.db"}, QDir::Files, QDir::Name);
    for (int i = 0; i < files.size() - keep; ++i) QFile::remove(d.filePath(files[i]));
}
