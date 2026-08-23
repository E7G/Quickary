# Quickary Architecture

## Hot path

`global hotkey / Explorer typing -> LauncherWindow -> 18 ms debounce -> SearchCoordinator -> providers -> Ranker -> SearchResultModel`

The UI thread never walks the filesystem. The Everything provider performs IPC on one QtConcurrent worker, and keystrokes received while it is busy are coalesced so only the newest pending query runs next.

## Providers

- `EverythingProvider`: local file/folder results through the official Everything SDK DLL.
- `AppProvider`: bounded asynchronous Start Menu / WindowsApps index.
- `RecentProvider`: empty-query recommendations from local activation history.
- `FavoritesProvider`: pinned paths.
- `CommandProvider`: built-in and JSON-defined commands.
- `WebProvider`: explicit keyword-triggered web searches; no background network suggestions.

Providers return the same `SearchItem` shape. `SearchCoordinator` deduplicates by stable identity and merges provider scores with local frequency/recency learning.

## Windows integration

- `HotkeyManager`: `RegisterHotKey` plus one event-driven low-level keyboard hook for double-Ctrl and optional Explorer type-to-search. There is no timer polling loop.
- `DialogNavigator`: foreground Explorer path via Shell COM; standard Open/Save dialog navigation via the location bar, with a conservative legacy fallback.
- `ShellActions`: open/reveal, clipboard copy/cut, Recycle Bin, UAC-capable custom actions and built-in commands.

## Resource strategy

- Idle filesystem work: none.
- Everything worker concurrency: one.
- Normal Launcher result cap: 64.
- Deep Search result cap: 300.
- Icon cache: 96 entries.
- Text preview cap: 256 KiB; preview is disabled until requested.
- Pinyin: optional and cached, so systems that do not need it do not carry the dependency.
- Remote suggestions/telemetry: none.

## Extension boundaries

Native Shell preview, a persistent cloud/NAS index, Gety-like full-text search, HTTP API and a full settings UI belong in separate adapters. They should not be allowed to add work to the normal launcher hot path when disabled.
