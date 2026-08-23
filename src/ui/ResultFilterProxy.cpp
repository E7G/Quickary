#include "ResultFilterProxy.h"

#include "SearchResultModel.h"

#include <QString>

namespace quickary {

ResultFilterProxy::ResultFilterProxy(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void ResultFilterProxy::setKindFilters(const QSet<int>& kinds)
{
    if (kinds_ == kinds) return;
    kinds_ = kinds;
    invalidateFilter();
}

bool ResultFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (kinds_.isEmpty()) return true;
    const QModelIndex index = sourceModel()->index(sourceRow, SearchResultModel::NameColumn, sourceParent);
    return kinds_.contains(index.data(SearchResultModel::KindRole).toInt());
}

bool ResultFilterProxy::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const
{
    const int column = sourceLeft.column();
    if (column == SearchResultModel::SizeColumn) {
        return sourceLeft.data(SearchResultModel::SizeRole).toULongLong()
            < sourceRight.data(SearchResultModel::SizeRole).toULongLong();
    }
    if (column == SearchResultModel::ModifiedColumn) {
        return sourceLeft.data(SearchResultModel::ModifiedRole).toDateTime()
            < sourceRight.data(SearchResultModel::ModifiedRole).toDateTime();
    }
    if (column == SearchResultModel::TypeColumn) {
        return sourceLeft.data(SearchResultModel::KindRole).toInt()
            < sourceRight.data(SearchResultModel::KindRole).toInt();
    }
    return QString::localeAwareCompare(sourceLeft.data(Qt::DisplayRole).toString(),
                                       sourceRight.data(Qt::DisplayRole).toString()) < 0;
}

} // namespace quickary
