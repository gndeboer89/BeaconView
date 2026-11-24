#pragma once
#include <QAbstractListModel>
#include <QUrl>
#include <QVariantMap>

// Local song database ("liederen"), stored as JSON in the app data folder.
class SongLibrary : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { TitleRole = Qt::UserRole + 1, NumberRole, LyricsRole, AuthorRole, CcliRole, CopyrightRole };
    struct Song { QString title, number, lyrics, author, ccli, copyright; };

    explicit SongLibrary(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_songs.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void add(const QString &title, const QString &number, const QString &lyrics);
    Q_INVOKABLE void update(int i, const QString &title, const QString &number, const QString &lyrics);
    Q_INVOKABLE void remove(int i);
    // Adds songs from SongSelect downloads (.txt, .usr). Returns {added, existing, failed} (counts) and
    // `titles` (the added songs). A song that is already there (same CCLI number, or same title and text) is skipped.
    Q_INVOKABLE QVariantMap importFiles(const QList<QUrl> &files);

private:
    void load();
    void save() const;
    QString path() const;
    QList<Song> m_songs;
};
