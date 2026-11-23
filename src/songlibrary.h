#pragma once
#include <QAbstractListModel>

// Local song database ("liederen"), stored as JSON in the app data folder.
class SongLibrary : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { TitleRole = Qt::UserRole + 1, NumberRole, LyricsRole };
    struct Song { QString title, number, lyrics; };

    explicit SongLibrary(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_songs.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void add(const QString &title, const QString &number, const QString &lyrics);
    Q_INVOKABLE void update(int i, const QString &title, const QString &number, const QString &lyrics);
    Q_INVOKABLE void remove(int i);

private:
    void load();
    void save() const;
    QString path() const;
    QList<Song> m_songs;
};
