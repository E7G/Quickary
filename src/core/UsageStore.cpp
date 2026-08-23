#include "UsageStore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QSettings>
#include <cmath>

namespace quickary {

UsageStore::UsageStore(QObject* parent) : QObject(parent) { load(); }

QString UsageStore::keyFor(const SearchItem& item) const
{
    const QByteArray raw = stableKey(item).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex());
}

void UsageStore::load()
{
    QSettings s;
    recent_ = s.value(QStringLiteral("usage/recent")).toStringList();
    s.beginGroup(QStringLiteral("usage/items"));
    for (const QString& k : s.childGroups()) {
        s.beginGroup(k);
        Entry e;
        e.hits = s.value(QStringLiteral("hits"), 0).toInt();
        e.lastMs = s.value(QStringLiteral("last"), 0).toLongLong();
        entries_.insert(k, e);
        s.endGroup();
    }
    s.endGroup();
}

void UsageStore::saveEntry(const QString& key, const Entry& entry)
{
    QSettings s;
    s.beginGroup(QStringLiteral("usage/items/") + key);
    s.setValue(QStringLiteral("hits"), entry.hits);
    s.setValue(QStringLiteral("last"), entry.lastMs);
    s.endGroup();
}

qreal UsageStore::boostFor(const SearchItem& item) const
{
    const auto it = entries_.constFind(keyFor(item));
    if (it == entries_.cend()) return 0.0;
    const qreal frequency = std::log2(static_cast<qreal>(it->hits) + 1.0) * 42.0;
    const qreal ageHours = std::max<qreal>(0.0, (QDateTime::currentMSecsSinceEpoch() - it->lastMs) / 3600000.0);
    const qreal recency = 180.0 * std::exp(-ageHours / (24.0 * 10.0));
    return frequency + recency;
}

void UsageStore::recordActivation(const SearchItem& item)
{
    const QString key = keyFor(item);
    Entry e = entries_.value(key);
    e.hits += 1;
    e.lastMs = QDateTime::currentMSecsSinceEpoch();
    entries_.insert(key, e);
    saveEntry(key, e);

    if (!item.path.isEmpty()) {
        recent_.removeAll(item.path);
        recent_.prepend(item.path);
        while (recent_.size() > 50) recent_.removeLast();
        QSettings().setValue(QStringLiteral("usage/recent"), recent_);
    }
}

QStringList UsageStore::recentPaths(int limit) const
{
    return recent_.mid(0, limit);
}

} // namespace quickary
