# Customization

Quickary keeps customization out of the hot search path. The configuration is parsed once and only re-read when you choose **Tray → Reload customization**.

Open the active file with **Tray → Open customization file**. The location is the Qt `AppConfigLocation` for Quickary.

A complete example is available in [`config.example.json`](config.example.json).

## Custom filters

```json
{
  "filters": [
    { "keyword": "code:", "expression": "ext:c;cpp;h;hpp;py;js;ts;rs;go" }
  ]
}
```

Typing `code: parser` expands the filter to the configured Everything expression.

## Custom commands

```json
{
  "commands": [
    {
      "keyword": "code",
      "title": "Open current folder in VS Code",
      "program": "code.cmd",
      "arguments": ["{current_folder}"],
      "admin": false
    }
  ]
}
```

Available placeholders in command arguments:

- `{current_folder}` — foreground Explorer folder captured when Quickary was summoned.
- `{query}` — text after the command keyword.

## Custom web search

```json
{
  "webSearch": [
    {
      "keyword": "gh",
      "name": "GitHub",
      "url": "https://github.com/search?q={query}",
      "requiresQuery": true
    }
  ]
}
```

`{query}` is URL-encoded before insertion.

## Custom actions

```json
{
  "actions": [
    {
      "name": "Open in VS Code",
      "program": "code.cmd",
      "arguments": ["{path}"],
      "extensions": ["*"],
      "admin": false
    }
  ]
}
```

Available placeholders:

- `{path}` — selected file/folder path.
- `{current_folder}` — captured Explorer folder.
- `{query}` — current Quickary query.

`extensions` accepts extension names without a dot, `"*"` for all items, or `"<folder>"` for folders. In Deep Search, a custom action is applied to each selected path.

## Design and security notes

- Nothing in this file runs in the background. Commands/actions only execute after explicit activation.
- `admin: true` uses Windows UAC (`runas`) and therefore prompts when elevation is required.
- Keep executable paths and templates under your control; this configuration is intentionally powerful.
