#pragma once

#include "SearchTypes.h"

namespace quickary {

class Ranker final {
public:
    static qreal textScore(const QString& query, const QString& text);
    static qreal rank(const QString& query, const SearchItem& item, qreal usageBoost = 0.0);
};

} // namespace quickary
