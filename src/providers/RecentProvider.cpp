#include "RecentProvider.h"

#include <QFileInfo>
#include <QSettings>

namespace quickary {

void RecentProvider::search(const SearchRequest& request)
{
    SearchBatch batch{id(), request.serial, {}};
    if (!request.rawQuery.trimmed().isEmpty()) {
        emit resultsReady(batch);
        return;
    }

    const QStringList recent = QSettings().value(QStringLiteral("usage/recent")).toStringList();
    const int cap = qMin(request.deepSearch ? 40 : 12, recent.size());
    batch.items.reserve(cap);
    for (int i = 0; i < cap; ++i) {
        QFileInfo fi(recent.at(i));
        if (!fi.exists()) continue;

        SearchItem item;
        const QString suffix = fi.suffix().toCaseFolded();
        if (fi.isDir()) item.kind = ItemKind::Folder;
        else if (suffix == QStringLiteral("lnk") || suffix == QStringLiteral("exe") || suffix == QStringLiteral("url")) item.kind = ItemKind::Application;
        else item.kind = ItemKind::File;
        item.title = fi.fileName().isEmpty() ? fi.absoluteFilePath() : fi.fileName();
        item.subtitle = QStringLiteral("Recent · ") + fi.absolutePath();
        item.path = fi.absoluteFilePath();
        item.provider = id();
        item.score = 1500.0 - (i * 24.0);
        batch.items.push_back(std::move(item));
    }
    emit resultsReady(batch);
}

} // namespace quickary
