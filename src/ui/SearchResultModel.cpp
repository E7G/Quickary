#include "SearchResultModel.h"

#include <QApplication>
#include <QFileInfo>
#include <QLocale>
#include <QStyle>

namespace quickary {

SearchResultModel::SearchResultModel(QObject* parent) : QAbstractTableModel(parent) {}

int SearchResultModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : items_.size();
}

int SearchResultModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QString SearchResultModel::kindLabel(ItemKind kind)
{
    switch (kind) {
    case ItemKind::File: return QStringLiteral("File");
    case ItemKind::Folder: return QStringLiteral("Folder");
    case ItemKind::Application: return QStringLiteral("Application");
    case ItemKind::Command: return QStringLiteral("Command");
    case ItemKind::Web: return QStringLiteral("Web");
    case ItemKind::Favorite: return QStringLiteral("Favorite");
    }
    return QStringLiteral("Item");
}

QString SearchResultModel::formatSize(quint64 bytes)
{
    if (!bytes) return {};
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    qreal value = static_cast<qreal>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    const int decimals = unit == 0 ? 0 : (value < 10.0 ? 1 : 0);
    return QStringLiteral("%1 %2").arg(QLocale().toString(value, 'f', decimals), QLatin1String(units[unit]));
}

QString SearchResultModel::iconKey(const SearchItem& item) const
{
    if (item.kind == ItemKind::Folder || item.kind == ItemKind::Favorite) return QStringLiteral("<folder>");
    if (item.kind == ItemKind::Command) return QStringLiteral("<command>");
    if (item.kind == ItemKind::Web) return QStringLiteral("<web>");
    if (item.kind == ItemKind::Application) return item.path.toCaseFolded();
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

    if (role == ItemRole) return QVariant::fromValue(item);
    if (role == SubtitleRole) return item.subtitle;
    if (role == KindRole) return static_cast<int>(item.kind);
    if (role == PathRole) return item.path;
    if (role == SizeRole) return QVariant::fromValue(item.size);
    if (role == ModifiedRole) return item.modified;
    if (role == ScoreRole) return item.score;
    if (role == ProviderRole) return item.provider;

    if (role == Qt::DecorationRole && index.column() == NameColumn) return iconFor(item);
    if (role == Qt::TextAlignmentRole && index.column() == SizeColumn)
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::ToolTipRole) {
        if (!item.path.isEmpty()) return item.path;
        return item.subtitle;
    }
    if (role != Qt::DisplayRole) return {};

    switch (index.column()) {
    case NameColumn: return item.title;
    case PathColumn: return item.path.isEmpty() ? item.subtitle : item.path;
    case TypeColumn: return kindLabel(item.kind);
    case SizeColumn:
        return (item.kind == ItemKind::File) ? formatSize(item.size) : QString{};
    case ModifiedColumn:
        return item.modified.isValid() ? QLocale().toString(item.modified, QLocale::ShortFormat) : QString{};
    default: return {};
    }
}

QVariant SearchResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case NameColumn: return QStringLiteral("Name");
    case PathColumn: return QStringLiteral("Path");
    case TypeColumn: return QStringLiteral("Type");
    case SizeColumn: return QStringLiteral("Size");
    case ModifiedColumn: return QStringLiteral("Modified");
    default: return {};
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    return {{Qt::DisplayRole, "title"},
            {SubtitleRole, "subtitle"},
            {KindRole, "kind"},
            {PathRole, "path"},
            {SizeRole, "size"},
            {ModifiedRole, "modified"},
            {ScoreRole, "score"},
            {ProviderRole, "provider"}};
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
