#pragma once

#include "ISearchProvider.h"
#include <QStringList>

namespace quickary {

class FavoritesProvider final : public ISearchProvider {
    Q_OBJECT
public:
    using ISearchProvider::ISearchProvider;
    QString id() const override { return QStringLiteral("favorites"); }

    static QStringList favorites();
    static bool isFavorite(const QString& path);
    static void setFavorite(const QString& path, bool favorite);

public slots:
    void search(const SearchRequest& request) override;
};

} // namespace quickary
