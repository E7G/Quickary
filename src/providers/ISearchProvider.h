#pragma once

#include "../core/SearchTypes.h"
#include <QObject>

namespace quickary {

class ISearchProvider : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~ISearchProvider() override = default;

    virtual QString id() const = 0;
    virtual bool isAvailable() const { return true; }

public slots:
    virtual void search(const SearchRequest& request) = 0;

signals:
    void resultsReady(const SearchBatch& batch);
    void availabilityChanged(bool available, const QString& detail);
};

} // namespace quickary
