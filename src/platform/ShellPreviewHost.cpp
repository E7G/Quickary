#include "ShellPreviewHost.h"

#include <QFileInfo>
#include <QResizeEvent>

#include <iterator>

#ifdef Q_OS_WIN
#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>
#include <shobjidl.h>
#endif

namespace quickary {

struct ShellPreviewHost::Impl {
#ifdef Q_OS_WIN
    IPreviewHandler* handler{};
    bool comOwned{false};
#endif
};

#ifdef Q_OS_WIN
namespace {

constexpr wchar_t kPreviewHandlerIid[] = L"{8895b1c6-b41f-4c1c-a562-0d564250836f}";

bool previewHandlerClsid(const QString& path, CLSID* clsid)
{
    const QString suffix = QFileInfo(path).suffix();
    if (suffix.isEmpty()) return false;
    const std::wstring association = (QStringLiteral(".") + suffix).toStdWString();

    wchar_t clsidText[64]{};
    DWORD length = static_cast<DWORD>(std::size(clsidText));
    const HRESULT hr = AssocQueryStringW(ASSOCF_INIT_DEFAULTTOSTAR,
                                         ASSOCSTR_SHELLEXTENSION,
                                         association.c_str(),
                                         kPreviewHandlerIid,
                                         clsidText,
                                         &length);
    if (FAILED(hr) || !clsidText[0]) return false;
    return SUCCEEDED(CLSIDFromString(clsidText, clsid));
}

HRESULT initializePreviewHandler(IPreviewHandler* handler, const QString& path)
{
    const std::wstring native = path.toStdWString();

    IInitializeWithFile* withFile = nullptr;
    if (SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&withFile))) && withFile) {
        const HRESULT hr = withFile->Initialize(native.c_str(), STGM_READ | STGM_SHARE_DENY_NONE);
        withFile->Release();
        if (SUCCEEDED(hr)) return hr;
    }

    IInitializeWithStream* withStream = nullptr;
    if (SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&withStream))) && withStream) {
        IStream* stream = nullptr;
        HRESULT hr = SHCreateStreamOnFileEx(native.c_str(),
                                            STGM_READ | STGM_SHARE_DENY_NONE,
                                            FILE_ATTRIBUTE_NORMAL,
                                            FALSE,
                                            nullptr,
                                            &stream);
        if (SUCCEEDED(hr) && stream) {
            hr = withStream->Initialize(stream, STGM_READ);
            stream->Release();
        }
        withStream->Release();
        if (SUCCEEDED(hr)) return hr;
    }

    IInitializeWithItem* withItem = nullptr;
    if (SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&withItem))) && withItem) {
        IShellItem* item = nullptr;
        HRESULT hr = SHCreateItemFromParsingName(native.c_str(), nullptr, IID_PPV_ARGS(&item));
        if (SUCCEEDED(hr) && item) {
            hr = withItem->Initialize(item, STGM_READ);
            item->Release();
        }
        withItem->Release();
        if (SUCCEEDED(hr)) return hr;
    }

    return E_NOINTERFACE;
}

RECT clientRectFor(const QWidget* widget)
{
    return RECT{0, 0, widget->width(), widget->height()};
}

} // namespace
#endif

ShellPreviewHost::ShellPreviewHost(QWidget* parent)
    : QWidget(parent), impl_(std::make_unique<Impl>())
{
    setAttribute(Qt::WA_NativeWindow, true);
    setMinimumWidth(280);
#ifdef Q_OS_WIN
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    impl_->comOwned = SUCCEEDED(hr);
#endif
}

ShellPreviewHost::~ShellPreviewHost()
{
    clear();
#ifdef Q_OS_WIN
    if (impl_->comOwned) CoUninitialize();
#endif
}

bool ShellPreviewHost::preview(const QString& path)
{
    clear();
#ifndef Q_OS_WIN
    Q_UNUSED(path);
    return false;
#else
    if (!QFileInfo(path).isFile()) return false;

    CLSID clsid{};
    if (!previewHandlerClsid(path, &clsid)) return false;

    IPreviewHandler* handler = nullptr;
    HRESULT hr = CoCreateInstance(clsid, nullptr,
                                  CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                                  IID_PPV_ARGS(&handler));
    if (FAILED(hr) || !handler) return false;

    hr = initializePreviewHandler(handler, path);
    if (FAILED(hr)) {
        handler->Release();
        return false;
    }

    RECT rect = clientRectFor(this);
    hr = handler->SetWindow(reinterpret_cast<HWND>(winId()), &rect);
    if (SUCCEEDED(hr)) hr = handler->SetRect(&rect);
    if (SUCCEEDED(hr)) hr = handler->DoPreview();
    if (FAILED(hr)) {
        handler->Unload();
        handler->Release();
        return false;
    }

    impl_->handler = handler;
    return true;
#endif
}

void ShellPreviewHost::clear()
{
#ifdef Q_OS_WIN
    if (!impl_->handler) return;
    impl_->handler->Unload();
    impl_->handler->Release();
    impl_->handler = nullptr;
#endif
}

bool ShellPreviewHost::isPreviewing() const
{
#ifdef Q_OS_WIN
    return impl_->handler != nullptr;
#else
    return false;
#endif
}

void ShellPreviewHost::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
#ifdef Q_OS_WIN
    if (impl_->handler) {
        RECT rect = clientRectFor(this);
        impl_->handler->SetRect(&rect);
    }
#endif
}

} // namespace quickary
