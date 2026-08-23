#include "ConfigStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace quickary {
namespace {

QStringList stringList(const QJsonValue& value)
{
    QStringList out;
    for (const auto& v : value.toArray()) {
        if (v.isString()) out << v.toString();
    }
    return out;
}

bool validDialogFocusMode(const QString& mode)
{
    return mode == QStringLiteral("alt-d")
        || mode == QStringLiteral("ctrl-l")
        || mode == QStringLiteral("legacy-edit");
}

} // namespace

ConfigStore& ConfigStore::instance()
{
    static ConfigStore store;
    return store;
}

ConfigStore::ConfigStore()
{
    ensureFile();
    reload();
}

QString ConfigStore::configPath() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath(QStringLiteral("quickary.json"));
}

bool ConfigStore::ensureFile()
{
    const QString path = configPath();
    if (QFile::exists(path)) return true;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QJsonObject root{
        {QStringLiteral("filters"), QJsonArray{}},
        {QStringLiteral("commands"), QJsonArray{}},
        {QStringLiteral("webSearch"), QJsonArray{}},
        {QStringLiteral("actions"), QJsonArray{}},
        {QStringLiteral("dialogAdapters"), QJsonArray{}},
    };
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ConfigStore::reload(QString* error)
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    QJsonParseError parse{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = parse.errorString();
        return false;
    }

    QVector<CustomFilter> filters;
    QVector<CustomCommand> commands;
    QVector<CustomWebEngine> web;
    QVector<CustomAction> actions;
    QVector<DialogAdapter> dialogAdapters;
    const QJsonObject root = document.object();

    for (const auto& value : root.value(QStringLiteral("filters")).toArray()) {
        const QJsonObject o = value.toObject();
        CustomFilter f{o.value(QStringLiteral("keyword")).toString().trimmed(), o.value(QStringLiteral("expression")).toString().trimmed()};
        if (!f.keyword.isEmpty() && !f.expression.isEmpty()) filters.push_back(std::move(f));
    }
    for (const auto& value : root.value(QStringLiteral("commands")).toArray()) {
        const QJsonObject o = value.toObject();
        CustomCommand c;
        c.keyword = o.value(QStringLiteral("keyword")).toString().trimmed();
        c.title = o.value(QStringLiteral("title")).toString().trimmed();
        c.description = o.value(QStringLiteral("description")).toString().trimmed();
        c.program = o.value(QStringLiteral("program")).toString().trimmed();
        c.arguments = stringList(o.value(QStringLiteral("arguments")));
        c.admin = o.value(QStringLiteral("admin")).toBool(false);
        if (!c.keyword.isEmpty() && !c.program.isEmpty()) commands.push_back(std::move(c));
    }
    for (const auto& value : root.value(QStringLiteral("webSearch")).toArray()) {
        const QJsonObject o = value.toObject();
        CustomWebEngine e;
        e.keyword = o.value(QStringLiteral("keyword")).toString().trimmed();
        e.name = o.value(QStringLiteral("name")).toString().trimmed();
        e.url = o.value(QStringLiteral("url")).toString().trimmed();
        e.requiresQuery = o.contains(QStringLiteral("requiresQuery")) ? o.value(QStringLiteral("requiresQuery")).toBool() : true;
        if (!e.keyword.isEmpty() && !e.url.isEmpty()) web.push_back(std::move(e));
    }
    for (const auto& value : root.value(QStringLiteral("actions")).toArray()) {
        const QJsonObject o = value.toObject();
        CustomAction a;
        a.name = o.value(QStringLiteral("name")).toString().trimmed();
        a.program = o.value(QStringLiteral("program")).toString().trimmed();
        a.arguments = stringList(o.value(QStringLiteral("arguments")));
        a.extensions = stringList(o.value(QStringLiteral("extensions")));
        a.admin = o.value(QStringLiteral("admin")).toBool(false);
        if (!a.name.isEmpty() && !a.program.isEmpty()) actions.push_back(std::move(a));
    }
    for (const auto& value : root.value(QStringLiteral("dialogAdapters")).toArray()) {
        const QJsonObject o = value.toObject();
        DialogAdapter adapter;
        adapter.process = o.value(QStringLiteral("process")).toString().trimmed();
        adapter.windowClass = o.value(QStringLiteral("windowClass")).toString().trimmed();
        adapter.focusMode = o.value(QStringLiteral("focus")).toString().trimmed().toCaseFolded();
        if (!adapter.process.isEmpty() && !adapter.windowClass.isEmpty() && validDialogFocusMode(adapter.focusMode))
            dialogAdapters.push_back(std::move(adapter));
    }

    filters_ = std::move(filters);
    commands_ = std::move(commands);
    webEngines_ = std::move(web);
    actions_ = std::move(actions);
    dialogAdapters_ = std::move(dialogAdapters);
    if (error) error->clear();
    return true;
}

QString ConfigStore::filterExpression(const QString& keyword) const
{
    const QString folded = keyword.toCaseFolded();
    for (const auto& filter : filters_) {
        if (filter.keyword.toCaseFolded() == folded) return filter.expression;
    }
    return {};
}

} // namespace quickary
