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

} // namespace

ParsedQuery QueryParser::parse(const QString& input)
{
    ParsedQuery out;
    out.original = input.trimmed();

    // Preserve quoted phrases. Everything itself understands quoted strings.
    static const QRegularExpression tokenRe(QStringLiteral(R"(("[^"]+")|(\S+))"));
    auto it = tokenRe.globalMatch(out.original);
    QStringList everything;
    QStringList plain;

    while (it.hasNext()) {
        const auto m = it.next();
        QString token = m.captured(0).trimmed();
        if (token.isEmpty())
            continue;

        const QString mapped = mapFilter(token);
        if (!mapped.isEmpty()) {
            everything << mapped;
            continue;
        }

        const QString folded = token.toCaseFolded();
        if (folded.startsWith(QStringLiteral("date:"))) {
            // Listary V7-style alias. Everything uses dm: for date modified.
            everything << QStringLiteral("dm:") + token.mid(5);
            continue;
        }
        if (folded.startsWith(QStringLiteral("ext:")) || folded.startsWith(QStringLiteral("size:"))
            || folded.startsWith(QStringLiteral("dm:")) || folded.startsWith(QStringLiteral("dc:"))) {
            everything << token;
            continue;
        }

        if (token.startsWith(QLatin1Char('-')) && token.size() > 1) {
            const QString term = token.mid(1);
            out.negativeTerms << term;
            everything << QStringLiteral("!") + term;
            continue;
        }

        // Listary-style path narrowing: a token containing or ending in '\\' is treated as path context.
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
