#include "AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

namespace quickary {
namespace {

constexpr auto kTheme = "appearance/theme";
constexpr auto kLaunchAtStartup = "general/launchAtStartup";
constexpr auto kExplorerType = "general/explorerTypeToSearch";
constexpr auto kPreviewDefault = "general/previewByDefault";
constexpr auto kNativePreview = "general/nativePreview";
constexpr auto kCloseAfterActivation = "general/closeAfterActivation";
constexpr auto kLauncherLimit = "search/launcherLimit";
constexpr auto kDeepLimit = "search/deepLimit";

} // namespace

AppSettings& AppSettings::instance()
{
    static AppSettings value;
    return value;
}

AppSettings::AppSettings() = default;

ThemeMode AppSettings::themeMode() const
{
    const int value = settings_.value(QLatin1String(kTheme), static_cast<int>(ThemeMode::Dark)).toInt();
    if (value == static_cast<int>(ThemeMode::Light)) return ThemeMode::Light;
    if (value == static_cast<int>(ThemeMode::System)) return ThemeMode::System;
    return ThemeMode::Dark;
}

bool AppSettings::launchAtStartup() const
{
    return settings_.value(QLatin1String(kLaunchAtStartup), false).toBool();
}

bool AppSettings::explorerTypeToSearch() const
{
    return settings_.value(QLatin1String(kExplorerType), true).toBool();
}

bool AppSettings::previewByDefault() const
{
    return settings_.value(QLatin1String(kPreviewDefault), false).toBool();
}

bool AppSettings::nativePreviewEnabled() const
{
    return settings_.value(QLatin1String(kNativePreview), true).toBool();
}

bool AppSettings::closeAfterActivation() const
{
    return settings_.value(QLatin1String(kCloseAfterActivation), true).toBool();
}

int AppSettings::launcherResultLimit() const
{
    return qBound(16, settings_.value(QLatin1String(kLauncherLimit), 64).toInt(), 256);
}

int AppSettings::deepSearchResultLimit() const
{
    return qBound(100, settings_.value(QLatin1String(kDeepLimit), 500).toInt(), 2000);
}

void AppSettings::setValue(const QString& key, const QVariant& value)
{
    if (settings_.value(key) == value) return;
    settings_.setValue(key, value);
    emit changed();
}

void AppSettings::setThemeMode(ThemeMode value)
{
    setValue(QLatin1String(kTheme), static_cast<int>(value));
}

void AppSettings::setLaunchAtStartup(bool value)
{
    const bool didChange = launchAtStartup() != value;
    settings_.setValue(QLatin1String(kLaunchAtStartup), value);
    updateStartupRegistration();
    if (didChange) emit changed();
}

void AppSettings::setExplorerTypeToSearch(bool value)
{
    setValue(QLatin1String(kExplorerType), value);
}

void AppSettings::setPreviewByDefault(bool value)
{
    setValue(QLatin1String(kPreviewDefault), value);
}

void AppSettings::setNativePreviewEnabled(bool value)
{
    setValue(QLatin1String(kNativePreview), value);
}

void AppSettings::setCloseAfterActivation(bool value)
{
    setValue(QLatin1String(kCloseAfterActivation), value);
}

void AppSettings::setLauncherResultLimit(int value)
{
    setValue(QLatin1String(kLauncherLimit), qBound(16, value, 256));
}

void AppSettings::setDeepSearchResultLimit(int value)
{
    setValue(QLatin1String(kDeepLimit), qBound(100, value, 2000));
}

void AppSettings::sync()
{
    settings_.sync();
    updateStartupRegistration();
}

void AppSettings::updateStartupRegistration()
{
#ifdef Q_OS_WIN
    QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
    if (launchAtStartup()) {
        const QString executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        runKey.setValue(QStringLiteral("Quickary"), QStringLiteral("\"%1\"").arg(executable));
    } else {
        runKey.remove(QStringLiteral("Quickary"));
    }
    runKey.sync();
#endif
}

} // namespace quickary
