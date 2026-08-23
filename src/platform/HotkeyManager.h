#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace quickary {

class HotkeyManager final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
    void setExplorerTypeToSearch(bool enabled);
    bool explorerTypeToSearch() const { return explorerTypeToSearch_; }

signals:
    void activated();
    void deepSearchRequested();
    void explorerTextTyped(const QString& text);

private:
#ifdef Q_OS_WIN
    static LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam);
    static HotkeyManager* instance_;
    HHOOK hook_{};
    ULONGLONG lastCtrlUp_{0};
    bool ctrlDown_{false};
#endif
    bool explorerTypeToSearch_{true};
};

} // namespace quickary
