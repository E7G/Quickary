#pragma once

#include "SearchTypes.h"
#include <QHash>
#include <QObject>
#include <QStringList>

namespace quickary {

class UsageStore final : public QObject {
    Q_OBJECT
public:
    explicit UsageStore(QObject* parent = nullptr);

    qreal boostFor(const SearchItem& item) const;
    void recordActivation(const SearchItem& item);
    QStringList recentPaths(int limit = 20) const;

private:
    struct Entry { int hits{0}; qint64 lastMs{0}; };
    QString keyFor(const SearchItem& item) const;
    void load();
    void saveEntry(const QString& key, const Entry& entry);

    QHash<QString, Entry> entries_;
    QStringList recent_;
};

} // namespace quickary
