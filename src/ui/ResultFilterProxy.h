#pragma once

#include <QSet>
#include <QSortFilterProxyModel>

namespace quickary {

class ResultFilterProxy final : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ResultFilterProxy(QObject* parent = nullptr);

    void setKindFilters(const QSet<int>& kinds);
    const QSet<int>& kindFilters() const { return kinds_; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const override;

private:
    QSet<int> kinds_;
};

} // namespace quickary
