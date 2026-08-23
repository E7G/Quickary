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

## Third-party file-dialog adapters

Windows standard Save/Open dialogs (`#32770`) work automatically. Some applications use proprietary file-dialog windows that do not expose the standard Windows dialog class. Quickary 1.2 supports **explicit allowlisted adapters** for those applications.

```json
{
  "dialogAdapters": [
    {
      "process": "ExampleApp.exe",
      "windowClass": "ExampleFileDialogClass",
      "focus": "ctrl-l"
    }
  ]
}
```

All three fields are required:

- `process` — exact executable file name, matched case-insensitively.
- `windowClass` — exact Win32 top-level window class. Use `"*"` only when you deliberately want every top-level window of the named process to be eligible.
- `focus` — one of the fixed modes below. Unknown values are rejected when the config is loaded.

Supported `focus` modes:

| Mode | Behavior |
|---|---|
| `alt-d` | Restore the captured target window, send `Alt+D`, select the address text, type the chosen folder, press Enter |
| `ctrl-l` | Restore the captured target window, send `Ctrl+L`, select the address text, type the chosen folder, press Enter |
| `legacy-edit` | For an explicitly matched window only, find its first visible/enabled Win32 `Edit` child, set the folder text, and press Enter |

Quickary captures the foreground window **before** the launcher takes focus. When you activate a folder result, it verifies that the captured HWND still exists, checks the process and window class against this allowlist, then runs the selected adapter. It does not inject code or install hooks inside the target application.

To identify a third-party dialog's process and Win32 class, use a trusted Windows inspection tool such as Visual Studio Spy++ or another window-inspection utility. Prefer the exact class over `"*"` whenever possible.

## Design and security notes

- Nothing in this file runs in the background. Commands/actions only execute after explicit activation.
- Third-party dialog navigation is disabled unless an adapter explicitly matches the target process and class; arbitrary keyboard macro strings are not supported.
- `admin: true` uses Windows UAC (`runas`) and therefore prompts when elevation is required.
- Keep executable paths and templates under your control; this configuration is intentionally powerful.
