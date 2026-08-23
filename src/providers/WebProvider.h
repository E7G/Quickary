#pragma once

#include "ISearchProvider.h"

namespace quickary {

class WebProvider final : public ISearchProvider {
    Q_OBJECT
public:
    using ISearchProvider::ISearchProvider;
    QString id() const override { return QStringLiteral("web"); }

public slots:
    void search(const SearchRequest& request) override;
};

} // namespace quickary
