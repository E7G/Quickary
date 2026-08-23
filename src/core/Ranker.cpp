#include "Ranker.h"

#include <algorithm>

#ifdef QUICKARY_HAVE_CPP_PINYIN
#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Pinyin.h>
#include <QCache>
#include <QCoreApplication>
#include <QMutex>
#include <memory>
#endif

namespace quickary {
namespace {

qreal subsequenceScore(const QString& needle, const QString& haystack)
{
    if (needle.isEmpty()) return 0.0;
    int j = 0;
    int gaps = 0;
    int last = -1;
    for (int i = 0; i < haystack.size() && j < needle.size(); ++i) {
        if (haystack.at(i) == needle.at(j)) {
            if (last >= 0) gaps += i - last - 1;
            last = i;
            ++j;
        }
    }
    if (j != needle.size()) return 0.0;
    return std::max<qreal>(50.0, 220.0 - gaps * 4.0 - (haystack.size() - needle.size()) * 0.5);
}

qreal basicTextScore(const QString& query, const QString& text)
{
    const QString q = query.trimmed().toCaseFolded();
    const QString t = text.toCaseFolded();
    if (q.isEmpty()) return 1.0;
    if (t == q) return 1200.0;
    if (t.startsWith(q)) return 900.0 - (t.size() - q.size()) * 0.4;

    const int pos = t.indexOf(q);
    if (pos >= 0) {
        const bool boundary = pos == 0 || !t.at(pos - 1).isLetterOrNumber();
        return (boundary ? 720.0 : 560.0) - pos * 2.0;
    }

    QString initials;
    bool atBoundary = true;
    for (const QChar ch : t) {
        if (ch.isLetterOrNumber()) {
            if (atBoundary) initials += ch;
            atBoundary = false;
        } else {
            atBoundary = true;
        }
    }
    if (!initials.isEmpty() && initials.startsWith(q)) return 520.0;
    return subsequenceScore(q, t);
}

#ifdef QUICKARY_HAVE_CPP_PINYIN
struct PinyinForms {
    QString full;
    QString initials;
};

class PinyinCache final {
public:
    PinyinCache()
    {
        Pinyin::setDictionaryPath((QCoreApplication::applicationDirPath() + QStringLiteral("/dict")).toStdString());
        converter_ = std::make_unique<Pinyin::Pinyin>();
    }

    PinyinForms forms(const QString& text)
    {
        QMutexLocker lock(&mutex_);
        if (auto* cached = cache_.object(text)) return *cached;
        PinyinForms f;
        if (converter_ && converter_->initialized()) {
            const auto result = converter_->hanziToPinyin(text.toUtf8().toStdString(), Pinyin::ManTone::NORMAL,
                                                          Pinyin::Default, false, false, false);
            QStringList syllables;
            for (const auto& part : result) {
                const QString py = QString::fromUtf8(part.pinyin);
                if (!py.isEmpty()) {
                    syllables << py;
                    f.initials += py.front();
                } else {
                    syllables << QString::fromUtf8(part.hanzi);
                }
            }
            f.full = syllables.join(QString());
        }
        cache_.insert(text, new PinyinForms(f));
        return f;
    }

private:
    QMutex mutex_;
    QCache<QString, PinyinForms> cache_{4096};
    std::unique_ptr<Pinyin::Pinyin> converter_;
};

PinyinCache& pinyinCache()
{
    static PinyinCache cache;
    return cache;
}
#endif

} // namespace

qreal Ranker::textScore(const QString& query, const QString& text)
{
    qreal score = basicTextScore(query, text);
#ifdef QUICKARY_HAVE_CPP_PINYIN
    const QString q = query.trimmed();
    const bool latinQuery = std::all_of(q.cbegin(), q.cend(), [](QChar c) { return c.unicode() < 128; });
    const bool hasCjk = std::any_of(text.cbegin(), text.cend(), [](QChar c) { return c.unicode() >= 0x3400 && c.unicode() <= 0x9fff; });
    if (latinQuery && hasCjk && score < 500.0) {
        const PinyinForms f = pinyinCache().forms(text);
        score = std::max(score, basicTextScore(q, f.full) - 25.0);
        score = std::max(score, basicTextScore(q, f.initials) - 15.0);
    }
#endif
    return score;
}

qreal Ranker::rank(const QString& query, const SearchItem& item, qreal usageBoost)
{
    qreal score = textScore(query, item.title);
    score = std::max(score, textScore(query, item.subtitle) * 0.55);

    switch (item.kind) {
    case ItemKind::Application: score += 60.0; break;
    case ItemKind::Favorite: score += 80.0; break;
    case ItemKind::Folder: score += 18.0; break;
    case ItemKind::Command: score += 40.0; break;
    case ItemKind::Web: score += 20.0; break;
    case ItemKind::File: break;
    }

    return score + usageBoost;
}

} // namespace quickary
