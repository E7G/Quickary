#include "CommandProvider.h"
#include "../core/Ranker.h"
#include "../core/ConfigStore.h"

namespace quickary {
namespace {
struct CommandDef { const char* key; const char* title; const char* description; };
constexpr CommandDef commands[] = {
    {"cmd", "Command Prompt", "Open Command Prompt in current folder"},
    {"cmda", "Command Prompt (Admin)", "Open elevated Command Prompt"},
    {"psh", "PowerShell", "Open PowerShell in current folder"},
    {"psha", "PowerShell (Admin)", "Open elevated PowerShell"},
    {"mkdir", "Create folder", "Create a folder in the current folder"},
    {"touch", "Create file", "Create an empty file in the current folder"},
    {"shutdown", "Shut down", "Shut down Windows"},
    {"reboot", "Restart", "Restart Windows"},
    {"uninstall", "Uninstall a program", "Open Programs and Features"},
    {"connections", "Network Connections", "Open network adapters"},
    {"hosts", "Edit hosts", "Open the Windows hosts file"},
};
}

void CommandProvider::search(const SearchRequest& request)
{
    const QString text = request.rawQuery.trimmed();
    if (text.isEmpty()) {
        emit resultsReady(SearchBatch{id(), request.serial, {}});
        return;
    }
    const QString keyword = text.section(QLatin1Char(' '), 0, 0).toCaseFolded();
    const QString args = text.section(QLatin1Char(' '), 1);
    QVector<SearchItem> items;

    for (const auto& def : commands) {
        const QString key = QString::fromLatin1(def.key);
        const qreal score = Ranker::textScore(keyword, key);
        if (score <= 0.0) continue;
        SearchItem item;
        item.kind = ItemKind::Command;
        item.title = QStringLiteral("%1  ·  %2").arg(key, QString::fromUtf8(def.title));
        item.subtitle = QString::fromUtf8(def.description);
        item.provider = id();
        item.score = score + 200.0;
        item.meta.insert(QStringLiteral("keyword"), key);
        item.meta.insert(QStringLiteral("args"), args);
        item.meta.insert(QStringLiteral("currentFolder"), request.currentFolder);
        items.push_back(std::move(item));
    }

    for (const auto& def : ConfigStore::instance().commands()) {
        const qreal score = Ranker::textScore(keyword, def.keyword);
        if (score <= 0.0) continue;
        SearchItem item;
        item.kind = ItemKind::Command;
        item.title = QStringLiteral("%1  ·  %2").arg(def.keyword, def.title.isEmpty() ? def.program : def.title);
        item.subtitle = def.description.isEmpty() ? QStringLiteral("Custom command") : def.description;
        item.provider = id();
        item.score = score + 220.0;
        item.meta.insert(QStringLiteral("keyword"), def.keyword);
        item.meta.insert(QStringLiteral("args"), args);
        item.meta.insert(QStringLiteral("currentFolder"), request.currentFolder);
        item.meta.insert(QStringLiteral("program"), def.program);
        item.meta.insert(QStringLiteral("argumentTemplates"), def.arguments);
        item.meta.insert(QStringLiteral("admin"), def.admin);
        items.push_back(std::move(item));
    }
    emit resultsReady(SearchBatch{id(), request.serial, std::move(items)});
}

} // namespace quickary
