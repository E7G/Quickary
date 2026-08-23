#include "SearchCoordinator.h"
#include "AppSettings.h"
#include "Ranker.h"
#include "QueryParser.h"
#include "../providers/ISearchProvider.h"

#include <QHash>
#include <algorithm>

namespace quickary {

SearchCoordinator::SearchCoordinator(QObject* parent) : QObject(parent), usage_(this) {}

void SearchCoordinator::addProvider(ISearchProvider* provider)
{
    provider->setParent(this);
    providers_.push_back(provider);
    connect(provider, &ISearchProvider::resultsReady, this, &SearchCoordinator::onBatch);
    connect(provider, &ISearchProvider::availabilityChanged, this,
            [this, provider](bool ok, const QString& detail) { emit providerStatus(provider->id(), ok, detail); });
}

void SearchCoordinator::search(QString query, QString currentFolder, bool deepSearch)
{
    query_ = std::move(query);
    rankQuery_ = QueryParser::parse(query_).plainText;
    ++serial_;
    const auto& settings = AppSettings::instance();
    limit_ = deepSearch ? settings.deepSearchResultLimit() : settings.launcherResultLimit();
    batches_.clear();

    SearchRequest request;
    request.rawQuery = query_;
    request.currentFolder = std::move(currentFolder);
    request.deepSearch = deepSearch;
    request.limit = limit_;
    request.serial = serial_;

    for (auto* provider : providers_) provider->search(request);
}

void SearchCoordinator::onBatch(const SearchBatch& batch)
{
    if (batch.serial != serial_) return;
    batches_.insert(batch.provider, batch.items);
    emitMerged();
}

void SearchCoordinator::emitMerged()
{
    QHash<QString, SearchItem> unique;
    for (auto it = batches_.cbegin(); it != batches_.cend(); ++it) {
        for (SearchItem item : it.value()) {
            item.score = qMax(item.score, Ranker::rank(rankQuery_, item, usage_.boostFor(item)));
            const QString key = stableKey(item);
            auto found = unique.find(key);
            if (found == unique.end() || found->score < item.score)
                unique.insert(key, std::move(item));
        }
    }

    QVector<SearchItem> merged;
    merged.reserve(unique.size());
    for (auto it = unique.begin(); it != unique.end(); ++it) merged.push_back(std::move(it.value()));
    std::sort(merged.begin(), merged.end(), [](const SearchItem& a, const SearchItem& b) {
        if (!qFuzzyCompare(a.score, b.score)) return a.score > b.score;
        return QString::localeAwareCompare(a.title, b.title) < 0;
    });
    if (merged.size() > limit_) merged.resize(limit_);
    emit resultsChanged(merged, serial_);
}

void SearchCoordinator::recordActivation(const SearchItem& item)
{
    usage_.recordActivation(item);
}

} // namespace quickary
