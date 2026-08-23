#include "QueryParser.h"
#include "ConfigStore.h"

#include <QRegularExpression>

namespace quickary {
namespace {

QString mapFilter(const QString& token)
{
    const QString folded = token.toCaseFolded();
    if (folded == QStringLiteral("folder:")) return QStringLiteral("folder:");
    if (folded == QStringLiteral("file:")) return QStringLiteral("file:");
    if (folded == QStringLiteral("doc:")) return QStringLiteral("ext:doc;docx;pdf;rtf;txt;md;xls;xlsx;ppt;pptx");
    if (folded == QStringLiteral("pic:")) return QStringLiteral("ext:jpg;jpeg;png;gif;bmp;webp;svg;heic;avif");
    if (folded == QStringLiteral("video:")) return QStringLiteral("ext:mp4;mkv;avi;mov;wmv;webm;m4v");
    if (folded == QStringLiteral("audio:")) return QStringLiteral("ext:mp3;flac;wav;aac;m4a;ogg;opus;ape");
    return ConfigStore::instance().filterExpression(token);
}

bool isEverythingPropertyTerm(const QString& folded)
{
    static const QStringList prefixes{
        QStringLiteral("ext:"),
        QStringLiteral("size:"),
        QStringLiteral("dm:"),
        QStringLiteral("dc:"),
        QStringLiteral("path:"),
        QStringLiteral("parent:"),
        QStringLiteral("regex:"),
        QStringLiteral("case:"),
        QStringLiteral("wholeword:"),
        QStringLiteral("wholewords:"),
        QStringLiteral("content:"),
        QStringLiteral("regex:content:"),
        QStringLiteral("utf8content:"),
        QStringLiteral("utf16content:"),
    };
    for (const QString& prefix : prefixes) {
        if (folded.startsWith(prefix)) return true;
    }
    return false;
}

QString mapAdvancedAlias(const QString& token)
{
    QString value = token;
    const bool negative = value.startsWith(QLatin1Char('-')) && value.size() > 1;
    if (negative) value.remove(0, 1);

    const QString folded = value.toCaseFolded();
    if (folded.startsWith(QStringLiteral("date:"))) {
        value = QStringLiteral("dm:") + value.mid(5);
    } else if (folded.startsWith(QStringLiteral("text:"))) {
        // Full-text search is explicit so ordinary launcher searches never read file contents.
        value = QStringLiteral("content:") + value.mid(5);
    }

    return negative ? QStringLiteral("!") + value : value;
}

} // namespace

ParsedQuery QueryParser::parse(const QString& input)
{
    ParsedQuery out;
    out.original = input.trimmed();

    // Keep quoted values attached to their Everything function/modifier prefix, e.g.
    // content:"hello world" and regex:content:"^hello.*world$".
    static const QRegularExpression tokenRe(QStringLiteral(R"(((?:\S+:)?"[^"]+")|(\S+))"));
    auto it = tokenRe.globalMatch(out.original);
    QStringList everything;
    QStringList plain;

    while (it.hasNext()) {
        const auto m = it.next();
        QString token = m.captured(0).trimmed();
        if (token.isEmpty()) continue;

        const QString mapped = mapFilter(token);
        if (!mapped.isEmpty()) {
            everything << mapped;
            continue;
        }

        const QString advanced = mapAdvancedAlias(token);
        QString advancedFolded = advanced.toCaseFolded();
        if (advancedFolded.startsWith(QLatin1Char('!'))) advancedFolded.remove(0, 1);
        if (isEverythingPropertyTerm(advancedFolded)) {
            if (token.startsWith(QLatin1Char('-'))) out.negativeTerms << token.mid(1);
            everything << advanced;
            continue;
        }

        if (token.startsWith(QLatin1Char('-')) && token.size() > 1) {
            const QString term = token.mid(1);
            out.negativeTerms << term;
            everything << QStringLiteral("!") + term;
            continue;
        }

        // Listary-style path narrowing: a token containing '\\' is treated as path context.
        if (token.contains(QLatin1Char('\\'))) {
            everything << QStringLiteral("path:") + token;
            out.positiveTerms << token;
            plain << token;
            continue;
        }

        out.positiveTerms << token;
        plain << token;
        everything << token;
    }

    out.plainText = plain.join(QLatin1Char(' '));
    out.everythingQuery = everything.join(QLatin1Char(' '));
    return out;
}

} // namespace quickary
