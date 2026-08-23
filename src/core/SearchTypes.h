#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace quickary {

enum class ItemKind {
    File,
    Folder,
    Application,
    Command,
    Web,
    Favorite
};

struct SearchItem {
    ItemKind kind{ItemKind::File};
    QString title;
    QString subtitle;
    QString path;
    QString provider;
    QVariantMap meta;
    qreal score{0.0};
    quint64 size{0};
    QDateTime modified;
};

struct SearchRequest {
    QString rawQuery;
    QString currentFolder;
    int limit{64};
    bool deepSearch{false};
    quint64 serial{0};
};

struct SearchBatch {
    QString provider;
    quint64 serial{0};
    QVector<SearchItem> items;
};

inline QString stableKey(const SearchItem& item)
{
    if (!item.path.isEmpty())
        return QString::number(static_cast<int>(item.kind)) + QLatin1Char('|') + item.path.toCaseFolded();
    return QString::number(static_cast<int>(item.kind)) + QLatin1Char('|') + item.title.toCaseFolded()
        + QLatin1Char('|') + item.meta.value(QStringLiteral("url")).toString().toCaseFolded();
}

} // namespace quickary

Q_DECLARE_METATYPE(quickary::SearchItem)
Q_DECLARE_METATYPE(quickary::SearchBatch)
