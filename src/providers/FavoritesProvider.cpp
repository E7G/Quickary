#include "FavoritesProvider.h"
#include "../core/Ranker.h"
#include <QFileInfo>
#include <QSettings>

namespace quickary {

QStringList FavoritesProvider::favorites()
{
    return QSettings().value(QStringLiteral("favorites/paths")).toStringList();
}

bool FavoritesProvider::isFavorite(const QString& path)
{
    const auto list = favorites();
    for (const QString& p : list)
        if (QString::compare(p, path, Qt::CaseInsensitive) == 0) return true;
    return false;
}

void FavoritesProvider::setFavorite(const QString& path, bool favorite)
{
    QStringList list = favorites();
    for (int i = list.size() - 1; i >= 0; --i)
        if (QString::compare(list.at(i), path, Qt::CaseInsensitive) == 0) list.removeAt(i);
    if (favorite && !path.isEmpty()) list.prepend(path);
    QSettings().setValue(QStringLiteral("favorites/paths"), list);
}

void FavoritesProvider::search(const SearchRequest& request)
{
    QVector<SearchItem> items;
    for (const QString& path : favorites()) {
        QFileInfo fi(path);
        if (!fi.exists()) continue;
        const qreal score = Ranker::textScore(request.rawQuery, fi.fileName());
        if (!request.rawQuery.trimmed().isEmpty() && score <= 0.0) continue;
        SearchItem item;
        item.kind = ItemKind::Favorite;
        item.title = fi.fileName().isEmpty() ? fi.absoluteFilePath() : fi.fileName();
        item.subtitle = fi.absolutePath();
        item.path = fi.absoluteFilePath();
        item.provider = id();
        item.score = score + 300.0;
        items.push_back(std::move(item));
    }
    emit resultsReady(SearchBatch{id(), request.serial, std::move(items)});
}

} // namespace quickary
