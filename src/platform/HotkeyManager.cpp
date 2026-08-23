#include "HotkeyManager.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QSettings>

namespace quickary {

#ifdef Q_OS_WIN
HotkeyManager* HotkeyManager::instance_ = nullptr;

namespace {

bool isExplorerWindow(HWND hwnd)
{
    if (!hwnd) return false;
    wchar_t cls[96]{};
    GetClassNameW(hwnd, cls, 95);
    return wcscmp(cls, L"CabinetWClass") == 0 || wcscmp(cls, L"ExploreWClass") == 0;
}

bool focusedControlLooksEditable()
{
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    if (!GetGUIThreadInfo(0, &info) || !info.hwndFocus) return false;
    wchar_t cls[128]{};
    GetClassNameW(info.hwndFocus, cls, 127);
    const QString name = QString::fromWCharArray(cls).toCaseFolded();
    return name.contains(QStringLiteral("edit")) || name.contains(QStringLiteral("richedit"));
}

QString translatedKey(const KBDLLHOOKSTRUCT* key)
{
    BYTE state[256]{};
    if (!GetKeyboardState(state)) return {};
    wchar_t buffer[8]{};
    const HWND foreground = GetForegroundWindow();
    const DWORD thread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const HKL layout = GetKeyboardLayout(thread);
    const int count = ToUnicodeEx(key->vkCode, key->scanCode, state, buffer, 8, 0, layout);
    if (count <= 0) return {};
    const QString text = QString::fromWCharArray(buffer, count);
    if (text.isEmpty() || text.front().isSpace() || !text.front().isPrint()) return {};
    return text;
}

} // namespace
#endif

HotkeyManager::HotkeyManager(QObject* parent) : QObject(parent)
{
    explorerTypeToSearch_ = QSettings().value(QStringLiteral("integration/explorerTypeToSearch"), true).toBool();
#ifdef Q_OS_WIN
    instance_ = this;
    QCoreApplication::instance()->installNativeEventFilter(this);
    RegisterHotKey(nullptr, 0x51A1, MOD_ALT | MOD_NOREPEAT, VK_SPACE); // Launcher.
    RegisterHotKey(nullptr, 0x51A2, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_SPACE); // Deep Search.
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, GetModuleHandleW(nullptr), 0);
#endif
}

HotkeyManager::~HotkeyManager()
{
#ifdef Q_OS_WIN
    if (hook_) UnhookWindowsHookEx(hook_);
    UnregisterHotKey(nullptr, 0x51A1);
    UnregisterHotKey(nullptr, 0x51A2);
    QCoreApplication::instance()->removeNativeEventFilter(this);
    if (instance_ == this) instance_ = nullptr;
#endif
}

void HotkeyManager::setExplorerTypeToSearch(bool enabled)
{
    explorerTypeToSearch_ = enabled;
    QSettings().setValue(QStringLiteral("integration/explorerTypeToSearch"), enabled);
}

bool HotkeyManager::nativeEventFilter(const QByteArray&, void* message, qintptr*)
{
#ifdef Q_OS_WIN
    const auto* msg = static_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY && msg->wParam == 0x51A1) {
        emit activated();
        return true;
    }
    if (msg->message == WM_HOTKEY && msg->wParam == 0x51A2) {
        emit deepSearchRequested();
        return true;
    }
#else
    Q_UNUSED(message);
#endif
    return false;
}

#ifdef Q_OS_WIN
LRESULT CALLBACK HotkeyManager::keyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0 && instance_) {
        const auto* k = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        const bool keyDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
        const bool keyUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
        const bool isCtrl = k->vkCode == VK_LCONTROL || k->vkCode == VK_RCONTROL || k->vkCode == VK_CONTROL;
        if (isCtrl) {
            if (keyDown) {
                instance_->ctrlDown_ = true;
            } else if (keyUp && instance_->ctrlDown_) {
                instance_->ctrlDown_ = false;
                const ULONGLONG now = GetTickCount64();
                if (now - instance_->lastCtrlUp_ <= 360) {
                    instance_->lastCtrlUp_ = 0;
                    QMetaObject::invokeMethod(instance_, [inst = instance_] { emit inst->activated(); }, Qt::QueuedConnection);
                } else {
                    instance_->lastCtrlUp_ = now;
                }
            }
        }

        if (keyDown && instance_->explorerTypeToSearch_ && isExplorerWindow(GetForegroundWindow())
            && !focusedControlLooksEditable()
            && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)
            && !(GetAsyncKeyState(VK_MENU) & 0x8000)
            && !(GetAsyncKeyState(VK_LWIN) & 0x8000)
            && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
            const QString text = translatedKey(k);
            if (!text.isEmpty()) {
                QMetaObject::invokeMethod(instance_, [inst = instance_, text] { emit inst->explorerTextTyped(text); }, Qt::QueuedConnection);
                return 1; // prevent Explorer's native type-to-select from competing with the launcher
            }
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
#endif

} // namespace quickary
