#pragma once
#include <QAbstractListModel>
#include <QUrl>
#include <QVariantMap>

// Local song database ("liederen"), stored as JSON in the app data folder.
class SongLibrary : public QAbstractListModel {
    Q_OBJECT
    // The bundles ("Opwekking", "Hillsong") and tags ("Kerst", "Pasen") that are in use, for the choices in the editor and the filter.
    Q_PROPERTY(QStringList categories READ categories NOTIFY metaChanged)
    Q_PROPERTY(QStringList tags READ tags NOTIFY metaChanged)
public:
    enum Roles { TitleRole = Qt::UserRole + 1, NumberRole, LyricsRole, AuthorRole, CcliRole, CopyrightRole, OrderRole, CategoryRole, TagsRole };
    struct Song { QString title, number, lyrics, author, ccli, copyright, order, category; QStringList tags; };

    explicit SongLibrary(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_songs.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // `order` is the standard order of the parts ("Couplet 1, Refrein, Couplet 2, Refrein"); empty = as written.
    // `category` is the bundle the song belongs to (one); `tags` are themes (as many as you like). Capitals do not matter:
    // "kerst" and "Kerst" are the same tag, and the spelling that was used first is kept.
    Q_INVOKABLE void add(const QString &title, const QString &number, const QString &lyrics, const QString &order = QString(),
                         const QString &category = QString(), const QStringList &tags = {});
    Q_INVOKABLE void update(int i, const QString &title, const QString &number, const QString &lyrics, const QString &order = QString(),
                            const QString &category = QString(), const QStringList &tags = {});
    Q_INVOKABLE void remove(int i);
    // Adds songs from SongSelect downloads (.txt, .usr). Returns {added, existing, failed} (counts) and
    // `titles` (the added songs). A song that is already there (same CCLI number, or same title and text) is skipped.
    Q_INVOKABLE QVariantMap importFiles(const QList<QUrl> &files);
    // Gives a bundle or a tag to several songs at once (for example after an import); `rows` are rows of the library.
    Q_INVOKABLE void setCategory(const QList<int> &rows, const QString &category);
    Q_INVOKABLE void addTag(const QList<int> &rows, const QString &tag);
    QStringList categories() const;
    QStringList tags() const;
    // "#kerst,  Pasen ," -> ["kerst", "Pasen"]
    static QStringList parseTags(const QString &text);
signals:
    void metaChanged();

private:
    QString canonicalCategory(const QString &c) const;
    QStringList canonicalTags(const QStringList &t) const;
    void load();
    void save() const;
    QString path() const;
    QList<Song> m_songs;
};
