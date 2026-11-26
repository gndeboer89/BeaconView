#pragma once
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// The song database: one SQLite file that the main program and the song program both use. Every program has its own connection;
// SQLite takes care that they do not write at the same moment (WAL mode, busy timeout), and every song has a revision number that
// goes up with each change, so a program can see that someone else changed a song while it was being edited.
class SongDatabase {
public:
    struct Row {
        qint64 id = 0;
        QString title, number, lyrics, author, ccli, copyright, order, category;
        QStringList tags;
        qint64 rev = 1;
    };
    enum UpdateResult { Updated, Conflict, Gone, Failed };

    // `path` ":memory:" or a file; a missing file (and its folder) is made.
    explicit SongDatabase(const QString &path);
    ~SongDatabase();
    SongDatabase(const SongDatabase &) = delete;
    SongDatabase &operator=(const SongDatabase &) = delete;

    bool isOpen() const { return m_open; }
    QString error() const { return m_error; }
    QString path() const { return m_path; }

    QList<Row> all() const;
    int count() const;
    // Adds a song, returns its id (0 on failure). `rev` of the row is set to 1.
    qint64 insert(const Row &r);
    // Changes a song. `expectedRev` >= 0: only when the song is still at that revision (otherwise Conflict); -1: always.
    // On Updated, `*newRev` is the new revision.
    UpdateResult update(const Row &r, qint64 expectedRev, qint64 *newRev = nullptr);
    bool remove(qint64 id);
    qint64 revisionOf(qint64 id) const;            // 0 when the song does not exist
    // Several changes as one: all or nothing.
    bool begin();
    bool commit();
    void rollback();

    // True when another connection (the other program) changed the database since the last call.
    bool changedByOthers();

    // Small values that belong to the database itself ("json_migrated", ...). claimMeta is true for exactly one caller, also when two programs start at once.
    QString meta(const QString &key) const;
    bool claimMeta(const QString &key, const QString &value = "1");

    // A consistent copy of the database in `file` (must not exist).
    bool backupTo(const QString &file) const;
    // One copy per day in `dir`, the newest `keep` are kept.
    void backupIfDue(const QString &dir, int keep = 14) const;

    static QString defaultPath();                  // songs.db in the data folder of the program

private:
    bool exec(const QString &sql) const;
    QString m_path, m_error, m_connection;
    bool m_open = false;
    qint64 m_dataVersion = -1;
};
