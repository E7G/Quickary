#include <QApplication>
#include <QDesktopServices>
#include <QMenu>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QUrl>

#include "core/AppSettings.h"
#include "core/ConfigStore.h"
#include "core/HttpApiServer.h"
#include "core/SearchCoordinator.h"
#include "platform/DialogNavigator.h"
#include "platform/HotkeyManager.h"
#include "providers/AppProvider.h"
#include "providers/CommandProvider.h"
#include "providers/EverythingProvider.h"
#include "providers/FavoritesProvider.h"
#include "providers/RecentProvider.h"
#include "providers/WebProvider.h"
#include "ui/LauncherWindow.h"
#include "ui/SettingsDialog.h"

using namespace quickary;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("QuickaryProject"));
    QCoreApplication::setApplicationName(QStringLiteral("Quickary"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.2.0"));
    QApplication::setQuitOnLastWindowClosed(false);

    auto& appSettings = AppSettings::instance();
    appSettings.sync();

    SearchCoordinator coordinator;
    coordinator.addProvider(new EverythingProvider);
    coordinator.addProvider(new AppProvider);
    coordinator.addProvider(new FavoritesProvider);
    coordinator.addProvider(new RecentProvider);
    coordinator.addProvider(new CommandProvider);
    coordinator.addProvider(new WebProvider);

    LauncherWindow window(&coordinator);
    SettingsDialog settingsDialog;
    HttpApiServer apiServer;

    HotkeyManager hotkeys;
    hotkeys.setExplorerTypeToSearch(appSettings.explorerTypeToSearch());
    QObject::connect(&hotkeys, &HotkeyManager::activated, &window, [&window] {
        DialogNavigator::captureForegroundTarget();
        window.summon(window.isVisible());
    });
    QObject::connect(&hotkeys, &HotkeyManager::deepSearchRequested, &window, [&window] {
        DialogNavigator::captureForegroundTarget();
        window.summon(true);
    });
    QObject::connect(&hotkeys, &HotkeyManager::explorerTextTyped, &window, [&window](const QString& text) { window.typeFromExplorer(text); });

    const auto showSettings = [&settingsDialog] {
        settingsDialog.show();
        settingsDialog.raise();
        settingsDialog.activateWindow();
    };
    QObject::connect(&window, &LauncherWindow::settingsRequested, &app, showSettings);

    QSystemTrayIcon tray(QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView));
    tray.setToolTip(QStringLiteral("Quickary"));
    QMenu trayMenu;
    trayMenu.addAction(QStringLiteral("Launcher"), &window, [&window] { window.summon(false); });
    trayMenu.addAction(QStringLiteral("Deep Search"), &window, [&window] { window.summon(true); });
    trayMenu.addAction(QStringLiteral("Settings…"), &app, showSettings);
    QAction* explorerTyping = trayMenu.addAction(QStringLiteral("Explorer type-to-search"));
    explorerTyping->setCheckable(true);
    explorerTyping->setChecked(appSettings.explorerTypeToSearch());
    QObject::connect(explorerTyping, &QAction::toggled, &app, [&hotkeys, &appSettings](bool enabled) {
        appSettings.setExplorerTypeToSearch(enabled);
        appSettings.sync();
        hotkeys.setExplorerTypeToSearch(enabled);
    });

    const auto applyApiSettings = [&apiServer, &appSettings] {
        if (!appSettings.httpApiEnabled()) {
            apiServer.stop();
            return;
        }
        apiServer.start(appSettings.httpApiPort(), appSettings.httpApiToken());
    };

    QObject::connect(&settingsDialog, &SettingsDialog::settingsApplied, &app, [&] {
        const bool enabled = appSettings.explorerTypeToSearch();
        explorerTyping->setChecked(enabled);
        hotkeys.setExplorerTypeToSearch(enabled);
        window.applySettings();
        applyApiSettings();
    });
    QObject::connect(&apiServer, &HttpApiServer::statusChanged, &app,
                     [&tray, &appSettings](bool listening, const QString& detail) {
        if (!listening && appSettings.httpApiEnabled())
            tray.showMessage(QStringLiteral("Quickary HTTP API"), detail, QSystemTrayIcon::Warning, 3500);
    });

    trayMenu.addSeparator();
    trayMenu.addAction(QStringLiteral("Open customization file"), &app, [] {
        auto& config = ConfigStore::instance();
        config.ensureFile();
        QDesktopServices::openUrl(QUrl::fromLocalFile(config.configPath()));
    });
    trayMenu.addAction(QStringLiteral("Reload customization"), &app, [&tray] {
        QString error;
        const bool ok = ConfigStore::instance().reload(&error);
        tray.showMessage(QStringLiteral("Quickary"), ok ? QStringLiteral("Customization reloaded")
                                                    : QStringLiteral("Could not reload customization: %1").arg(error),
                         ok ? QSystemTrayIcon::Information : QSystemTrayIcon::Warning, 2500);
    });
    trayMenu.addSeparator();
    trayMenu.addAction(QStringLiteral("Quit"), &app, &QCoreApplication::quit);
    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &window, [&window](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) window.summon(false);
    });
    tray.show();

    applyApiSettings();
    return app.exec();
}
