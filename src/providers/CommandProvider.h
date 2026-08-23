#pragma once

#include "ISearchProvider.h"

namespace quickary {

class CommandProvider final : public ISearchProvider {
    Q_OBJECT
public:
    using ISearchProvider::ISearchProvider;
    QString id() const override { return QStringLiteral("commands"); }

public slots:
    void search(const SearchRequest& request) override;
};

} // namespace quickary
