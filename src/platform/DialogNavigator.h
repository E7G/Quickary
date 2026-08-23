#pragma once

#include <QString>

namespace quickary {

class DialogNavigator final {
public:
    // Capture the foreground window before Quickary takes focus. Windows owned by
    // the Quickary process itself are ignored, so repeated hotkey summons can keep
    // the original dialog context.
    static void captureForegroundTarget();
    static void discardForegroundTarget();

    static QString activeExplorerFolder();
    static bool jumpActiveFileDialogTo(const QString& folder);
};

} // namespace quickary
