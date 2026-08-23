# Quickary 1.1

Quickary is an independent Windows-first launcher and file-search application built with Qt 6 and C++20. It focuses on fast keyboard workflows, low idle resource use, and native Windows integration.

See [`FEATURES.md`](FEATURES.md) for the detailed implementation matrix, [`CUSTOMIZATION.md`](CUSTOMIZATION.md) for custom filters/commands/actions/web engines, and [`HTTP_API.md`](HTTP_API.md) for the optional localhost API.

## Included in 1.1

- Instant file/folder search through Everything SDK IPC.
- Compact Launcher and a full Deep Search workspace.
- Deep Search columns: Name, Path, Type, Size and Modified.
- Live type counts and combinable Files/Folders/Apps/Favorites/Commands/Web facets.
- Smart ranking plus local frequency/recency learning.
- Optional cached Chinese Pinyin/initials matching.
- Filters including `folder:`, `file:`, `doc:`, `pic:`, `video:`, `audio:`, `ext:`, `size:`, `date:`, `dm:` and `dc:`.
- Explicit advanced syntax for `path:`, `parent:`, `regex:`, `content:` and `regex:content:`. `text:` is a Quickary alias for `content:`.
- Built-in image/text preview plus installed Windows Shell preview handlers when available.
- Open, reveal, copy path, copy, cut, recycle, favorites, custom actions and access to the Windows context menu.
- Multi-select batch actions in Deep Search.
- Standard Windows file-dialog navigation and Explorer folder awareness/type-to-search.
- Network/NAS locations through Everything Folder Index without a second resident crawler.
- Graphical Settings for startup, Explorer typing, preview behavior, native preview, result limits, close behavior and System/Dark/Light themes.
- Per-user Start with Windows support without administrator rights.
- **Optional localhost HTTP search API**: disabled by default, `127.0.0.1` only, bearer-token authentication, bounded queue, 200-result cap and no command/action execution surface.
- Tray operation, no telemetry and no timer-based global polling while idle.

## Requirements

- Windows 10/11 x64
- Qt 6.11.x (CI uses 6.11.2)
- Current CMake with the `Visual Studio 18 2026` generator
- Visual Studio 2026 / MSVC x64
- Everything 1.4.1+ for normal indexed filename/path search
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

The Windows build script builds Release, runs `windeployqt`, downloads the official Everything SDK and places the runtime DLL beside Quickary. GitHub Actions uses `windows-latest` with the current VS2026 image and installs the official Qt 6.11.2 Windows archives directly.

## Keyboard workflow

| Key | Action |
|---|---|
| Double `Ctrl` / `Alt+Space` | Launcher |
| `Ctrl+Alt+Space` | Deep Search |
| `F2` | Toggle Launcher / Deep Search |
| `Enter` | Open / execute |
| `Ctrl+Enter` | Reveal in Explorer |
| `Ctrl+N` / `Ctrl+P` | Move selection |
| `Alt+P` | Toggle preview |
| `Ctrl+O` or `Right` | Actions menu |
| `Ctrl+,` | Settings |
| `Esc` | Hide |

## Search examples

```text
report pdf
folder: project
file: ext:pdf size:>10mb report
pic: date:week
-project temp
text:"invoice total" ext:pdf
regex:^IMG_\d{4}\.jpg$
```

Content search is explicit by design. Ordinary Launcher input remains on the fast indexed filename/path route and does not read file contents.

## Local HTTP API

Enable it under **Settings → API**. The default port is `32142`.

```powershell
$token = '<token from Quickary Settings>'
Invoke-RestMethod `
  -Uri 'http://127.0.0.1:32142/v1/search?q=report%20ext%3Apdf&limit=25' `
  -Headers @{ Authorization = "Bearer $token" }
```

`GET /health` is unauthenticated. `GET /v1/search` requires the token. The API exposes only the Everything-backed file/folder search path; it cannot execute Quickary commands, custom actions, shell commands, or web actions. See [`HTTP_API.md`](HTTP_API.md).

## Performance model

Everything queries run on background workers, UI keystrokes are coalesced, and all Everything provider instances share a process-wide SDK lock so the optional API cannot corrupt UI query state. UI input uses a short debounce, icons/Pinyin are cached, facets and sorting operate locally on returned results, preview is lazy, and Quickary does not run a competing network or filesystem crawler. The HTTP listener and its dedicated provider are not created while the API is disabled.
