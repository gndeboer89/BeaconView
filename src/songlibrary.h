#pragma once
#include <QAbstractListModel>
#include <QTimer>
#include <memory>
#include "songdatabase.h"
#include <QUrl>
#include <QVariantMap>

// The songs ("liederen"), kept in the song database (songs.db in the data folder), which the main program and the song program share.
// The list follows changes made by the other program (checked every second and a half).
class SongLibrary : public QAbstractListModel {
    Q_OBJECT
    // The bundles ("Opwekking", "Hillsong") and tags ("Kerst", "Pasen") that are in use, for the choices in the editor and the filter.
    Q_PROPERTY(QStringList categories READ categories NOTIFY metaChanged)
    Q_PROPERTY(QStringList tags READ tags NOTIFY metaChanged)
    Q_PROPERTY(QString storageError READ storageError NOTIFY storageErrorChanged)   // empty when all is well; else the songs are not being saved
public:
    enum Roles { TitleRole = Qt::UserRole + 1, NumberRole, LyricsRole, AuthorRole, CcliRole, CopyrightRole, OrderRole, CategoryRole, TagsRole };
    struct Song { QString title, number, lyrics, author, ccli, copyright, order, category; QStringList tags; qint64 id = 0, rev = 1; };

    // `dbPath` empty: the database of this program (songs.db in the data folder).
    explicit SongLibrary(const QString &dbPath = QString(), QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_songs.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // `order` is the standard order of the parts ("Couplet 1, Refrein, Couplet 2, Refrein"); empty = as written.
    // `category` is the bundle the song belongs to (one); `tags` are themes (as many as you like). Capitals do not matter:
    // "kerst" and "Kerst" are the same tag, and the spelling that was used first is kept.
    // add returns the id of the new song (0 when it could not be saved).
    Q_INVOKABLE qint64 add(const QString &title, const QString &number, const QString &lyrics, const QString &order = QString(),
                           const QString &category = QString(), const QStringList &tags = {});
    // `expectedRev` (the "rev" that get() gave when the song was opened): when the song was changed by someone else since, nothing is
    // saved and false is returned. -1 saves in any case.
    Q_INVOKABLE bool update(int i, const QString &title, const QString &number, const QString &lyrics, const QString &order = QString(),
                            const QString &category = QString(), const QStringList &tags = {}, qint64 expectedRev = -1);
    Q_INVOKABLE void remove(int i);
    // All fields of one song: {id, rev, title, number, lyrics, order, category, tags, author, ccli, copyright}; empty map for a wrong row.
    Q_INVOKABLE QVariantMap get(int i) const;
    // Author and CCLI number of a song (the editor of the page changes them next to update()).
    Q_INVOKABLE void setCredits(int i, const QString &author, const QString &ccli);
    // Adds songs from SongSelect downloads (.txt, .usr). Returns {added, existing, failed} (counts) and
    // `titles` (the added songs). A song that is already there (same CCLI number, or same title and text) is skipped.
    Q_INVOKABLE QVariantMap importFiles(const QList<QUrl> &files);
    // Gives a bundle or a tag to several songs at once (for example after an import); `rows` are rows of the library.
    Q_INVOKABLE void setCategory(const QList<int> &rows, const QString &category);
    Q_INVOKABLE void addTag(const QList<int> &rows, const QString &tag);
    Q_INVOKABLE qint64 idAt(int row) const { return (row >= 0 && row < m_songs.size()) ? m_songs[row].id : 0; }
    Q_INVOKABLE int rowOfId(qint64 id) const;           // -1 when the song is gone
    Q_INVOKABLE void reload();                           // reads the list from the database again
    Q_INVOKABLE void checkNow();                         // looks at once whether the other program changed something
    QString storageError() const { return m_storageError; }
    QStringList categories() const;
    QStringList tags() const;
    // "#kerst,  Pasen ," -> ["kerst", "Pasen"]
    static QStringList parseTags(const QString &text);
signals:
    void metaChanged();
    void storageErrorChanged();
    void externallyChanged();    // the other program changed songs: the rows may have moved (use rowOfId)

private:
    QString canonicalCategory(const QString &c) const;
    QStringList canonicalTags(const QStringList &t) const;
    void firstRun(const QString &jsonPath);
    void setError(const QString &e);
    static SongDatabase::Row toRow(const Song &s);
    static Song fromRow(const SongDatabase::Row &r);
    std::unique_ptr<SongDatabase> m_db;
    QTimer m_poll;
    QString m_storageError;
    QList<Song> m_songs;
};
