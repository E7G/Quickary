#pragma once

#include "ISearchProvider.h"
#include <QFutureWatcher>
#include <QLibrary>
#include <QMutex>
#include <optional>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace quickary {

class EverythingProvider final : public ISearchProvider {
    Q_OBJECT
public:
    explicit EverythingProvider(QObject* parent = nullptr);
    ~EverythingProvider() override;
    QString id() const override { return QStringLiteral("everything"); }
    bool isAvailable() const override { return available_; }

public slots:
    void search(const SearchRequest& request) override;

private:
    SearchBatch runQuery(const SearchRequest& request);
    void start(const SearchRequest& request);
    bool loadSdk();

#ifdef Q_OS_WIN
    using SetSearchWFn = void (WINAPI*)(LPCWSTR);
    using SetMaxFn = void (WINAPI*)(DWORD);
    using SetOffsetFn = void (WINAPI*)(DWORD);
    using SetRequestFlagsFn = void (WINAPI*)(DWORD);
    using QueryWFn = BOOL (WINAPI*)(BOOL);
    using GetNumResultsFn = DWORD (WINAPI*)();
    using GetFullPathWFn = DWORD (WINAPI*)(DWORD, LPWSTR, DWORD);
    using IsFolderFn = BOOL (WINAPI*)(DWORD);
    using GetSizeFn = BOOL (WINAPI*)(DWORD, LARGE_INTEGER*);
    using GetDateModifiedFn = BOOL (WINAPI*)(DWORD, FILETIME*);
    using GetLastErrorFn = DWORD (WINAPI*)();
    using ResetFn = void (WINAPI*)();

    SetSearchWFn setSearchW_{};
    SetMaxFn setMax_{};
    SetOffsetFn setOffset_{};
    SetRequestFlagsFn setRequestFlags_{};
    QueryWFn queryW_{};
    GetNumResultsFn getNumResults_{};
    GetFullPathWFn getFullPathW_{};
    IsFolderFn isFolder_{};
    GetSizeFn getSize_{};
    GetDateModifiedFn getDateModified_{};
    GetLastErrorFn getLastError_{};
    ResetFn reset_{};
#endif

    QLibrary sdk_;
    QMutex sdkMutex_;
    QFutureWatcher<SearchBatch> watcher_;
    std::optional<SearchRequest> pending_;
    bool busy_{false};
    bool available_{false};
};

} // namespace quickary
