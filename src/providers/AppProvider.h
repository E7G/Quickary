#pragma once

#include "ISearchProvider.h"
#include <QFutureWatcher>

namespace quickary {

class AppProvider final : public ISearchProvider {
    Q_OBJECT
public:
    explicit AppProvider(QObject* parent = nullptr);
    ~AppProvider() override;
    QString id() const override { return QStringLiteral("apps"); }

public slots:
    void search(const SearchRequest& request) override;

private:
    QVector<SearchItem> buildIndex() const;
    QVector<SearchItem> apps_;
    QFutureWatcher<QVector<SearchItem>> indexWatcher_;
};

} // namespace quickary
