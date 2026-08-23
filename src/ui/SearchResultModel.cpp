#include "SearchResultModel.h"

#include <QFileInfo>
#include <QApplication>
#include <QStyle>

namespace quickary {

SearchResultModel::SearchResultModel(QObject* parent) : QAbstractListModel(parent) {}

int SearchResultModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : items_.size();
}

QString SearchResultModel::iconKey(const SearchItem& item) const
{
    if (item.kind == ItemKind::Folder || item.kind == ItemKind::Favorite) return QStringLiteral("<folder>");
    if (item.kind == ItemKind::Command) return QStringLiteral("<command>");
    if (item.kind == ItemKind::Web) return QStringLiteral("<web>");
    const QString suffix = QFileInfo(item.path).suffix().toCaseFolded();
    return suffix.isEmpty() ? item.path.toCaseFolded() : suffix;
}

QIcon SearchResultModel::iconFor(const SearchItem& item) const
{
    const QString key = iconKey(item);
    if (auto* cached = iconCache_.object(key)) return *cached;
    QIcon icon;
    if (!item.path.isEmpty()) icon = iconProvider_.icon(QFileInfo(item.path));
    if (icon.isNull()) icon = QApplication::style()->standardIcon(
        item.kind == ItemKind::Folder || item.kind == ItemKind::Favorite ? QStyle::SP_DirIcon : QStyle::SP_FileIcon);
    iconCache_.insert(key, new QIcon(icon));
    return icon;
}

QVariant SearchResultModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) return {};
    const auto& item = items_.at(index.row());
    switch (role) {
    case Qt::DisplayRole: return item.title;
    case Qt::DecorationRole: return iconFor(item);
    case SubtitleRole: return item.subtitle;
    case KindRole: return static_cast<int>(item.kind);
    case PathRole: return item.path;
    case ItemRole: return QVariant::fromValue(item);
    default: return {};
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    return {{Qt::DisplayRole, "title"}, {SubtitleRole, "subtitle"}, {KindRole, "kind"}, {PathRole, "path"}};
}

void SearchResultModel::setItems(QVector<SearchItem> items)
{
    beginResetModel();
    items_ = std::move(items);
    endResetModel();
}

const SearchItem* SearchResultModel::itemAt(int row) const
{
    return row >= 0 && row < items_.size() ? &items_.at(row) : nullptr;
}

} // namespace quickary
