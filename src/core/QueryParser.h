#pragma once

#include <QString>
#include <QStringList>

namespace quickary {

struct ParsedQuery {
    QString original;
    QString everythingQuery;
    QString plainText;
    QStringList positiveTerms;
    QStringList negativeTerms;
};

class QueryParser final {
public:
    static ParsedQuery parse(const QString& input);
};

} // namespace quickary
