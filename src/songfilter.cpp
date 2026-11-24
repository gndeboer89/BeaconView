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

void SongFilter::setCategory(const QString &c) {
    if (c == m_category) return;
    m_category = c;
    emit categoryChanged();
    invalidateFilter();
}

void SongFilter::setTag(const QString &t) {
    if (t == m_tag) return;
    m_tag = t;
    emit tagChanged();
    invalidateFilter();
}

int SongFilter::sourceRow(int row) const {
    const QModelIndex src = mapToSource(index(row, 0));
    return src.isValid() ? src.row() : -1;
}

bool SongFilter::filterAcceptsRow(int row, const QModelIndex &parent) const {
    const QModelIndex i = sourceModel()->index(row, 0, parent);
    if (!m_category.isEmpty() && fold(sourceModel()->data(i, SongLibrary::CategoryRole).toString()) != fold(m_category)) return false;
    if (!m_tag.isEmpty()) {
        bool has = false;
        for (const QString &t : sourceModel()->data(i, SongLibrary::TagsRole).toStringList()) if (fold(t) == fold(m_tag)) has = true;
        if (!has) return false;
    }
    if (m_words.isEmpty()) return true;
    const QString hay = fold(sourceModel()->data(i, SongLibrary::TitleRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::NumberRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::AuthorRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::CcliRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::CategoryRole).toString() + ' '
                             + sourceModel()->data(i, SongLibrary::TagsRole).toStringList().join(' ') + ' '
                             + sourceModel()->data(i, SongLibrary::LyricsRole).toString());
    for (const QString &w : m_words) if (!hay.contains(w)) return false;
    return true;
}

bool SongFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    // the locale of the computer decides how letters sort, which differs per system: fold case and accents ourselves
    return fold(sourceModel()->data(left, SongLibrary::TitleRole).toString())
         < fold(sourceModel()->data(right, SongLibrary::TitleRole).toString());
}
