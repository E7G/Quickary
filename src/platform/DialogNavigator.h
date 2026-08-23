#pragma once

#include <QString>

namespace quickary {

class DialogNavigator final {
public:
    static QString activeExplorerFolder();
    static bool jumpActiveFileDialogTo(const QString& folder);
};

} // namespace quickary
