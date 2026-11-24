#include "songfilter.h"
#include "songlibrary.h"

SongFilter::SongFilter(QObject *parent) : QSortFilterProxyModel(parent) {
    setSortRole(SongLibrary::TitleRole);
    setDynamicSortFilter(true);
}

void SongFilter::setSourceModel(QAbstractItemModel *m) {
    QSortFilterProxyModel::setSourceModel(m);
    sort(0);
    if (m) {
        connect(m, &QAbstractItemModel::rowsInserted, this, &SongFilter::totalChanged);
        connect(m, &QAbstractItemModel::rowsRemoved, this, &SongFilter::totalChanged);
        connect(m, &QAbstractItemModel::modelReset, this, &SongFilter::totalChanged);
    }
}

QString SongFilter::fold(const QString &s) {
    QString out;
    for (const QChar c : s.normalized(QString::NormalizationForm_D))
        if (c.category() != QChar::Mark_NonSpacing) out += c;
    return out.toCaseFolded();
}

void SongFilter::setQuery(const QString &q) {
    if (q == m_query) return;
    m_query = q;
    m_words = fold(q).split(' ', Qt::SkipEmptyParts);
    emit queryChanged();
    invalidateFilter();
}

int SongFilter::sourceRow(int row) const {
    const QModelIndex src = mapToSource(index(row, 0));
    return src.isValid() ? src.row() : -1;
}

bool SongFilter::filterAcceptsRow(int row, const QModelIndex &parent) const {
    if (m_words.isEmpty()) return true;
    const QModelIndex i = sourceModel()->index(row, 0, parent);
    const QString hay = fold(sourceModel()->data(i, SongLibrary::TitleRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::NumberRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::AuthorRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::CcliRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::LyricsRole).toString());
    for (const QString &w : m_words) if (!hay.contains(w)) return false;
    return true;
}

bool SongFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    // the locale of the computer decides how letters sort, which differs per system: fold case and accents ourselves
    return fold(sourceModel()->data(left, SongLibrary::TitleRole).toString())
         < fold(sourceModel()->data(right, SongLibrary::TitleRole).toString());
}
