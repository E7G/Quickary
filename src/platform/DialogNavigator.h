#pragma once

#include <QString>

namespace quickary {

class DialogNavigator final {
public:
    // Capture the foreground window before Quickary takes focus. Windows owned by
    // the Quickary process itself are ignored, so repeated summons do not destroy
    // the original Explorer/file-dialog context.
    static void captureForegroundTarget();

    static QString activeExplorerFolder();
    static bool jumpActiveFileDialogTo(const QString& folder);
};

} // namespace quickary
