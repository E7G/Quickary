#include "DialogNavigator.h"

#include "../core/ConfigStore.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <ole2.h>
#include <oaidl.h>
#include <exdisp.h>
#include <oleauto.h>
#include <iterator>
#include <vector>
#endif

namespace quickary {

#ifdef Q_OS_WIN
namespace {

HWND gCapturedForeground{};

DWORD windowProcessId(HWND window)
{
    DWORD pid = 0;
    if (window) GetWindowThreadProcessId(window, &pid);
    return pid;
}

QString windowClassName(HWND window)
{
    wchar_t buffer[256]{};
    const int length = window ? GetClassNameW(window, buffer, static_cast<int>(std::size(buffer))) : 0;
    return length > 0 ? QString::fromWCharArray(buffer, length) : QString{};
}

QString processImageName(HWND window)
{
    const DWORD pid = windowProcessId(window);
    if (!pid) return {};

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return {};

    std::wstring buffer(32768, L'\0');
    DWORD size = static_cast<DWORD>(buffer.size());
    QString result;
    if (QueryFullProcessImageNameW(process, 0, buffer.data(), &size) && size > 0)
        result = QFileInfo(QString::fromWCharArray(buffer.data(), static_cast<int>(size))).fileName();
    CloseHandle(process);
    return result;
}

bool isQuickaryWindow(HWND window)
{
    return window && windowProcessId(window) == GetCurrentProcessId();
}

bool isStandardFileDialog(HWND root)
{
    return windowClassName(root) == QStringLiteral("#32770");
}

void pushVirtualKey(std::vector<INPUT>& inputs, WORD vk, bool up = false)
{
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
    inputs.push_back(in);
}

void pushUnicode(std::vector<INPUT>& inputs, wchar_t ch)
{
    INPUT down{};
    down.type = INPUT_KEYBOARD;
    down.ki.wScan = ch;
    down.ki.dwFlags = KEYEVENTF_UNICODE;
    inputs.push_back(down);

    INPUT up = down;
    up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    inputs.push_back(up);
}

void pushChord(std::vector<INPUT>& inputs, WORD modifier, WORD key)
{
    pushVirtualKey(inputs, modifier);
    pushVirtualKey(inputs, key);
    pushVirtualKey(inputs, key, true);
    pushVirtualKey(inputs, modifier, true);
}

bool sendLocationWithKeyboard(HWND target, const QString& folder, const QString& focusMode)
{
    if (!target || !IsWindow(target)) return false;
    if (!SetForegroundWindow(target)) return false;

    std::vector<INPUT> inputs;
    inputs.reserve(static_cast<size_t>(folder.size()) * 2 + 12);

    if (focusMode == QStringLiteral("ctrl-l"))
        pushChord(inputs, VK_CONTROL, 'L');
    else
        pushChord(inputs, VK_MENU, 'D');

    pushChord(inputs, VK_CONTROL, 'A');

    const std::wstring native = QDir::toNativeSeparators(folder).toStdWString();
    for (const wchar_t ch : native) pushUnicode(inputs, ch);
    pushVirtualKey(inputs, VK_RETURN);
    pushVirtualKey(inputs, VK_RETURN, true);

    if (inputs.empty()) return false;
    const UINT expected = static_cast<UINT>(inputs.size());
    return SendInput(expected, inputs.data(), sizeof(INPUT)) == expected;
}

struct LegacyEditState { HWND edit{}; };

BOOL CALLBACK findLegacyEdit(HWND child, LPARAM lp)
{
    auto* state = reinterpret_cast<LegacyEditState*>(lp);
    wchar_t cls[64]{};
    GetClassNameW(child, cls, static_cast<int>(std::size(cls)));
    if (wcscmp(cls, L"Edit") == 0 && IsWindowVisible(child) && IsWindowEnabled(child)) {
        state->edit = child;
        return FALSE;
    }
    return TRUE;
}

bool setLegacyEdit(HWND root, const QString& folder)
{
    LegacyEditState state;
    EnumChildWindows(root, findLegacyEdit, reinterpret_cast<LPARAM>(&state));
    if (!state.edit) return false;

    const std::wstring native = QDir::toNativeSeparators(folder).toStdWString();
    SendMessageW(state.edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(native.c_str()));
    SendMessageW(state.edit, WM_KEYDOWN, VK_RETURN, 0);
    SendMessageW(state.edit, WM_KEYUP, VK_RETURN, 0);
    return true;
}

HWND navigationTarget()
{
    const HWND foreground = GetForegroundWindow();
    if (foreground && !isQuickaryWindow(foreground)) return foreground;
    if (gCapturedForeground && IsWindow(gCapturedForeground) && !isQuickaryWindow(gCapturedForeground))
        return gCapturedForeground;
    return nullptr;
}

const DialogAdapter* matchingAdapter(HWND target)
{
    const QString process = processImageName(target);
    const QString cls = windowClassName(target);
    if (process.isEmpty() || cls.isEmpty()) return nullptr;

    for (const auto& adapter : ConfigStore::instance().dialogAdapters()) {
        if (adapter.process.compare(process, Qt::CaseInsensitive) != 0) continue;
        if (adapter.windowClass != QStringLiteral("*")
            && adapter.windowClass.compare(cls, Qt::CaseInsensitive) != 0) continue;
        return &adapter;
    }
    return nullptr;
}

} // namespace
#endif

void DialogNavigator::captureForegroundTarget()
{
#ifdef Q_OS_WIN
    const HWND foreground = GetForegroundWindow();
    if (foreground && IsWindow(foreground) && !isQuickaryWindow(foreground))
        gCapturedForeground = foreground;
#endif
}

void DialogNavigator::discardForegroundTarget()
{
#ifdef Q_OS_WIN
    gCapturedForeground = nullptr;
#endif
}

QString DialogNavigator::activeExplorerFolder()
{
#ifndef Q_OS_WIN
    return QDir::homePath();
#else
    const HWND foreground = GetForegroundWindow();
    if (!foreground) return QDir::homePath();

    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitialize = SUCCEEDED(init);

    QString result;
    IShellWindows* windows = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&windows))) && windows) {
        long count = 0;
        windows->get_Count(&count);
        for (long i = 0; i < count && result.isEmpty(); ++i) {
            VARIANT index;
            VariantInit(&index);
            index.vt = VT_I4;
            index.lVal = i;

            IDispatch* dispatch = nullptr;
            if (SUCCEEDED(windows->Item(index, &dispatch)) && dispatch) {
                IWebBrowserApp* browser = nullptr;
                if (SUCCEEDED(dispatch->QueryInterface(IID_PPV_ARGS(&browser))) && browser) {
                    SHANDLE_PTR browserHwnd = 0;
                    if (SUCCEEDED(browser->get_HWND(&browserHwnd))
                        && reinterpret_cast<HWND>(browserHwnd) == foreground) {
                        BSTR location = nullptr;
                        if (SUCCEEDED(browser->get_LocationURL(&location)) && location) {
                            const QString url = QString::fromWCharArray(location, static_cast<int>(SysStringLen(location)));
                            SysFreeString(location);
                            const QString local = QUrl(url).toLocalFile();
                            if (!local.isEmpty() && QDir(local).exists()) result = QDir::cleanPath(local);
                        }
                    }
                    browser->Release();
                }
                dispatch->Release();
            }
            VariantClear(&index);
        }
        windows->Release();
    }

    if (shouldUninitialize) CoUninitialize();
    return result.isEmpty() ? QDir::homePath() : result;
#endif
}

bool DialogNavigator::jumpActiveFileDialogTo(const QString& folder)
{
#ifndef Q_OS_WIN
    Q_UNUSED(folder);
    return false;
#else
    if (!QDir(folder).exists()) return false;
    HWND target = navigationTarget();
    if (!target) return false;

    bool handled = false;
    if (isStandardFileDialog(target)) {
        handled = sendLocationWithKeyboard(target, folder, QStringLiteral("alt-d"));
        if (!handled) handled = setLegacyEdit(target, folder);
    } else if (const DialogAdapter* adapter = matchingAdapter(target)) {
        if (adapter->focusMode == QStringLiteral("legacy-edit"))
            handled = setLegacyEdit(target, folder);
        else
            handled = sendLocationWithKeyboard(target, folder, adapter->focusMode);
    }

    if (handled) gCapturedForeground = nullptr;
    return handled;
#endif
}

} // namespace quickary
