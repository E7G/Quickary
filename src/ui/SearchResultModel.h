#pragma once

#include "../core/SearchTypes.h"
#include <QAbstractListModel>
#include <QCache>
#include <QFileIconProvider>

namespace quickary {

class SearchResultModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { ItemRole = Qt::UserRole + 1, SubtitleRole, KindRole, PathRole };

    explicit SearchResultModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(QVector<SearchItem> items);
    const SearchItem* itemAt(int row) const;

private:
    QString iconKey(const SearchItem& item) const;
    QIcon iconFor(const SearchItem& item) const;

    QVector<SearchItem> items_;
    mutable QFileIconProvider iconProvider_;
    mutable QCache<QString, QIcon> iconCache_{96};
};

} // namespace quickary
