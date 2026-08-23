#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace quickary {

struct CustomFilter {
    QString keyword;
    QString expression;
};

struct CustomCommand {
    QString keyword;
    QString title;
    QString description;
    QString program;
    QStringList arguments;
    bool admin{false};
};

struct CustomWebEngine {
    QString keyword;
    QString name;
    QString url;
    bool requiresQuery{true};
};

struct CustomAction {
    QString name;
    QString program;
    QStringList arguments;
    QStringList extensions;
    bool admin{false};
};

struct DialogAdapter {
    QString process;
    QString windowClass;
    QString focusMode;
};

class ConfigStore final {
public:
    static ConfigStore& instance();

    QString configPath() const;
    bool ensureFile();
    bool reload(QString* error = nullptr);

    const QVector<CustomFilter>& filters() const { return filters_; }
    const QVector<CustomCommand>& commands() const { return commands_; }
    const QVector<CustomWebEngine>& webEngines() const { return webEngines_; }
    const QVector<CustomAction>& actions() const { return actions_; }
    const QVector<DialogAdapter>& dialogAdapters() const { return dialogAdapters_; }

    QString filterExpression(const QString& keyword) const;

private:
    ConfigStore();

    QVector<CustomFilter> filters_;
    QVector<CustomCommand> commands_;
    QVector<CustomWebEngine> webEngines_;
    QVector<CustomAction> actions_;
    QVector<DialogAdapter> dialogAdapters_;
};

} // namespace quickary
