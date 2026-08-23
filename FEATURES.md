# Quickary 1.2 feature matrix

This document tracks the publicly documented Listary-style workflow surface against Quickary. Quickary is an independent implementation and does not copy Listary code, assets, branding, or proprietary implementation details.

Legend: **Implemented** = usable in this repository; **Implemented (optional)** = available when its external/system component or opt-in setting is enabled; **Delegated** = intentionally supplied by the system/indexer instead of duplicated inside Quickary.

| Area | Status | Quickary implementation |
|---|---|---|
| Instant file/folder search | **Implemented** | Everything SDK IPC provider; no per-query disk crawl |
| Mixed app + file launcher | **Implemented** | Start Menu / WindowsApps provider merged with files, folders, favorites, commands and web actions |
| Launcher / Deep Search split | **Implemented** | compact low-latency Launcher plus full Deep Search table |
| Smart ranking / usage learning | **Implemented** | exact/prefix/fuzzy score plus local frequency + recency boost |
| Fuzzy matching | **Implemented** | substring, acronym and subsequence scoring |
| Chinese Pinyin / initials | **Implemented (optional)** | cached `cpp-pinyin` scoring when package + dictionary are available |
| Path narrowing with `\` | **Implemented** | translated into Everything path constraints |
| File type filters | **Implemented** | built-ins plus custom filter keywords mapped to Everything expressions through JSON |
| Advanced search syntax | **Implemented** | quoted phrases, exclusions, `ext:`, `size:`, `date:` alias, `dm:/dc:`, `path:`, `parent:`, `regex:` and explicit content search |
| Full-text content search | **Implemented (optional)** | `text:` aliases to Everything `content:`; normal searches never read file contents |
| Preview | **Implemented** | fast image/text preview first, then installed Windows Shell `IPreviewHandler` when registered |
| Actions | **Implemented** | open, reveal, copy path, copy, cut, recycle, favorite, custom actions and native Windows context menu |
| Multi-select batch actions | **Implemented** | Deep Search row multi-selection with batch open/copy/cut/copy-path/recycle |
| Commands | **Implemented** | cmd/cmda, psh/psha, mkdir, touch, shutdown, reboot, uninstall, connections, hosts |
| Custom commands | **Implemented** | JSON executable/argument templates with optional UAC elevation |
| Web search | **Implemented** | built-in engines plus explicit web commands |
| Custom web engines | **Implemented** | local JSON keyword/name/URL templates |
| Menus / Favorites | **Implemented** | pin/unpin paths and merge favorites into ranking |
| Standard Quick Save/Open | **Implemented** | foreground dialog is captured before Quickary takes focus; standard `#32770` dialogs use address navigation with legacy Edit fallback |
| Third-party file-dialog adapters | **Implemented (optional)** | explicit process + Win32 class allowlist in JSON; only `alt-d`, `ctrl-l`, or `legacy-edit` modes are accepted; no arbitrary macro strings or target-process injection |
| Explorer type-to-search | **Implemented** | event-driven printable typing detection while Explorer is foreground |
| Current Explorer-folder awareness | **Implemented** | Shell COM resolves foreground Explorer location without polling |
| Cloud / NAS / network search | **Implemented integration** | locations indexed by Everything Folder Index are searched through the same IPC path |
| External storage | **Implemented integration** | searchable when indexed by Everything |
| Live index updates | **Delegated** | Everything owns NTFS/folder-index updates |
| Empty-query recommendations | **Implemented** | bounded local activation history without an all-database query |
| Filter result counts / multi-filter UI | **Implemented** | live type counts and combinable facets |
| Sortable file columns | **Implemented** | Name, Path, Type, Size and Modified local sorting |
| Themes / theme presets | **Implemented** | Follow Windows, Dark and Light |
| Graphical settings | **Implemented** | startup, Explorer typing, preview behavior, result limits, theme and API controls |
| Transactional settings | **Implemented** | reopening reloads saved values; Cancel discards unsaved UI state; generated API tokens persist only on Apply/OK |
| Start with Windows | **Implemented** | per-user HKCU Run registration |
| Local HTTP search API | **Implemented (optional)** | localhost-only, token-authenticated search API with bounded requests/results; see `HTTP_API.md` |
| Global hotkeys | **Implemented** | double Ctrl / Alt+Space Launcher and Ctrl+Alt+Space Deep Search |
| Offline-first / no telemetry | **Implemented** | no telemetry; no resident remote service |

## Performance invariants

- No filesystem crawl on the UI thread.
- No timer-based global polling while idle.
- Everything SDK queries are serialized process-wide so UI and optional API search cannot corrupt shared DLL query state.
- Rapid UI keystrokes are coalesced to the newest request.
- Launcher keeps a small configurable working result set; Deep Search expands only on request.
- Facets and column sorting operate on already-returned results.
- Icons and optional Pinyin conversions are cached.
- Preview is lazy.
- Full-text content search is explicit.
- Network/NAS crawling is not duplicated.
- HTTP API is absent while disabled.
- File-dialog adapter logic runs only when the user activates a folder result; it installs no global hook and starts no worker.

## Quickary 1.2 desktop completion scope

Quickary 1.2 includes Launcher, Deep Search, ranking, filtering, sorting, previews, Windows context actions, standard Save/Open navigation, explicit third-party dialog adapters, Explorer integration, settings/themes, startup, Everything-backed local/network search, commands/web actions, favorites, recommendations, customization JSON, content syntax and the optional authenticated localhost API.

Third-party dialog support is intentionally allowlist-driven because proprietary applications can implement arbitrary UI structures. Adding a new adapter requires identifying its executable and Win32 top-level class instead of guessing against every foreground window.

## Public reference surface reviewed

- https://www.listary.com/
- https://www.listary.com/feature/search-files
- https://www.listary.com/feature/minimal-launcher
- https://www.listary.com/feature/search-cloud-and-network
- https://www.listary.com/help-center
- https://www.listary.com/v7
- https://www.voidtools.com/support/everything/sdk/
- https://www.voidtools.com/support/everything/search_syntax/
