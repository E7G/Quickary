#pragma once

#include "../core/SearchTypes.h"
#include <QAbstractTableModel>
#include <QCache>
#include <QFileIconProvider>

namespace quickary {

class SearchResultModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        NameColumn = 0,
        PathColumn,
        TypeColumn,
        SizeColumn,
        ModifiedColumn,
        ColumnCount
    };

    enum Role {
        ItemRole = Qt::UserRole + 1,
        SubtitleRole,
        KindRole,
        PathRole,
        SizeRole,
        ModifiedRole,
        ScoreRole,
        ProviderRole
    };

    explicit SearchResultModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(QVector<SearchItem> items);
    const SearchItem* itemAt(int row) const;
    const QVector<SearchItem>& items() const { return items_; }

    static QString kindLabel(ItemKind kind);
    static QString formatSize(quint64 bytes);

private:
    QString iconKey(const SearchItem& item) const;
    QIcon iconFor(const SearchItem& item) const;

    QVector<SearchItem> items_;
    mutable QFileIconProvider iconProvider_;
    mutable QCache<QString, QIcon> iconCache_{128};
};

} // namespace quickary
