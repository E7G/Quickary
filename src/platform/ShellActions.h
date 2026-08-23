#pragma once

#include "../core/SearchTypes.h"
#include <QObject>
#include <QStringList>

class QWidget;

namespace quickary {

class ShellActions final {
public:
    static bool openPath(const QString& path);
    static bool revealInFolder(const QString& path);
    static void copyPath(const QString& path);
    static void copyPaths(const QStringList& paths);
    static void copyFiles(const QStringList& paths, bool cut);
    static bool recycle(const QString& path);
    static bool executeCommand(const SearchItem& item, QWidget* parent = nullptr);
    static bool executeTemplate(const QString& program, const QStringList& argumentTemplates,
                                const QString& path, const QString& currentFolder, const QString& query,
                                bool admin = false);
};

} // namespace quickary
