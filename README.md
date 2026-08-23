# Quickary 1.2

Quickary is an independent Windows-first launcher and file-search application built with Qt 6 and C++20. It focuses on fast keyboard workflows, low idle resource use, and native Windows integration.

See [`FEATURES.md`](FEATURES.md) for the implementation matrix, [`CUSTOMIZATION.md`](CUSTOMIZATION.md) for custom filters/commands/actions/web engines/file-dialog adapters, and [`HTTP_API.md`](HTTP_API.md) for the optional localhost API.

## Included in 1.2

- Instant file/folder search through Everything SDK IPC.
- Compact Launcher and full Deep Search workspace with sortable Name/Path/Type/Size/Modified columns.
- Live type counts and combinable Files/Folders/Apps/Favorites/Commands/Web facets.
- Smart ranking, local frequency/recency learning, fuzzy matching and optional cached Chinese Pinyin/initials matching.
- Advanced Everything-oriented syntax including explicit `content:`/`text:` full-text search.
- Built-in image/text preview plus installed Windows Shell preview handlers when available.
- Windows context menu access, batch actions, favorites, custom actions, commands and web engines.
- Explorer folder awareness/type-to-search and global keyboard activation.
- **Reliable Quick Save/Open context capture:** Quickary records the foreground window before taking focus, then validates and restores that target when a folder result is activated.
- Standard Windows Save/Open dialog navigation automatically.
- **Explicit third-party file-dialog adapters** configured by exact executable + Win32 window class, with only three allowlisted navigation modes: `alt-d`, `ctrl-l`, `legacy-edit`.
- Network/NAS locations through Everything Folder Index without a second resident crawler.
- Graphical Settings for startup, Explorer typing, preview behavior, result limits, themes and the local API.
- Settings are transactional: Cancel discards unsaved UI changes, and API token rotation is persisted only by Apply/OK.
- Optional localhost HTTP search API: disabled by default, `127.0.0.1` only, bearer-token authentication, bounded queue and no command/action execution surface.
- Tray operation, no telemetry and no timer-based global polling while idle.

## Requirements

- Windows 10/11 x64
- Qt 6.11.x (CI uses 6.11.2)
- Current CMake with the `Visual Studio 18 2026` generator
- Visual Studio 2026 / MSVC x64
- Everything 1.4.1+ for indexed filename/path search
- An Everything version supporting `content:` if you want explicit full-text searches
- `Everything64.dll` from the official Everything SDK beside `Quickary.exe`

## Build

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64"
cmake --build build --config Release --parallel
```

Or use:

```powershell
./scripts/build-windows.ps1 -QtPrefix "C:/Qt/6.11.2/msvc2022_64"
```

The Windows build script builds Release, runs `windeployqt`, downloads the official Everything SDK and places its runtime DLL beside Quickary. GitHub Actions uses the current Windows/VS2026 image and official Qt 6.11.2 Windows archives.

## Keyboard workflow

| Key | Action |
|---|---|
| Double `Ctrl` / `Alt+Space` | Launcher |
| `Ctrl+Alt+Space` | Deep Search |
| `F2` | Toggle Launcher / Deep Search |
| `Enter` | Open / execute / navigate an active file dialog when a folder is selected |
| `Ctrl+Enter` | Reveal in Explorer |
| `Ctrl+N` / `Ctrl+P` | Move selection |
| `Alt+P` | Toggle preview |
| `Ctrl+O` or `Right` | Actions menu |
| `Ctrl+,` | Settings |
| `Esc` | Hide |

## Third-party file dialogs

Standard Windows dialogs require no configuration. Proprietary dialogs can be allowlisted in `quickary.json`:

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

Quickary does not accept arbitrary macro strings here. `focus` must be exactly `alt-d`, `ctrl-l`, or `legacy-edit`. Use the exact Win32 class whenever possible; `"*"` is supported only as an explicit opt-in for all top-level windows of the named process. See [`CUSTOMIZATION.md`](CUSTOMIZATION.md).

## Local HTTP API

Enable it under **Settings → API**. The default port is `32142`.

```powershell
$token = '<token from Quickary Settings>'
Invoke-RestMethod `
  -Uri 'http://127.0.0.1:32142/v1/search?q=report%20ext%3Apdf&limit=25' `
  -Headers @{ Authorization = "Bearer $token" }
```

`GET /health` is unauthenticated. `GET /v1/search` requires the token and exposes only Everything-backed file/folder search. See [`HTTP_API.md`](HTTP_API.md).

## Performance model

Everything queries run on background workers and all Everything provider instances share a process-wide SDK lock. UI input is debounced/coalesced, icons and optional Pinyin are cached, facets/sorting operate locally, preview is lazy, network indexing is delegated to Everything, and the HTTP listener/provider are absent while disabled. File-dialog adapters are purely activation-time integrations: they add no polling, hooks or background workers.
