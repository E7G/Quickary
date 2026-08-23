#pragma once

#include "ISearchProvider.h"

namespace quickary {

// Empty-query recommendations backed by Quickary's own activation history.
// This provider does no filesystem enumeration; it only validates a small bounded list of paths.
class RecentProvider final : public ISearchProvider {
    Q_OBJECT
public:
    explicit RecentProvider(QObject* parent = nullptr) : ISearchProvider(parent) {}
    QString id() const override { return QStringLiteral("recent"); }

public slots:
    void search(const SearchRequest& request) override;
};

} // namespace quickary
