#pragma once

#include "SearchTypes.h"
#include "UsageStore.h"
#include <QHash>
#include <QObject>

namespace quickary {

class ISearchProvider;

class SearchCoordinator final : public QObject {
    Q_OBJECT
public:
    explicit SearchCoordinator(QObject* parent = nullptr);

    void addProvider(ISearchProvider* provider);
    void search(QString query, QString currentFolder, bool deepSearch);
    void recordActivation(const SearchItem& item);

signals:
    void resultsChanged(const QVector<SearchItem>& items, quint64 serial);
    void providerStatus(const QString& provider, bool available, const QString& detail);

private slots:
    void onBatch(const SearchBatch& batch);

private:
    void emitMerged();

    QVector<ISearchProvider*> providers_;
    QHash<QString, QVector<SearchItem>> batches_;
    UsageStore usage_;
    QString query_;
    QString rankQuery_;
    quint64 serial_{0};
    int limit_{64};
};

} // namespace quickary
