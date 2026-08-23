# Feature parity matrix

This document tracks the **publicly documented Listary workflow surface** against Quickary.
The project is an independent implementation: it does not copy Listary code, assets, or branding.

Legend: **Implemented** = usable in this repository; **Partial** = core path exists but not full Listary parity; **Planned** = intentionally not presented as finished.

| Area | Status | Quickary implementation |
|---|---|---|
| Instant file/folder search | **Implemented** | Everything SDK IPC provider; no per-query disk scan |
| Mixed app + file launcher | **Implemented** | Start Menu app provider merged with file results |
| Launcher / File Search split | **Implemented** | compact Launcher and larger Deep Search (`F2` / second summon) |
| Smart ranking / usage learning | **Implemented** | exact/prefix/fuzzy score plus local frequency + recency boost |
| Fuzzy matching | **Implemented** | substring, acronym and subsequence scoring |
| Chinese Pinyin / initials | **Implemented (optional)** | cached `cpp-pinyin` scoring when package + dictionary are available |
| Path narrowing with `\` | **Implemented** | translated into Everything path constraints |
| File type filters | **Implemented** | built-ins plus custom filter keywords mapped to Everything expressions through JSON |
| Advanced search syntax | **Implemented core** | quoted phrases, exclusions, `ext:`, `size:`, `date:` alias plus Everything `dm:/dc:` |
| Preview | **Partial** | images + common text/source files; native Shell `IPreviewHandler` hosting remains |
| Actions | **Implemented core** | open, reveal, copy path, copy, cut, recycle, favorite, plus custom executable actions |
| Multi-select batch actions | **Implemented core** | Deep Search uses extended selection; open/copy/cut/copy-path/recycle can operate in batches |
| Commands | **Implemented core** | cmd/cmda, psh/psha, mkdir, touch, shutdown, reboot, uninstall, connections, hosts |
| Custom commands | **Implemented core** | local JSON definitions support executable, argument templates and optional UAC elevation; hot reload from tray |
| Web search | **Implemented core** | Google, Wikipedia, Stack Overflow, Bing, Baidu, YouTube, Maps, Amazon, IMDb, Gmail |
| Custom web engines | **Implemented core** | local JSON keyword/name/URL templates with encoded `{query}`; hot reload from tray |
| Menus / Favorites | **Implemented core** | pin/unpin paths and merge favorites into ranking |
| Quick Save & Open / Quick Switch | **Implemented core** | standard Windows file dialogs use event-injected address navigation with a legacy `#32770` fallback |
| Explorer type-to-search | **Implemented core** | low-level keyboard event hook detects printable typing only while Explorer is foreground; can be disabled from tray |
| Current Explorer-folder awareness | **Implemented** | Shell COM (`IShellWindows`) resolves the foreground Explorer location without polling |
| Cloud / NAS / network search | **Partial** | fastest current path is Everything Folder Index; native persistent provider remains |
| External storage | **Partial** | visible through Everything index when configured |
| Live index updates | **Delegated** | Everything owns NTFS/folder-index updates |
| Empty-query recommendations | **Implemented core** | recent activated files/folders are shown on empty input without scanning |
| Filter result counts / multi-filter UI | **Planned** | syntax filters exist, interactive facet panel remains |
| Sortable file columns | **Planned** | result model has size/modified metadata; table UI remains |
| Themes / theme presets | **Partial** | polished dark theme is included; settings/theme presets remain |
| Full-text fallback (Gety-like) | **Planned** | intentionally separate provider boundary |
| Local HTTP search API | **Planned** | can be added as an opt-in localhost-only adapter; disabled by default for attack-surface/power reasons |
| Global hotkeys | **Implemented** | double Ctrl + Alt+Space launcher, Ctrl+Alt+Space Deep Search; event driven |
| Offline-first / no telemetry | **Implemented** | no telemetry; web provider only acts on an explicit web command |

## Performance invariants

- No filesystem crawl on the UI thread.
- No timer-based global polling while idle.
- Only one Everything query worker runs at a time; rapid keystrokes are coalesced to the newest request.
- Launcher limits the working result set; Deep Search increases it only when requested.
- Icons and Pinyin conversions are cached.
- Preview is opt-in, so normal search does no content I/O.

## Public reference surface reviewed

- https://www.listary.com/
- https://www.listary.com/feature/search-files
- https://www.listary.com/feature/minimal-launcher
- https://www.listary.com/feature/search-cloud-and-network
- https://www.listary.com/help-center
- https://www.listary.com/v7
- https://www.voidtools.com/support/everything/sdk/
