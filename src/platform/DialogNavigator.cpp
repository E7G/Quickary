#include "DialogNavigator.h"

#include <QDir>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <ole2.h>
#include <oaidl.h>
#include <exdisp.h>
#include <oleauto.h>
#include <vector>
#endif

namespace quickary {

#ifdef Q_OS_WIN
namespace {

bool isStandardFileDialog(HWND root)
{
    if (!root) return false;
    wchar_t cls[128]{};
    GetClassNameW(root, cls, 127);
    return wcscmp(cls, L"#32770") == 0;
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

bool navigateDialogWithKeyboard(const QString& folder)
{
    // Common Item Dialogs and modern #32770 dialogs support Alt+D to focus the location bar.
    // SendInput keeps us out of the target process and avoids injection/hooks in third-party apps.
    std::vector<INPUT> inputs;
    inputs.reserve(static_cast<size_t>(folder.size()) * 2 + 12);

    pushVirtualKey(inputs, VK_MENU);
    pushVirtualKey(inputs, 'D');
    pushVirtualKey(inputs, 'D', true);
    pushVirtualKey(inputs, VK_MENU, true);

    // Select any existing address text before typing the target folder.
    pushVirtualKey(inputs, VK_CONTROL);
    pushVirtualKey(inputs, 'A');
    pushVirtualKey(inputs, 'A', true);
    pushVirtualKey(inputs, VK_CONTROL, true);

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
    GetClassNameW(child, cls, 63);
    if (wcscmp(cls, L"Edit") == 0 && IsWindowVisible(child) && IsWindowEnabled(child)) {
        state->edit = child;
        return FALSE;
    }
    return TRUE;
}

} // namespace
#endif

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
    HWND root = GetForegroundWindow();
    if (!isStandardFileDialog(root) || !QDir(folder).exists()) return false;

    // Preferred path for Windows 10/11 Common Item Dialogs.
    if (navigateDialogWithKeyboard(folder)) return true;

    // Conservative fallback for older standard dialogs.
    LegacyEditState state;
    EnumChildWindows(root, findLegacyEdit, reinterpret_cast<LPARAM>(&state));
    if (!state.edit) return false;
    const std::wstring native = QDir::toNativeSeparators(folder).toStdWString();
    SendMessageW(state.edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(native.c_str()));
    SendMessageW(state.edit, WM_KEYDOWN, VK_RETURN, 0);
    SendMessageW(state.edit, WM_KEYUP, VK_RETURN, 0);
    return true;
#endif
}

} // namespace quickary
