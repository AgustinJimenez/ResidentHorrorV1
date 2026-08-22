# Task 002: Bullet Impact vs. Gun Barrel Alignment

## Goal

Investigate and fix the reported issue: weapon hit impacts do not land where the gun barrel visually points. Determine whether the fire-trace `Start`/`End` are derived from the player camera, the weapon muzzle socket, or some blend of both, and correct the mismatch if the trace origin/direction diverges from the visible barrel.

This task is in the **investigation stage**; no gameplay fix has been implemented yet.

## Project context

- Repository: `E:\repo\unreal_engine\ResidentHorrorV1`
- Weapon component (owns fire logic): `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/Structure/BPC_Base_Weapon`
- Weapon base actor: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/BP_Base_Weapon`
- Pickup implementation used for testing: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/PickUp/HandGun/BP_Pistol` (its own EventGraph just forwards `Start Shooting` to the parent component; all trace logic lives in `BPC_Base_Weapon`)
- Existing repository documentation: `AGENTS.md`, `docs/gameplay-systems.md`, `docs/unreal-mcp.md`
- Related existing doc note (`docs/gameplay-systems.md`, first-person section): weapon traces share their origin with `GetActiveGameplayCamera` — i.e. traces are camera-based, not barrel-based, by design.

## Findings so far

Read via the `unreal-engine` (custom `ClaudeUnrealMCP`) MCP server, `read_event_graph_detailed` on `BPC_Base_Weapon`:

- The fire logic (`Start Shooting` custom event, in `BPC_Base_Weapon`'s single `EventGraph`) calls `LineTraceSingle` (node id `EE8F0246...`).
- `Start` is fed by a `K2Node_Composite` ("Line trace Weapon" collapsed graph) via a reroute node. This collapsed graph takes **no external inputs** — it derives everything internally (almost certainly from the active gameplay camera, per the existing docs note above).
- `End` is fed by a second `K2Node_Composite` ("Random line trace Shotgun" collapsed graph), which takes the first composite's `ReturnValue` as its base point (`A`) and three `self`/`self2`/`self3` inputs sourced from `Get Equipped Weapon` (on the character) — i.e. it re-derives a reference to the currently equipped weapon actor, likely to compute a spread cone. Notably this "shotgun" spread graph is shared by **all weapon types** including the pistol, since it lives on the shared `BPC_Base_Weapon` component.
- One of the "Line trace Weapon" composite's outputs also feeds a `Rotation From X Vector` node, suggesting the computed trace direction is reused to orient VFX (muzzle flash / laser) to match.

**Root-cause hypothesis (not yet confirmed):** the hit-scan trace originates from the player camera (aim point), while the visible weapon barrel is offset from the camera (third-person shoulder camera, and even in first-person the barrel isn't exactly at the eye/camera position). At close range or near geometry edges, this is a classic cause of "the shot didn't come from where the gun visibly points." This has **not been confirmed** by reading the actual math inside the two collapsed graphs — see blocker below.

## Blocker: collapsed/composite graphs are opaque to both MCPs

Neither MCP server currently exposes the contents of a Blueprint "Collapsed Graph" (`K2Node_Composite`'s `BoundGraph`):

- The custom `ClaudeUnrealMCP`'s `read_event_graph`/`read_event_graph_detailed`/`read_function_graphs` only iterate `Blueprint->UbergraphPages` / `Blueprint->FunctionGraphs` — they do not recurse into a composite node's bound subgraph.
- `docs/unreal-mcp.md` already documents this exact gap (see "Visible editor automation fallback" section) and recommends the Windows Computer Use bridge for narrow, visible operations when "the native MCP reader cannot traverse a collapsed/composite graph" — **not** a custom MCP/C++ change.
- UE's Python console (`py ...`, confirmed available and working via `execute_console_command` + reading `Saved/Logs/ResidentHorrorV1.log`) also cannot help here: `UEdGraph::Nodes` is blocked from Python reflection (`"Property 'Nodes' ... is protected and cannot be read"`), even though the same property is plain public C++ and freely read by the MCP plugin's own native code.

### Incident: Live Coding crash (do not repeat this approach)

During this investigation a `read_collapsed_graph` C++ handler was added to `Plugins/ClaudeUnrealMCP` to recurse into composite subgraphs, and `LiveCoding.Compile` was triggered from the running editor to hot-patch it in. The compile **crashed the editor** (`EngineUnhandledExceptionFilter`, editor force-exited) immediately after linking. The link step also wrote output into `E:\repo\unreal_engine\UE5MCPTest 5.8\Plugins\ClaudeUnrealMCP\Binaries\...` — a **different project's** path — which lines up with `docs/unreal-mcp.md`'s note that the "validated" working copy of this plugin lives at that `UE5MCPTest 5.8` location; Live Coding appears to have resolved module/object paths against that reference copy rather than this project's copy.

The editor was reopened successfully afterward (no apparent data loss — no Blueprint edits had been made yet, only reads), and the C++ change was reverted (`git checkout` on the four touched plugin files) to restore the plugin to its committed state.

**Lesson recorded:** do not attempt to extend/hot-compile `ClaudeUnrealMCP` via Live Coding to solve a read-only inspection gap. Follow `docs/unreal-mcp.md`'s documented fallback (visible/UI-driven editor automation, e.g. opening the collapsed graph in the Blueprint editor and inspecting/screenshotting it) or make a deliberate, out-of-editor plugin rebuild (close editor, rebuild, reopen) if a permanent MCP capability improvement is later justified through the doc's "Improvement loop" process.

## MCP setup notes discovered along the way

- This project actually runs **two** Unreal MCP servers simultaneously, exactly as `docs/unreal-mcp.md` describes: Epic's native `ModelContextProtocol` plugin (port `8000`, `/mcp`, streamable HTTP) and the custom `ClaudeUnrealMCP` (TCP JSON, port `9877`). Only the custom one was wired into this Claude Code project's config at the start of this task; the native one has since been added too (`unreal-engine-official` in `~/.claude.json`, `http://127.0.0.1:8000/mcp`).
- Added a `CLAUDE.md` at the repo root (previously missing) that points to `AGENTS.md` and `docs/unreal-mcp.md`, since Claude Code auto-loads `CLAUDE.md` but not `AGENTS.md` — this task's early missteps (attempting a live C++ patch) happened partly because that routing doc wasn't loaded automatically.
- `codegraph` (`.codegraph/`) was initialized for this repo (`codegraph init -i`); it only indexes text-based source (the `ClaudeUnrealMCP` plugin's C++/JS, 57 files), not `.uasset` Blueprint content, so it is not useful for the Blueprint-graph part of this investigation.

## Next steps

- [ ] Use Epic's native MCP (`unreal-engine-official`, now configured — needs a Claude Code session restart to activate) to see whether its Blueprint/graph tools can traverse the collapsed graphs where the custom MCP cannot.
- [ ] If still blocked, open `BPC_Base_Weapon`'s `EventGraph` in the Blueprint editor, double-click into the "Line trace Weapon" and "Random line trace Shotgun" collapsed graphs, and read/screenshot their contents directly (per the documented UI-automation fallback) rather than via MCP graph-dump tools.
- [ ] Confirm whether `Start` is exactly the active camera location (and `End` the camera forward-projected aim point), or whether a muzzle-socket transform is blended in anywhere.
- [ ] If confirmed camera-based: decide and implement a fix — e.g. trace from the muzzle socket toward the camera's aim point (`End`) rather than from the camera itself, so the visible barrel and the hit-scan agree at short range. Validate pistol, both shotguns, and AS VAL, in both third-person and first-person camera modes.
- [ ] Re-run the standard weapon validation path from `docs/gameplay-systems.md` (pickup, aim, fire, ammo, reload) after any Blueprint change, plus a focused PIE test aiming at a nearby wall/corner from an angle to confirm impacts now match the visible barrel direction.
