# Quickary Product Design Notes

## Product principles

Quickary is designed around **time-to-first-action**, not visual decoration. The interface should feel like a native Windows utility that disappears when the job is done.

1. **Keyboard first, mouse complete** — every primary action has a key path, but no feature requires memorizing shortcuts.
2. **One query, mixed results** — apps, files, folders, favorites, commands, and web intents compete in one ranked list. The user never has to select a search mode first.
3. **Progressive disclosure** — Launcher stays compact; Deep Search expands only when comparison, filtering, or preview is needed.
4. **Stable geometry** — no bouncing cards, animated reflow, or expensive blur. Result rows stay 58 px high and the search field never moves.
5. **Local by default** — no telemetry and no network suggestions in the hot path. Web access happens only after the user explicitly invokes a web keyword and activates it.
6. **Visible cause/effect** — filters and commands are text-readable (`doc:`, `cmd`, `g query`) rather than hidden behind nested menus.

## Interaction model

### Launcher

- 760 × 430 logical px target size.
- Search field is the dominant control and receives focus immediately.
- 64-result cap prevents the view/model from becoming a memory sink.
- Empty input shows bounded recent recommendations; first result is preselected so `hotkey → type → Enter` is the fastest path.
- Double `Ctrl` while already open expands into Deep Search without clearing the query.

### Deep Search

- 1060 × 690 logical px target size.
- Same query and result model; no context reset when switching modes.
- Up to 300 results for comparison with extended multi-selection and batch actions.
- Optional preview pane is off by default and toggled with `Alt+P`.

### Result rows

- 34 px icon, 2-line information hierarchy.
- Primary name uses semibold weight.
- Secondary path is de-emphasized and middle-elided to preserve both root and file context.
- Selection uses a single rounded rectangle, not per-subcontrol highlights.

### Action menu

`Ctrl+O`, Right Arrow, or context click opens actions. Common actions are kept above destructive actions; Recycle Bin is visually separated.

## Performance budget

- Idle CPU target: effectively 0% aside from Windows hook dispatch and tray messages.
- No polling timers while hidden. Explorer integration is hook/event driven.
- No filesystem crawling in the UI thread.
- Search debounce: 18 ms.
- Everything provider concurrency: 1 worker; pending queries are coalesced.
- Icon cache: 96 entries; mostly keyed by extension.
- Preview file read cap: 256 KiB for text; image decoding only when preview is enabled.
- App index: one asynchronous startup scan.

## Visual language

The current scaffold deliberately avoids copying Listary's branded visuals. It uses a compact dark neutral surface with one accent selection state, rounded 10–12 px corners, Segoe UI / Microsoft YaHei UI, and no ornamental animation. A later settings page should expose system/light/dark theme and density while retaining the same information architecture.
