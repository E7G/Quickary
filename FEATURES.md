# Quickary 1.0 feature matrix

This document tracks the **publicly documented Listary-style workflow surface** against Quickary.
Quickary is an independent implementation: it does not copy Listary code, assets, branding, or proprietary implementation details.

Legend: **Implemented** = usable in this repository; **Implemented (optional)** = available when its external Windows/Everything component is present; **Delegated** = intentionally supplied by the system/indexer instead of duplicated inside Quickary; **Planned** = non-core extension that is not presented as finished.

| Area | Status | Quickary implementation |
|---|---|---|
| Instant file/folder search | **Implemented** | Everything SDK IPC provider; no per-query disk crawl |
| Mixed app + file launcher | **Implemented** | Start Menu / WindowsApps provider merged with files, folders, favorites, commands and web actions |
| Launcher / Deep Search split | **Implemented** | compact low-latency Launcher plus full Deep Search table (`F2` / second summon) |
| Smart ranking / usage learning | **Implemented** | exact/prefix/fuzzy score plus local frequency + recency boost |
| Fuzzy matching | **Implemented** | substring, acronym and subsequence scoring |
| Chinese Pinyin / initials | **Implemented (optional)** | cached `cpp-pinyin` scoring when package + dictionary are available |
| Path narrowing with `\` | **Implemented** | translated into Everything path constraints |
| File type filters | **Implemented** | built-ins plus custom filter keywords mapped to Everything expressions through JSON |
| Advanced search syntax | **Implemented** | quoted phrases, exclusions, `ext:`, `size:`, `date:` alias, `dm:/dc:`, `path:`, `parent:`, `regex:` and explicit content search |
| Full-text content search | **Implemented (optional)** | `text:` aliases to Everything `content:`; `content:` / `regex:content:` pass through explicitly. Requires an Everything version that supports those functions; normal searches never read file contents |
| Preview | **Implemented** | fast image/text preview first, then installed Windows Shell `IPreviewHandler` for formats such as PDF/Office when a handler is registered |
| Actions | **Implemented** | open, reveal, copy path, copy, cut, recycle, favorite, custom executable actions, plus the native Windows Shell context menu for a selected item |
| Multi-select batch actions | **Implemented** | Deep Search row multi-selection; open/copy/cut/copy-path/recycle operate in batches |
| Commands | **Implemented** | cmd/cmda, psh/psha, mkdir, touch, shutdown, reboot, uninstall, connections, hosts |
| Custom commands | **Implemented** | local JSON definitions support executable, argument templates and optional UAC elevation; hot reload from tray |
| Web search | **Implemented** | Google, Wikipedia, Stack Overflow, Bing, Baidu, YouTube, Maps, Amazon, IMDb, Gmail |
| Custom web engines | **Implemented** | local JSON keyword/name/URL templates with encoded `{query}`; hot reload from tray |
| Menus / Favorites | **Implemented** | pin/unpin paths and merge favorites into ranking |
| Quick Save & Open / Quick Switch | **Implemented core** | standard Windows file dialogs use address-bar navigation with a legacy `#32770` fallback |
| Explorer type-to-search | **Implemented** | event-driven printable typing detection while Explorer is foreground; toggleable from Settings/tray |
| Current Explorer-folder awareness | **Implemented** | Shell COM (`IShellWindows`) resolves the foreground Explorer location without polling |
| Cloud / NAS / network search | **Implemented integration** | network shares and NAS folders added to Everything Folder Index are searchable through the same IPC path; Quickary deliberately does not run a second competing crawler |
| External storage | **Implemented integration** | searchable when the location is indexed by Everything |
| Live index updates | **Delegated** | Everything owns NTFS/folder-index updates and Quickary consumes the live index through IPC |
| Empty-query recommendations | **Implemented** | bounded local activation history is shown without an all-database query |
| Filter result counts / multi-filter UI | **Implemented** | Deep Search has live All/Files/Folders/Apps/Favorites/Commands/Web counts and combinable type facets |
| Sortable file columns | **Implemented** | Deep Search table exposes Name, Path, Type, Size and Modified columns with local sorting |
| Themes / theme presets | **Implemented** | Follow Windows, Dark and Light modes, applied live from Settings |
| Graphical settings | **Implemented** | startup, Explorer typing, preview behavior, native preview, close-after-activation, result limits and theme controls |
| Start with Windows | **Implemented** | per-user HKCU Run registration; no administrator permission required |
| Local HTTP search API | **Planned / non-core** | intentionally omitted from the default desktop build to avoid adding a localhost attack surface and idle service |
| Global hotkeys | **Implemented** | double Ctrl + Alt+Space launcher, Ctrl+Alt+Space Deep Search; event driven |
| Offline-first / no telemetry | **Implemented** | no telemetry; network access only occurs for an explicit web action or user-configured network index |

## Performance invariants

- No filesystem crawl on the UI thread.
- No timer-based global polling while idle.
- Only one Everything query worker runs at a time; rapid keystrokes are coalesced to the newest request.
- Launcher keeps a small configurable working result set; Deep Search expands it only on request.
- Type facets and column sorting operate on the already-returned result model; they do not launch another disk query.
- Icons and optional Pinyin conversions are cached.
- Preview is lazy. Image/text/native shell handlers are opened only when the preview pane is visible.
- Full-text `content:`/`text:` search is explicit and never runs for an ordinary filename query.
- Network/NAS crawling is not duplicated: Quickary consumes Everything Folder Index instead of maintaining a second background crawler.

## Quickary 1.0 desktop completion scope

The 1.0 desktop release treats the following as product-complete surfaces: Launcher, Deep Search, sorting/facets, actions, native context menu access, preview, settings/themes, startup behavior, Explorer integration, standard file-dialog navigation, Everything-backed local/network search, commands/web actions, favorites, recommendations and customization JSON.

Future work can still deepen individual integrations (for example third-party proprietary file-dialog adapters or an optional local API) without changing the 1.0 desktop architecture or requiring them to be advertised as completed today.

## Public reference surface reviewed

- https://www.listary.com/
- https://www.listary.com/feature/search-files
- https://www.listary.com/feature/minimal-launcher
- https://www.listary.com/feature/search-cloud-and-network
- https://www.listary.com/help-center
- https://www.listary.com/v7
- https://www.voidtools.com/support/everything/sdk/
- https://www.voidtools.com/support/everything/search_syntax/
