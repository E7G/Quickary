#include "EverythingProvider.h"

#include "../core/QueryParser.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QtConcurrent>
#include <string>

namespace quickary {

#ifdef Q_OS_WIN
namespace {
constexpr DWORD EVERYTHING_REQUEST_FULL_PATH_AND_FILE_NAME = 0x00000004;
constexpr DWORD EVERYTHING_REQUEST_SIZE = 0x00000010;
constexpr DWORD EVERYTHING_REQUEST_DATE_MODIFIED = 0x00000040;
}
#endif

EverythingProvider::EverythingProvider(QObject* parent) : ISearchProvider(parent)
{
    available_ = loadSdk();
    connect(&watcher_, &QFutureWatcher<SearchBatch>::finished, this, [this] {
        const SearchBatch batch = watcher_.result();
        busy_ = false;
        emit resultsReady(batch);
        if (pending_) {
            SearchRequest next = *pending_;
            pending_.reset();
            start(next);
        }
    });
}

EverythingProvider::~EverythingProvider()
{
    // QtConcurrent::run is not cancellable here; wait during orderly application shutdown
    // so the worker cannot observe a destroyed provider or unloaded Everything DLL.
    if (watcher_.isRunning()) watcher_.waitForFinished();
}

bool EverythingProvider::loadSdk()
{
#ifndef Q_OS_WIN
    emit availabilityChanged(false, QStringLiteral("Everything SDK is Windows-only"));
    return false;
#else
#if defined(_WIN64)
    const QString dllName = QStringLiteral("Everything64.dll");
#else
    const QString dllName = QStringLiteral("Everything32.dll");
#endif
    const QString local = QCoreApplication::applicationDirPath() + QLatin1Char('/') + dllName;
    if (QFileInfo::exists(local)) sdk_.setFileName(local);
    else sdk_.setFileName(dllName);

    if (!sdk_.load()) {
        emit availabilityChanged(false, QStringLiteral("%1 not found. Put the Everything SDK DLL beside Quickary.exe").arg(dllName));
        return false;
    }

#define RESOLVE(name, type) reinterpret_cast<type>(sdk_.resolve(name))
    setSearchW_ = RESOLVE("Everything_SetSearchW", SetSearchWFn);
    setMax_ = RESOLVE("Everything_SetMax", SetMaxFn);
    setOffset_ = RESOLVE("Everything_SetOffset", SetOffsetFn);
    setRequestFlags_ = RESOLVE("Everything_SetRequestFlags", SetRequestFlagsFn);
    queryW_ = RESOLVE("Everything_QueryW", QueryWFn);
    getNumResults_ = RESOLVE("Everything_GetNumResults", GetNumResultsFn);
    getFullPathW_ = RESOLVE("Everything_GetResultFullPathNameW", GetFullPathWFn);
    isFolder_ = RESOLVE("Everything_IsFolderResult", IsFolderFn);
    getSize_ = RESOLVE("Everything_GetResultSize", GetSizeFn);
    getDateModified_ = RESOLVE("Everything_GetResultDateModified", GetDateModifiedFn);
    getLastError_ = RESOLVE("Everything_GetLastError", GetLastErrorFn);
    reset_ = RESOLVE("Everything_Reset", ResetFn);
#undef RESOLVE

    const bool ok = setSearchW_ && setMax_ && setOffset_ && setRequestFlags_ && queryW_
        && getNumResults_ && getFullPathW_ && isFolder_ && getSize_ && getDateModified_ && reset_;
    emit availabilityChanged(ok, ok ? QStringLiteral("Everything SDK ready") : QStringLiteral("Everything SDK exports are incomplete"));
    return ok;
#endif
}

void EverythingProvider::search(const SearchRequest& request)
{
    // Empty input is recommendation mode. Avoid an all-database IPC query entirely.
    if (request.rawQuery.trimmed().isEmpty()) {
        emit resultsReady(SearchBatch{id(), request.serial, {}});
        return;
    }
    if (!available_) {
        emit resultsReady(SearchBatch{id(), request.serial, {}});
        return;
    }
    if (busy_) {
        pending_ = request; // Coalesce keystrokes: only the newest query matters.
        return;
    }
    start(request);
}

void EverythingProvider::start(const SearchRequest& request)
{
    busy_ = true;
    watcher_.setFuture(QtConcurrent::run([this, request] { return runQuery(request); }));
}

SearchBatch EverythingProvider::runQuery(const SearchRequest& request)
{
    SearchBatch batch{id(), request.serial, {}};
#ifndef Q_OS_WIN
    return batch;
#else
    QMutexLocker lock(&sdkMutex_);
    reset_();

    const ParsedQuery parsed = QueryParser::parse(request.rawQuery);
    const std::wstring query = parsed.everythingQuery.toStdWString();
    setSearchW_(query.c_str());
    setOffset_(0);
    setMax_(static_cast<DWORD>(request.deepSearch ? qMax(request.limit, 256) : request.limit));
    setRequestFlags_(EVERYTHING_REQUEST_FULL_PATH_AND_FILE_NAME | EVERYTHING_REQUEST_SIZE | EVERYTHING_REQUEST_DATE_MODIFIED);

    if (!queryW_(TRUE))
        return batch;

    const DWORD count = getNumResults_();
    batch.items.reserve(static_cast<int>(count));
    for (DWORD i = 0; i < count; ++i) {
        const DWORD needed = getFullPathW_(i, nullptr, 0);
        if (!needed) continue;
        std::wstring buffer(static_cast<size_t>(needed) + 1, L'\0');
        getFullPathW_(i, buffer.data(), static_cast<DWORD>(buffer.size()));
        const QString fullPath = QString::fromWCharArray(buffer.c_str());
        QFileInfo fi(fullPath);

        SearchItem item;
        item.kind = isFolder_(i) ? ItemKind::Folder : ItemKind::File;
        item.title = fi.fileName().isEmpty() ? fullPath : fi.fileName();
        item.subtitle = fi.absolutePath();
        item.path = fullPath;
        item.provider = id();

        LARGE_INTEGER size{};
        if (getSize_(i, &size)) item.size = static_cast<quint64>(size.QuadPart);
        FILETIME ft{};
        if (getDateModified_(i, &ft)) {
            ULARGE_INTEGER value{};
            value.LowPart = ft.dwLowDateTime;
            value.HighPart = ft.dwHighDateTime;
            if (value.QuadPart > 116444736000000000ULL) {
                const qint64 ms = static_cast<qint64>((value.QuadPart - 116444736000000000ULL) / 10000ULL);
                item.modified = QDateTime::fromMSecsSinceEpoch(ms, Qt::UTC).toLocalTime();
            }
        }
        batch.items.push_back(std::move(item));
    }
    return batch;
#endif
}

} // namespace quickary
