# Task 003: Hip-Fire (Fire-Without-Aiming) Weapon Mode

## Goal

Add a second fire mode alongside the existing aim-gated fire: pressing `LMB` alone (without holding `RMB`/`Z` to aim) should raise the weapon into a ready pose, fire once ready, keep the arm raised for a short hold window to allow rapid follow-up shots, and lower the arm automatically if firing stops and the player isn't genuinely aiming. The existing aim-down-sights fire path (`RMB`/`Z` then `LMB`) must remain unchanged.

## Status: implemented and structurally verified; dynamic (PIE) verification incomplete

The Blueprint wiring is complete, compiles cleanly (0 errors/warnings across `/Game/ResidentHorrorV1/Character`), and was saved. It was **observed working correctly once** in a clean automated PIE test. Follow-up automated re-tests hit an unresolved input-simulation quirk (see below) that prevented full confirmatory coverage of the hold/lower behavior. A manual PIE playtest is recommended to close this out.

## What changed

### `BP_Resident_HorrorV1` (character)

New variables: `HipFireRaised?` (bool), `HipFireRaiseDuration` (float, default `0.35`), `HipFireHoldDuration` (float, default `2.0`).

**`Fire Weapon` collapsed graph** (inside `EventGraph`, node id `16352046446C1C1375CB92B2219CD745`): the previously-empty `Aim?==false` branch of the existing `In Ladder? -> Reloading? -> Aim?` gate chain now reads:

```
Aim?==false
  -> Branch(HipFireRaised?)
       true  -> [existing Is Valid(Equipped Weapon)] -> Start Shooting   (continuous fire while held)
       false -> Call "Raise Weapon (Hip Fire)"
                -> Retriggerable Delay(HipFireRaiseDuration)
                -> Set HipFireRaised? = true
                -> [same Is Valid(Equipped Weapon)] -> Start Shooting
```

Every successful `Start Shooting` (both this path and the pre-existing `Aim?==true` path) now also feeds a second Retriggerable Delay keyed to `HipFireHoldDuration`; when it expires without another shot re-triggering it, and only if `Zoomed?==false` (i.e. the player isn't genuinely aiming), it sets `HipFireRaised? = false` and calls `Lower Weapon (Hip Fire)`. `IA_Fire`'s `Completed` pin (LMB release) was left untouched — the hold-timer approach means lowering doesn't need to hook release at all.

**`Aim Weapon` collapsed graph** (node id `852EC4AA411AE1481B508BA671F5AAE0`): two new custom events, callable from anywhere in the Blueprint:
- `Raise Weapon (Hip Fire)` -> `Set Aim? = true`. Does **not** call `StartZoom`, so the camera FOV/`Zoomed?` is untouched.
- `Lower Weapon (Hip Fire)` -> `Branch(Zoomed?)` -> only if `Zoomed?==false` -> `Set Aim? = false`. This guards against clobbering a real aim session that started while the hip-fire hold timer was pending.

Both events were placed alongside (not replacing) the existing `For aim ON`/`Force Aim Off` custom events, which remain the real-aim (`RMB`/`Z`) path, untouched.

### `IMC_Player` (Enhanced Input mapping context)

Removed two pre-existing duplicate key bindings that predate this task but directly broke the hip-fire distinction: `LeftMouseButton` was mapped to **three** actions simultaneously (`IA_Fire`, `IA_Zoom`, `IA_LeanLeft`). Pressing LMB alone therefore already triggered the aim/zoom flow before this task, making a separate "fire without aiming" mode impossible. Removed the `LeftMouseButton -> IA_Zoom` and `LeftMouseButton -> IA_LeanLeft` mappings; `LeftMouseButton` now maps only to `IA_Fire`. `RightMouseButton`/`Z` still map to `IA_Zoom`; `Q`/`E` still map to leaning.

Note: `E` and `F` remain similarly dual-mapped (`E` -> `IA_Interact` + `IA_LeanRight`; `F` -> `IA_Fire` + `IA_Flashlight`) and were **not** touched — out of scope for this task, but worth a future cleanup pass since the same class of bug likely affects them too.

## Bullet-impact-from-barrel finding (feeds back into task 002)

While tracing `Fire Weapon`, also found and read the character's own `Line Trace Weapon` function (a real Function, not a collapsed graph): it branches on the weapon's `Have laser?` bool — if true, returns the weapon's `Arrow` component's transform (the barrel); by default (false), returns `GetActiveGameplayCamera()`'s transform. This **confirms** task 002's root-cause hypothesis: the hit-scan trace is camera-based by default, not barrel-based, and the weapon already has an unused barrel-aligned `Arrow` component that could anchor a fix. `BPC_Base_Weapon`'s `Line trace Weapon`/`Random line trace Shotgun` collapsed graphs call this same function; `Start = Line Trace Weapon().ReturnValue`, `End = Start + Line Trace Weapon().ReturnValue1.Forward * Get Shooting Range`. Task 002's Step 4 (barrel-aligned trace) is not yet implemented — only investigated further.

## MCP tooling built along the way (all now in `Plugins/ClaudeUnrealMCP`)

Implementing this feature required extending the custom MCP plugin, since neither it nor Epic's official `ModelContextProtocol`/`EditorToolset` could read *or* write inside Blueprint "Collapsed Graphs" (`K2Node_Composite`) before this task — confirmed both by the earlier read-only gap (task 002) and by Epic's own `BlueprintTools.read_graph_dsl` throwing `Cannot cast type 'K2Node_Composite' to 'Blueprint'` on any nested graph. New commands, each built with the editor closed via `UnrealBuildTool` (never Live Coding — see the incident logged in task 002):

- `read_collapsed_graph` (path, node_ids: NodeGuid chain) — reads inside nested collapsed graphs, following the same per-node/pin/connection shape as `read_event_graph_detailed`.
- `create_node` (path, node_type: Branch/CallFunction/VariableGet/VariableSet/CustomEvent, node_ids) — creates a new node, including inside a nested collapsed graph. `CallFunction` resolves the target `UFunction` from an explicit `function_owner_class`, the Blueprint's own class (covers calling a just-created Custom Event by name), or `KismetSystemLibrary` as a fallback (covers `Delay`/`RetriggerableDelay`).
- `add_variable` (path, name, type: bool/float/int, default_value) — adds a new Blueprint member variable.
- `remove_input_mapping` (context_path, action_path, key) — symmetric counterpart to the existing `add_input_mapping`; wraps `UInputMappingContext::UnmapKey`.
- `simulate_input_key` (key, event: Pressed/Released/Repeat, value, player_index) — simulates a key/button event in a running PIE session via `APlayerController::InputKey(FInputKeyEventArgs::CreateSimulated(...))`, the same mechanism Epic's own automation tests use. Requires PIE already running.
- `connect_nodes` was extended to accept an optional `node_ids` chain (same composite-graph resolution as the above), in addition to its existing top-level `graph_name` lookup.

`set_pin_default` needed no changes — it already resolves nested composite graphs via `UBlueprint::GetAllGraphs()`, which does recurse into `K2Node_Composite` bound graphs (unlike the other pre-existing handlers, which only checked `UbergraphPages`/`FunctionGraphs` directly).

Also discovered: UE's console `set <Object> <Property> <Value>` command is blocked entirely ("Set commands not allowed in the editor") for a PIE session running inside the editor — cannot be used to force state for testing. `GetAll <Class> <Property>` is **not** blocked and reliably reads live PIE actor state straight into the log, keyed by the actor's PIE-world path (e.g. `UEDPIE_0_<Map>.<Map>:PersistentLevel.<Actor>`) — this became the primary tool for automated state verification in this task, alongside `read_actor_properties`/`list_actors`/`find_actors_by_name`, which are confirmed **editor-world only** (they do not see live PIE-spawned/possessed actors).

## Automated PIE test: what was confirmed, what wasn't

**Confirmed, with a clean repeatable observation:**
- Fresh PIE start in `Map_FPV_PoseLab` (which auto-equips the pistol): `Aim?` starts `false` (clean).
- `simulate_input_key` for `LeftMouseButton` "Pressed" (no `RMB`/`Z` held) correctly drove `Aim? -> true` and `HipFireRaised? -> true` — the hip-fire raise path fired exactly as designed.
- `simulate_input_key` for `Z` "Pressed"/"Released" reliably and repeatably drove `Zoomed?`/`Aim?` through the real `Aim Weapon` logic, in both `Map_FPV_PoseLab` and `Map_MechanicMap`, confirming the input-injection mechanism itself works.
- Re-reading `Fire Weapon`'s full structure after later editor restarts showed it byte-for-byte unchanged (only cosmetic pin-order/compiled-`LatentInfo` differences) — no corruption from any of the several close/rebuild/reopen cycles this task required.

**Not resolved — a real but likely narrow quirk:** in later re-test attempts, `LeftMouseButton`/`F`/`E` intermittently stopped registering via `simulate_input_key` (state didn't change even though the call reported `handled: true`), while single-purpose keys (`Z`, `C`) kept working reliably every time. The apparent pattern: **keys mapped to more than one Enhanced Input action** (`E` -> `IA_Interact` + `IA_LeanRight`; `F` -> `IA_Fire` + `IA_Flashlight`; `LeftMouseButton` originally -> three actions) behave inconsistently under simulated input, while singly-mapped keys don't. This was not root-caused — plausible explanations include a genuine Enhanced Input engine quirk specific to simulated (non-hardware) input fanning out to multiple action bindings, or simply that the test script didn't leave a full clean frame between rapid-fire simulated events for these cases. Real hardware input was not tested, so it's unknown whether this affects actual play.

Also relevant: `Map_FPV_PoseLab`'s harness (`BP_FPV_PoseLabHarness`) starts with `Zoomed?==true` already set (before any input) and switches input mode to "Game and UI" for its live tuner widget — this makes it an environment with side effects for this specific test, not a neutral bed, and should be kept in mind for any future automated test in that map.

## Bug found and fixed during user playtest: `Zoomed?` defaulted to `True`

User report: "the very first time I equip the pistol, it automatically aims, then I hit right click and release, it unlocks, then it's normal." `read_class_defaults` on `BP_Resident_HorrorV1` showed `Zoomed?`'s class default (CDO) was `True`, while every sibling state flag (`Aim?`, `IsSprinting?`, `Reloading weapon?`, `In Ladder?`, etc.) correctly defaults to `False` — a pre-existing template/project bug, unrelated to this task's edits (never touched by any of the create_node/connect_nodes work above). Every spawned character therefore started "zoomed"; the first `RMB` press+release exercises `Force Aim Off` for the first time, which explicitly sets `Zoomed? = false`, permanently correcting it for that session — matching the reported symptom exactly.

Fixed via Python console (`unreal.get_default_object(bp.generated_class()).set_editor_property("Zoomed?", False)`, then `bp.modify()` + `EditorAssetLibrary.save_loaded_asset(bp)`), since neither `set_blueprint_cdo_property` (object-reference properties only) nor the existing `run_python` `set_property` op (loads via `UEditorAssetLibrary::LoadAsset`, which resolves the `UBlueprint` object, not its CDO) covered a plain bool CDO property. Verified via a fresh `read_class_defaults` showing `Zoomed?` now `False`, and confirmed durable across an explicit `compile_blueprint` (does not get regenerated/reverted by recompiling). Worth adding a dedicated `set_blueprint_cdo_bool_property` (or generalizing `set_blueprint_cdo_property`) to the plugin if this pattern recurs.

## Follow-up: "raised weapon pose on equip" — investigated, not a bug

After the `Zoomed?` fix, user reported a second, distinct symptom: on first equip the character's arm/weapon looks raised into a ready pose (not a camera-FOV zoom — confirmed separately from the `Zoomed?` bug above). Investigated by:

1. Confirming `Zoomed?`/`Aim?` class defaults on both `BP_Resident_HorrorV1` and `ABP_Player` are correctly `False`.
2. Confirming `Equip Weapon Player`'s own logic has zero references to `Zoomed?`/`Aim?`/`StartZoom`/`Cliping On`.
3. Calling `Equip Weapon Player` directly on the live PIE pawn via Python (`pawn.call_method("Equip Weapon Player", (pistol_cls,))`, since `simulate_input_key`-driven `E`/Interact pickup wasn't reliable — see below) and reading state immediately after: `Aim?=False`, `Zoomed?=False`, camera `FieldOfView=90` — all clean.
4. Tracing `ABP_Player`'s main `AnimGraph` (readable via `read_function_graphs` with `name: "AnimGraph"` — for an Animation Blueprint this graph lives in `FunctionGraphs`, not `UbergraphPages`) to the actual `Blend Poses by bool` node gating on `Get Aim?`: when `Aim?==false` it correctly selects the base/locomotion pose (via a `Component To Local` conversion), not any aim pose. Wiring is correct; no stuck state possible here since this is a data-driven blend, not a persistent-state machine.
5. Per user's request, added temporary `PrintString` debug taps (via `create_node`/`connect_nodes`, non-invasively fanned out from each event's existing `then` pin so nothing existing was disturbed) at the 5 places any aim-adjacent logic could fire: `Equip Weapon Player`'s entry, `For aim ON`, `Force Aim Off`, `Raise Weapon (Hip Fire)`, `Lower Weapon (Hip Fire)`. Re-ran the equip test: **only** `[LOG] Equip Weapon Player called` printed — none of the four aim/hip-fire logs fired.
6. Confirmed with the user what the pose actually looks like: a raised, two-handed "ready" stance (not a tight ADS/ready-to-fire lock).

**Conclusion: this is intended behavior, not a bug.** The raised stance is the template's normal "weapon equipped" idle pose (survival-horror convention — weapon held up and ready whenever equipped, not lowered), driven by `Using Weapon?` in `ABP_Player`, entirely independent of the `Aim?`/`Zoomed?` ADS system. No fix applied. The 5 debug `PrintString` nodes were removed afterward (via `delete_node`, extended below) and the Blueprint re-saved clean (0 errors/warnings).

### More MCP tooling built during this investigation

- **`set_anim_state_machine_entry`** (path, node_ids, target_state_node_id) — rewires an `AnimStateEntryNode` to a different default entry state. Built to test a "wrong default entry state" hypothesis for the raised-pose symptom; the hypothesis turned out wrong (there is no dedicated aim state machine — `ABP_Player`'s only state machines are locomotion, under `AnimGraphNode_StateMachine_0.UnarmedMovement`), but the tool is now in place for any future Animation Blueprint state-machine work.
- **`ResolveGraphChain`** (the shared helper backing `read_collapsed_graph`/`create_node`/`connect_nodes`) now also descends through `AnimGraphNode_StateMachine`/`AnimGraphNode_StateMachineBase` nodes (via `EditorStateMachineGraph`), not just `K2Node_Composite`. Discovered via Epic's `BlueprintTools.list_graphs`, which conveniently enumerates state machine states/transitions by name too (e.g. `AnimGraph.AnimGraphNode_StateMachine_0.UnarmedMovement.AnimStateNode_0.OnGround...`) — useful for locating a state machine's NodeGuid chain before calling our tools.
- **`delete_node`** extended the same way (previously only searched `UbergraphPages` by exact name; now uses the shared resolver, so it also reaches `FunctionGraphs`/`Interface` graphs by name and nested composite/state-machine graphs via `node_ids`).
- Confirmed an Animation Blueprint's main `AnimGraph` is read via `read_function_graphs` (it lives in `Blueprint->FunctionGraphs`, same as any other function), not `read_event_graph`.
- Confirmed (again) that `E` behaves inconsistently under `simulate_input_key` in a real gameplay map too (not just the pose-lab harness) — matching the earlier-documented pattern of multi-mapped keys (`E` → `IA_Interact` + `IA_LeanRight`) misbehaving under simulated input. Equip testing was done by calling `Equip Weapon Player` directly via Python instead. Note: `Equip Weapon Player`'s `Weapon to equip` parameter is a `TSubclassOf<BP_Base_Weapon_C>` (a class), not an actor instance — pass the weapon's class, not a pickup actor reference.

## Follow-up / next steps

- [ ] Manual PIE playtest of the actual, intended experience: equip pistol in `Map_MechanicMap`, tap LMB without aiming (arm should raise then fire once), hold LMB (continuous fire, arm stays up), release and wait ~2s (arm should lower), then confirm `RMB`/`Z` + `LMB` aim-fire is unchanged. This is the most direct way to close out the "not fully confirmed" gap above with real input.
- [ ] If the multi-mapped-key simulated-input quirk needs to be resolved (e.g. to build further automated regression tests), investigate whether it's specific to `simulate_input_key`/`FInputKeyEventArgs::CreateSimulated`, or reproduces with real hardware input on `E`/`F` too — if the latter, it's a separate pre-existing bug worth its own task.
- [ ] Complete task 002 Step 4: align `BPC_Base_Weapon`'s trace `Start` with the weapon's `Arrow` (barrel) component instead of the camera, using `read_collapsed_graph`/`create_node`/`connect_nodes` on `Line trace Weapon` the same way this task did for `Fire Weapon`/`Aim Weapon`.
- [ ] Consider the same dual-mapping cleanup for `E` (`IA_Interact`/`IA_LeanRight`) and `F` (`IA_Fire`/`IA_Flashlight`) that was done for `LeftMouseButton`, if they're found to cause real gameplay issues (e.g. flashlight toggling every time `F` is used as an alternate fire key).
- [ ] Re-run the standard weapon validation checklist from `docs/gameplay-systems.md` (ammo accounting, reload, empty-ammo behavior) since `Start Shooting`'s call sites changed.
- [ ] Update `docs/gameplay-systems.md`'s "Weapon firing rules and diagnostics" section with the new hip-fire mode once manually confirmed.
- [ ] Update `docs/unreal-mcp.md`'s custom-MCP tool table/known-limitations with the new `read_collapsed_graph`/`create_node`/`add_variable`/`remove_input_mapping`/`simulate_input_key` commands and the composite-graph read/write capability, per its own documented "Improvement loop".
