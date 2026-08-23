#include "AppProvider.h"

#include "../core/Ranker.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>
#include <QtConcurrent>
#include <algorithm>

namespace quickary {

AppProvider::AppProvider(QObject* parent) : ISearchProvider(parent)
{
    connect(&indexWatcher_, &QFutureWatcher<QVector<SearchItem>>::finished, this, [this] {
        apps_ = indexWatcher_.result();
        emit availabilityChanged(true, QStringLiteral("%1 applications indexed").arg(apps_.size()));
    });
    indexWatcher_.setFuture(QtConcurrent::run([this] { return buildIndex(); }));
}

AppProvider::~AppProvider()
{
    if (indexWatcher_.isRunning()) indexWatcher_.waitForFinished();
}

QVector<SearchItem> AppProvider::buildIndex() const
{
    QVector<SearchItem> out;
    QSet<QString> seen;
    QStringList roots = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
#ifdef Q_OS_WIN
    const QString local = qEnvironmentVariable("LOCALAPPDATA");
    if (!local.isEmpty()) roots << local + QStringLiteral("/Microsoft/WindowsApps");
#endif
    for (const QString& root : roots) {
        QDirIterator it(root, QStringList{QStringLiteral("*.lnk"), QStringLiteral("*.exe"), QStringLiteral("*.url")},
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            const QString folded = path.toCaseFolded();
            if (seen.contains(folded)) continue;
            seen.insert(folded);
            QFileInfo fi(path);
            SearchItem item;
            item.kind = ItemKind::Application;
            item.title = fi.completeBaseName();
            item.subtitle = fi.absolutePath();
            item.path = path;
            item.provider = id();
            out.push_back(std::move(item));
        }
    }
    return out;
}

void AppProvider::search(const SearchRequest& request)
{
    if (request.rawQuery.trimmed().isEmpty()) {
        emit resultsReady(SearchBatch{id(), request.serial, {}});
        return;
    }
    QVector<SearchItem> out;
    out.reserve(qMin(request.limit, static_cast<int>(apps_.size())));
    for (const SearchItem& src : apps_) {
        const qreal s = Ranker::textScore(request.rawQuery, src.title);
        if (s > 0.0) {
            SearchItem item = src;
            item.score = s;
            out.push_back(std::move(item));
        }
    }
    std::partial_sort(out.begin(), out.begin() + qMin(request.limit, static_cast<int>(out.size())), out.end(),
                      [](const SearchItem& a, const SearchItem& b) { return a.score > b.score; });
    if (out.size() > request.limit) out.resize(request.limit);
    emit resultsReady(SearchBatch{id(), request.serial, std::move(out)});
}

} // namespace quickary
