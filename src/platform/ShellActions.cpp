#include "ShellActions.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace quickary {
namespace {

QString expandTemplate(QString value, const QString& path, const QString& currentFolder, const QString& query)
{
    value.replace(QStringLiteral("{path}"), path, Qt::CaseSensitive);
    value.replace(QStringLiteral("{current_folder}"), currentFolder, Qt::CaseSensitive);
    value.replace(QStringLiteral("{query}"), query, Qt::CaseSensitive);
    return value;
}

#ifdef Q_OS_WIN
QString quoteWindowsArgument(QString arg)
{
    // Sufficient for ShellExecute parameters: quote whitespace/quotes and escape embedded quotes.
    arg.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    if (arg.contains(QLatin1Char(' ')) || arg.contains(QLatin1Char('\t')) || arg.contains(QLatin1Char('"')))
        return QLatin1Char('"') + arg + QLatin1Char('"');
    return arg;
}
#endif

} // namespace


bool ShellActions::openPath(const QString& path)
{
#ifdef Q_OS_WIN
    const std::wstring p = QDir::toNativeSeparators(path).toStdWString();
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}

bool ShellActions::revealInFolder(const QString& path)
{
#ifdef Q_OS_WIN
    const QString args = QStringLiteral("/select,\"") + QDir::toNativeSeparators(path) + QStringLiteral("\"");
    return QProcess::startDetached(QStringLiteral("explorer.exe"), QStringList{args});
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
}

void ShellActions::copyPath(const QString& path)
{
    QApplication::clipboard()->setText(QDir::toNativeSeparators(path));
}

void ShellActions::copyPaths(const QStringList& paths)
{
    QStringList native;
    native.reserve(paths.size());
    for (const QString& path : paths) native << QDir::toNativeSeparators(path);
    QApplication::clipboard()->setText(native.join(QLatin1Char('\n')));
}

void ShellActions::copyFiles(const QStringList& paths, bool cut)
{
    auto* mime = new QMimeData;
    QList<QUrl> urls;
    for (const QString& path : paths) urls << QUrl::fromLocalFile(path);
    mime->setUrls(urls);
#ifdef Q_OS_WIN
    // Explorer recognizes Preferred DropEffect: DROPEFFECT_MOVE=2, DROPEFFECT_COPY=1.
    QByteArray effect(4, '\0');
    effect[0] = cut ? char(2) : char(1);
    mime->setData(QStringLiteral("Preferred DropEffect"), effect);
#endif
    QApplication::clipboard()->setMimeData(mime);
}

bool ShellActions::recycle(const QString& path)
{
#ifdef Q_OS_WIN
    QString native = QDir::toNativeSeparators(path);
    std::wstring from = native.toStdWString();
    from.push_back(L'\0'); // SHFileOperation expects double-NUL termination.
    SHFILEOPSTRUCTW op{};
    op.wFunc = FO_DELETE;
    op.pFrom = from.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    return SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
#else
    return QFile::moveToTrash(path);
#endif
}

bool ShellActions::executeTemplate(const QString& program, const QStringList& argumentTemplates,
                                   const QString& path, const QString& currentFolder, const QString& query,
                                   bool admin)
{
    if (program.trimmed().isEmpty()) return false;
    QStringList args;
    args.reserve(argumentTemplates.size());
    for (const QString& value : argumentTemplates)
        args << expandTemplate(value, path, currentFolder, query);

#ifdef Q_OS_WIN
    if (admin) {
        QStringList quoted;
        quoted.reserve(args.size());
        for (const QString& arg : args) quoted << quoteWindowsArgument(arg);
        const std::wstring exe = program.toStdWString();
        const std::wstring params = quoted.join(QLatin1Char(' ')).toStdWString();
        const std::wstring cwd = QDir::toNativeSeparators(currentFolder).toStdWString();
        return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", exe.c_str(),
                                                        params.empty() ? nullptr : params.c_str(),
                                                        cwd.empty() ? nullptr : cwd.c_str(),
                                                        SW_SHOWNORMAL)) > 32;
    }
#endif
    return QProcess::startDetached(program, args, currentFolder);
}

bool ShellActions::executeCommand(const SearchItem& item, QWidget* parent)
{
    const QString key = item.meta.value(QStringLiteral("keyword")).toString();
    const QString args = item.meta.value(QStringLiteral("args")).toString().trimmed();
    QString cwd = item.meta.value(QStringLiteral("currentFolder")).toString();
    if (cwd.isEmpty()) cwd = QDir::homePath();

    const QString program = item.meta.value(QStringLiteral("program")).toString();
    if (!program.isEmpty()) {
        return executeTemplate(program, item.meta.value(QStringLiteral("argumentTemplates")).toStringList(),
                               {}, cwd, args, item.meta.value(QStringLiteral("admin")).toBool());
    }

    if (key == QStringLiteral("mkdir")) return QDir(cwd).mkpath(args);
    if (key == QStringLiteral("touch")) {
        QFile f(QDir(cwd).filePath(args));
        return f.open(QIODevice::WriteOnly | QIODevice::Append);
    }
    if (key == QStringLiteral("shutdown") || key == QStringLiteral("reboot")) {
        const auto choice = QMessageBox::question(parent, QStringLiteral("Confirm system action"),
            key == QStringLiteral("shutdown") ? QStringLiteral("Shut down Windows now?") : QStringLiteral("Restart Windows now?"));
        if (choice != QMessageBox::Yes) return false;
        return QProcess::startDetached(QStringLiteral("shutdown.exe"),
            key == QStringLiteral("shutdown") ? QStringList{QStringLiteral("/s"), QStringLiteral("/t"), QStringLiteral("0")}
                                                : QStringList{QStringLiteral("/r"), QStringLiteral("/t"), QStringLiteral("0")});
    }

#ifdef Q_OS_WIN
    if (key == QStringLiteral("cmd") || key == QStringLiteral("cmda")) {
        if (key == QStringLiteral("cmda")) {
            const std::wstring params = (QStringLiteral("/K cd /d \"") + QDir::toNativeSeparators(cwd) + QStringLiteral("\"")).toStdWString();
            return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", L"cmd.exe", params.c_str(), nullptr, SW_SHOWNORMAL)) > 32;
        }
        return QProcess::startDetached(QStringLiteral("cmd.exe"), {QStringLiteral("/K"), QStringLiteral("cd"), QStringLiteral("/d"), cwd}, cwd);
    }
    if (key == QStringLiteral("psh") || key == QStringLiteral("psha")) {
        QString escapedCwd = cwd;
        escapedCwd.replace(QLatin1Char('\''), QStringLiteral("''"));
        const QString command = QStringLiteral("Set-Location -LiteralPath '%1'").arg(escapedCwd);
        if (key == QStringLiteral("psha")) {
            const std::wstring params = (QStringLiteral("-NoExit -Command \"") + command + QStringLiteral("\"")).toStdWString();
            return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", L"powershell.exe", params.c_str(), nullptr, SW_SHOWNORMAL)) > 32;
        }
        return QProcess::startDetached(QStringLiteral("powershell.exe"), {QStringLiteral("-NoExit"), QStringLiteral("-Command"), command}, cwd);
    }
    if (key == QStringLiteral("uninstall")) return QProcess::startDetached(QStringLiteral("control.exe"), {QStringLiteral("appwiz.cpl")});
    if (key == QStringLiteral("connections")) return QProcess::startDetached(QStringLiteral("control.exe"), {QStringLiteral("ncpa.cpl")});
    if (key == QStringLiteral("hosts")) {
        const QString hosts = qEnvironmentVariable("WINDIR", QStringLiteral("C:/Windows")) + QStringLiteral("/System32/drivers/etc/hosts");
        return QProcess::startDetached(QStringLiteral("notepad.exe"), {hosts});
    }
#endif
    return false;
}

} // namespace quickary
