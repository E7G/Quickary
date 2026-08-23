# Quickary

A Windows-first, Qt Widgets + C++20 launcher/file-search application inspired by the *workflow* of Listary, implemented independently with a focus on low latency, low idle CPU use, and keyboard-first interaction.

> This project does **not** copy Listary source code, branding, icons, or proprietary assets. It recreates common productivity patterns with its own implementation and UI.

See [`FEATURES.md`](FEATURES.md) for an explicit implemented/partial/planned parity matrix, including Listary V7 additions.

## What works in this production-oriented v0.2 scaffold

- **Instant file/folder search via Everything SDK IPC** (Everything remains the indexer; Quickary does no full-disk scan on each query)
- **App launcher** from Start Menu / WindowsApps entries
- **Launcher + Deep Search modes** (`F2` toggles)
- **Smart ranking**: exact/prefix/subsequence + application/folder/favorite bias + frequency/recency learning
- **Empty-query recommendations** from a bounded local recent-activation history
- **Optional Chinese Pinyin / initials ranking** when `cpp-pinyin` is installed (conversion is cached; dictionary work stays out of the hot path after warm-up)
- **Listary-like filters**: `folder:`, `file:`, `doc:`, `pic:`, `video:`, `audio:`, plus `ext:`, `size:`, `date:` and Everything-style `dm:`, `dc:`
- **Path narrowing** using `\\` tokens
- **Favorites/Menu-like pinned paths**
- **Hot-reload customization** for filters, commands, actions, and web engines through a local JSON file (see `CUSTOMIZATION.md`)
- **Web search keywords**: `g`, `wiki`, `so`, `bing`, `b`, `youtube`, `maps`, `amazon`, `imdb`, `gmail`
- **Commands**: `cmd`, `cmda`, `psh`, `psha`, `mkdir`, `touch`, `shutdown`, `reboot`, `uninstall`, `connections`, `hosts`
- **Actions**: open, reveal, copy path, copy, cut, recycle, pin/unpin; Deep Search supports multi-select batch actions
- **Preview** (`Alt+P`) for images and common text/source files
- **Quick Save/Open** for standard Windows file dialogs: folder activation navigates the foreground dialog through its location bar, with a legacy `#32770` fallback
- **Explorer integration**: foreground folder resolution through Shell COM and event-driven type-to-search (toggleable from tray)
- **Global launcher shortcuts**: double `Ctrl` / `Alt+Space` for Launcher, `Ctrl+Alt+Space` for Deep Search
- **Tray mode** and no polling loops while idle

## Performance design

1. Everything queries run on one background worker and are **coalesced**: if the user types faster than a query completes, only the newest pending query is executed.
2. UI input has an 18 ms debounce; this is short enough to feel immediate but prevents redundant IPC churn.
3. Only the top 64 launcher results are kept normally; Deep Search expands the window and result cap.
4. Icons are lazily resolved and cached by file type; the worker threads never create GUI objects.
5. Application indexing is done once asynchronously at startup.
6. Ranking/usage history is small, local, and event-driven. There is no telemetry.
7. Preview is opt-in (`Alt+P`) so expensive image/text reads do not occur during normal search.

## Requirements

- Windows 10/11 x64
- Qt 6.11.2 recommended (CI uses the official 6.11.2 Windows/MSVC binaries)
- A current CMake with the `Visual Studio 18 2026` generator for the default build path
- Visual Studio 2026 / MSVC x64
- Everything 1.4.1+ running in the background
- `Everything64.dll` from the official Everything SDK placed beside `Quickary.exe` (for 64-bit builds)

Everything's SDK DLL is an IPC wrapper; the Everything application/service must be running.

## Build

Current 2026 toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64"
cmake --build build --config Release --parallel
```

For the convenient Windows path, run:

```powershell
./scripts/build-windows.ps1 -QtPrefix "C:/Qt/6.11.2/msvc2022_64"
```

The build script defaults to **Visual Studio 18 2026**, builds Release, runs `windeployqt`, downloads the **official** Everything SDK, and copies `Everything64.dll` beside the executable. Pass `-Generator "Visual Studio 17 2022"` or `-Generator Ninja` only if you intentionally want an older/different local toolchain.

GitHub Actions uses `windows-latest`, which currently resolves to the Windows Server 2025 image with Visual Studio 2026. Qt 6.11 changed the online repository layout while aqtinstall 3.3.0 still assumes the older layout, so CI downloads the official Qt 6.11.2 `qtbase`, `qttools`, and `qtsvg` archives directly instead of downgrading Qt.

For a manual build, copy `Everything64.dll` from the official SDK into the folder containing `Quickary.exe`, then run the standard (non-Lite) Everything client and Quickary.

Optional Pinyin support can be installed with `vcpkg install cpp-pinyin:x64-windows` and exposed to CMake via your vcpkg toolchain. Quickary auto-detects the vcpkg dictionary when possible; otherwise pass `-DQUICKARY_CPP_PINYIN_DICT_DIR=C:/path/to/cpp-pinyin/res/dict`. Quickary builds and runs without Pinyin support.

## Keyboard workflow

| Key | Action |
|---|---|
| Double `Ctrl` | Open launcher |
| `Alt+Space` | Open launcher fallback |
| `Ctrl+Alt+Space` | Open Deep Search globally |
| `F2` | Toggle Launcher / Deep Search |
| `Enter` | Open/execute |
| `Ctrl+Enter` | Reveal in Explorer |
| `Ctrl+N` / `Ctrl+P` | Move selection |
| `Alt+P` | Toggle preview |
| `Ctrl+O` or `Right` | Actions menu |
| `Esc` | Hide |

## Feature parity roadmap

The following are architecturally planned but intentionally **not faked** in v0.2:

- Precomputed Pinyin side-index for millions of arbitrary Everything results. v0.2 already supports cached Pinyin scoring when `cpp-pinyin` is installed; a background side-index is the next step for zero-conversion search at very large scale.
- Native Windows `IPreviewHandler` hosting for Office/PDF/other shell preview handlers
- Deeper third-party file-dialog adapters beyond standard Windows dialogs
- Explorer toolbar chrome/overlay beyond the implemented event-driven type-to-search integration
- Built-in persistent network/NAS/cloud folder index. Today, the fastest low-power path is to add those locations to Everything's Folder Index and search them through the same IPC provider.
- A full visual settings editor/import-export UI (v0.2 already supports hot-reload JSON customization)
- Full Windows context-menu hosting (`IContextMenu`) inside the actions panel
- Remote search suggestions (disabled by design in v0.2 to keep idle/network usage at zero)

These are separable providers/platform modules, so adding them does not require rewriting the launcher.
