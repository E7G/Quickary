#pragma once

#include <QObject>
#include <QSettings>

namespace quickary {

enum class ThemeMode {
    System = 0,
    Dark = 1,
    Light = 2,
};

class AppSettings final : public QObject {
    Q_OBJECT
public:
    static AppSettings& instance();
    static QString generateHttpApiToken();

    ThemeMode themeMode() const;
    bool launchAtStartup() const;
    bool explorerTypeToSearch() const;
    bool previewByDefault() const;
    bool nativePreviewEnabled() const;
    bool closeAfterActivation() const;
    int launcherResultLimit() const;
    int deepSearchResultLimit() const;
    bool httpApiEnabled() const;
    quint16 httpApiPort() const;
    QString httpApiToken() const;

    void setThemeMode(ThemeMode value);
    void setLaunchAtStartup(bool value);
    void setExplorerTypeToSearch(bool value);
    void setPreviewByDefault(bool value);
    void setNativePreviewEnabled(bool value);
    void setCloseAfterActivation(bool value);
    void setLauncherResultLimit(int value);
    void setDeepSearchResultLimit(int value);
    void setHttpApiEnabled(bool value);
    void setHttpApiPort(quint16 value);
    void setHttpApiToken(const QString& value);

    void sync();

signals:
    void changed();

private:
    AppSettings();
    void setValue(const QString& key, const QVariant& value);
    void updateStartupRegistration();

    QSettings settings_;
};

} // namespace quickary
