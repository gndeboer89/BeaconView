#pragma once
#include <QSortFilterProxyModel>

// The song list as the user sees it: sorted by title and filtered by a search text. The text is matched against
// title, number, author, CCLI number and the lyrics, without regard to case or accents; every word has to match.
class SongFilter : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int total READ total NOTIFY totalChanged)           // songs in the library, whatever the search
public:
    explicit SongFilter(QObject *parent = nullptr);
    void setSourceModel(QAbstractItemModel *m) override;
    QString query() const { return m_query; }
    void setQuery(const QString &q);
    int total() const { return sourceModel() ? sourceModel()->rowCount() : 0; }
    // Row in the library (what presenter.addSong and the editor need) for a row of this list.
    Q_INVOKABLE int sourceRow(int row) const;
    // Lower case without accents: "Génade" and "genade" are the same.
    static QString fold(const QString &s);
signals:
    void queryChanged();
    void totalChanged();
protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;   // by title, without regard to case and accents
private:
    QString m_query;
    QStringList m_words;
};
