#include "WebProvider.h"
#include "../core/ConfigStore.h"

#include <QUrl>
#include <QUrlQuery>

namespace quickary {
namespace {
struct Engine { const char* key; const char* name; const char* url; bool needsQuery; };
constexpr Engine engines[] = {
    {"g", "Google", "https://www.google.com/search?q=%1", true},
    {"wiki", "Wikipedia", "https://en.wikipedia.org/w/index.php?search=%1", true},
    {"so", "Stack Overflow", "https://stackoverflow.com/search?q=%1", true},
    {"bing", "Bing", "https://www.bing.com/search?q=%1", true},
    {"b", "Baidu", "https://www.baidu.com/s?wd=%1", true},
    {"youtube", "YouTube", "https://www.youtube.com/results?search_query=%1", true},
    {"maps", "Google Maps", "https://www.google.com/maps/search/%1", true},
    {"amazon", "Amazon", "https://www.amazon.com/s?k=%1", true},
    {"imdb", "IMDb", "https://www.imdb.com/find/?q=%1", true},
    {"gmail", "Gmail", "https://mail.google.com/", false},
};
}

void WebProvider::search(const SearchRequest& request)
{
    const QString input = request.rawQuery.trimmed();
    const QString key = input.section(QLatin1Char(' '), 0, 0).toCaseFolded();
    const QString terms = input.section(QLatin1Char(' '), 1);
    QVector<SearchItem> items;

    for (const auto& engine : engines) {
        const QString ekey = QString::fromLatin1(engine.key);
        if (key != ekey) continue;
        if (engine.needsQuery && terms.trimmed().isEmpty()) continue;

        SearchItem item;
        item.kind = ItemKind::Web;
        item.title = engine.needsQuery
            ? QStringLiteral("Search %1 for “%2”").arg(QString::fromLatin1(engine.name), terms)
            : QStringLiteral("Open %1").arg(QString::fromLatin1(engine.name));
        item.subtitle = QStringLiteral("Web Search · %1").arg(ekey);
        item.provider = id();
        const QByteArray encoded = QUrl::toPercentEncoding(terms);
        item.meta.insert(QStringLiteral("url"), QString::fromLatin1(engine.url).arg(QString::fromLatin1(encoded)));
        item.score = 1800.0;
        items.push_back(std::move(item));
        emit resultsReady(SearchBatch{id(), request.serial, std::move(items)});
        return;
    }

    for (const auto& engine : ConfigStore::instance().webEngines()) {
        if (key != engine.keyword.toCaseFolded()) continue;
        if (engine.requiresQuery && terms.trimmed().isEmpty()) continue;
        const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(terms));
        QString url = engine.url;
        url.replace(QStringLiteral("{query}"), encoded);

        SearchItem item;
        item.kind = ItemKind::Web;
        item.title = engine.requiresQuery
            ? QStringLiteral("Search %1 for “%2”").arg(engine.name.isEmpty() ? engine.keyword : engine.name, terms)
            : QStringLiteral("Open %1").arg(engine.name.isEmpty() ? engine.keyword : engine.name);
        item.subtitle = QStringLiteral("Custom Web Search · %1").arg(engine.keyword);
        item.provider = id();
        item.meta.insert(QStringLiteral("url"), url);
        item.score = 1850.0;
        items.push_back(std::move(item));
        break;
    }
    emit resultsReady(SearchBatch{id(), request.serial, std::move(items)});
}

} // namespace quickary
