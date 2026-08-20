# Task 001: Adding a First-Person View

## Goal

Add an optional first-person view to the existing third-person ResidentHorrorV1 player, preserving the current third-person setup and allowing the player to switch between both camera modes.

This task should be implemented incrementally, documented as the project is explored, and validated in Unreal Engine 5.8. The preferred first slice is deliberately reversible: add a separate first-person camera rather than repurposing or overwriting the existing third-person camera.

## Project context

- Repository: `E:\repo\unreal_engine\ResidentHorrorV1`
- Project: `ResidentHorrorV1.uproject`
- Original template version: Unreal Engine 5.7
- Current converted checkout: Unreal Engine 5.8
- Project type: Blueprint-only; there is currently no project `Source/` module.
- Primary test map: `/Game/ResidentHorrorV1/Maps/Map_MechanicMap`
- Player Blueprint: `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1`
- Player Blueprint CDO: `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1.Default__BP_Resident_HorrorV1_C`
- Existing repository documentation:
  - `AGENTS.md`
  - `docs/gameplay-systems.md`
  - `docs/unreal-mcp.md`

The project was freshly cloned from the store before being opened and converted in UE 5.8. Preserve the original template behavior as the baseline when judging regressions.

## Repository state and safety notes

At the time this handoff was written, the last project commit pushed to `master` was:

```text
81555e9 Document gameplay systems and configure Unreal MCP
```

There are three known deleted external-actor assets in the working tree:

```text
Content/__ExternalActors__/ResidentHorrorV1/Maps/Map_MechanicMap/4/KI/PQJJFXRQQDHJ4XLI0BPAQV.uasset
Content/__ExternalActors__/ResidentHorrorV1/Maps/Map_MechanicMap/B/AI/MKK14MGU267J2DR61V8XCM.uasset
Content/__ExternalActors__/ResidentHorrorV1/Maps/Map_MechanicMap/E/JW/EOQKHZ360WYK7PZN7DWLCU.uasset
```

They likely correspond to the three map actors whose missing base class is `/Game/WWG_ZombieLite/Blueprints/BP_ZombieLite`. They were intentionally left uncommitted. Do not stage, restore, delete, or otherwise include them in this feature without a deliberate decision.

Unreal assets are binary. Modify `.uasset` and `.umap` files only through Unreal Editor or an Unreal-aware MCP/editor API. Do not edit them as text.

## Current camera setup

The existing player uses a conventional third-person shoulder camera:

- Camera component: `Camera_GEN_VARIABLE`
- Spring arm component: `SpringArm_GEN_VARIABLE`
- The camera is parented to the spring arm.
- The spring arm is parented to the inherited capsule component, exposed as `CollisionCylinder` in inspected data.

Observed camera properties:

```text
Relative location: (0, 0, 0)
Relative rotation: (0, 0, 0)
Field of view: 90
Use pawn control rotation: false
Active: true
```

Observed spring-arm properties:

```text
Relative location: (0, 0, 50)
Target arm length: 140
Socket offset: (0, 60, 0)
Use pawn control rotation: true
Camera lag enabled: true
Camera lag speed: 7
Collision test enabled: true
Inherit pitch/yaw/roll: true
```

This existing configuration should remain intact so switching back to third person restores the current template presentation exactly.

## Related player systems already observed

The character contains components and logic that may interact with the camera feature:

- `LineTraceInteract`
- `SaveLoad`
- `CameraShakeSource`
- `Health`
- `Audio`
- `DropItemManager`
- `Interact`
- `Inventory`
- `BaseWeapon`
- inherited capsule, mesh, and character movement components

The player Blueprint contains graphs or sections for:

- camera offset;
- clipping on/off;
- locomotion;
- weapons;
- zoom;
- interactions;
- flashlight;
- doors, ladders, teleporting, and item inspection;
- camera shake;
- music and injured-state updates.

The existing zoom logic changes camera FOV from approximately `90` to `58`. The first-person implementation must check how zoom chooses its target camera. If it directly references the current third-person camera component, first-person zoom will need an explicit follow-up adjustment.

## Input findings

The project normally uses Enhanced Input:

- Mapping context: `/Game/ResidentHorrorV1/Input/IMC_Player`
- Existing actions cover movement, look, jump, sprint, crouch, lean, interact, inventory, fire, reload, zoom, flashlight, pause, and any-key handling.

Inspection of the player Blueprint initially suggested raw debug input-key nodes including `V`, `B`, and `X`. Although the raw `V` event in the visible Debug composite is disconnected, runtime validation later confirmed that `V` already triggers the template's character-damage behavior elsewhere. Treat runtime behavior as authoritative and keep `V` reserved.

Preferred long-term input design:

1. Add a dedicated Enhanced Input action, such as `IA_ToggleView`.
2. Map it in `IMC_Player` to `T`, which is absent from both the mapping context and the visible raw debug-key set.
3. Handle that action in the player Blueprint.

Acceptable prototype design if the available tooling cannot yet create the Enhanced Input asset/event safely:

- Reuse or create a raw input-key event only after confirming both its graph connectivity and runtime behavior.
- Document this as prototype debt and migrate to Enhanced Input later.
- Never overwrite an existing key behavior based only on an apparently disconnected graph event.

## Proposed first implementation

Keep third person as the default and add a separate camera component, tentatively named `FirstPersonCamera`.

Suggested behavior:

1. Attach `FirstPersonCamera` to the character capsule/root at approximate eye height.
2. Give it a default FOV of `90` initially.
3. Enable pawn-control rotation as appropriate for first-person pitch/yaw.
4. Make it inactive by default.
5. Add a boolean such as `bIsFirstPerson`, defaulting to `false`.
6. On the toggle input:
   - invert `bIsFirstPerson`;
   - activate `FirstPersonCamera` when true;
   - deactivate `Camera_GEN_VARIABLE` when true;
   - reverse both states when false.
7. Prevent the local character head/body from obstructing the first-person camera.

Do not finalize the camera transform blindly. Inspect the player capsule height, base eye height, skeletal-mesh offset, head socket/bone, crouch behavior, and animations first. A starting relative Z near `64` may be reasonable, but it is only a tentative value.

## Investigation update: 2026-08-20

Read-only inspection through Unreal MCP and the Blueprint editor established the following details:

- Capsule half-height and radius: `90` and `34`.
- Standing and crouched eye heights: `58` and `32`.
- Character mesh relative transform: location approximately `(0, 0, -93)` and yaw `-90`.
- The Manny mesh contains a `CameraSocket` attached to `neck_01`. Using it couples view motion to full-body animation and may introduce camera jitter, but it was selected in the realism revision so the viewpoint follows the animated head/eye level.
- `V` is not mapped in `IMC_Player`, and the raw `V` event in the Debug composite has no connected logic. Runtime validation nevertheless confirmed that `V` already triggers character damage elsewhere in the template, so it is not available for the camera toggle. `T` is absent from both `IMC_Player` and the visible raw debug-key set.
- `IMC_Player` stores its current mappings in UE 5.8's `defaultKeyMappings` data.
- The interaction component does not use the player's `LineTraceInteract` scene component for its actual sight trace. `BPC_Interact` directly reads the existing player `Camera` component.
- `StartZoom` directly sets the existing camera FOV from `90` to `58` through `Zoom_TL`.
- `Line Trace Weapon`, `Cliping On`, and `Cliping Off` directly read the existing camera transform.
- `BPC_Base_Weapon` uses the player camera location as the weapon camera-shake origin.
- Ladder traversal calls the player's `Zoom camera` event with `120`, so switching views during ladder camera behavior must be prevented or explicitly coordinated.
- Existing booleans provide useful transition guards: `CanDoAction?`, `In Ladder?`, `Inspect Activate?`, `Teleport ?`, and `Open Menu Teleport?`.

Accepted first-slice design:

1. Add a dedicated Boolean Enhanced Input action named `IA_ToggleView` and map it to `T` in `IMC_Player`, preserving the original `V` behavior.
2. Add an inactive `FirstPersonCamera`, attach it at runtime to the mesh's `CameraSocket`, and use FOV `90`, pawn-control rotation, and zero local offset.
3. Add `bIsFirstPerson`, a focused camera-mode setter, and an active-gameplay-camera getter.
4. Keep exactly one player camera active at a time and leave the existing third-person camera and spring-arm defaults unchanged.
5. Route zoom, interaction, weapon traces, clipping traces, and weapon camera-shake origin through the active-camera getter.
6. Let standing, crouched, and animated eye height follow `CameraSocket`; keep the existing update function as a compatibility path that reapplies the zero socket-relative offset.
7. Reject toggles during ladder, inspection, teleport, or other states where `CanDoAction?` is false.
8. Keep the full head and body rendered to preserve realistic shadows; use the authored eye-level socket without an extra forward push.

Epic's UE 5.8 First Person Rendering system was reviewed as an alternative. It complements rather than replaces the camera toggle: it provides separate FOV and anti-clipping scale for primitives tagged as first person, but it does not reposition the viewpoint or switch between first- and third-person cameras. A primitive marked `FirstPerson` does not supply the required world shadow, so the single full-body mesh must not be switched directly to that type. The likely polished follow-up is dedicated first-person arm/weapon geometry using First Person Rendering plus a `WorldSpaceRepresentation` duplicate for shadows and reflections, while retaining the separate camera-mode architecture.

## Mesh and weapon considerations

The simplest first-person prototype may set the existing player mesh to `Owner No See` while in first person. This avoids seeing inside the character's head and torso, but it has important limitations:

- It can hide the entire body, so looking down will show no player body.
- Attached weapon meshes may also become hidden depending on ownership, attachment, and component visibility settings.
- Existing weapon aiming, recoil, reload animations, interaction traces, camera shake, flashlight, and inspection views were built around the third-person camera.
- Full-body animation can produce camera clipping if the first-person camera is attached directly to a head bone.

For the first slice, prioritize a functional view toggle without breaking third-person behavior. Treat polished first-person arms/weapons, face/body clipping, recoil, reload presentation, and animation layering as later tasks.

Potential later approaches include:

- hide only the head and obstructing upper-body sections locally;
- use a separate first-person arms mesh with dedicated animation assets;
- retain a visible lower body while separating arms/weapon presentation;
- add camera offsets and animation-aware stabilization;
- maintain separate first- and third-person weapon transforms.

## Animation architecture

The project uses a traditional player Animation Blueprint rather than Motion Matching/GASP:

- Main player animation asset: `ABP_Player`
- State machines and blend spaces drive locomotion.
- Aim offsets, cached poses, additive/layered bone blends, slots, and Control Rig foot IK are present.

This is sufficient to begin prototyping a first-person camera. It is not, by itself, a polished first-person animation solution. The first camera slice should avoid rewriting the animation architecture. Evaluate dedicated first-person arm and weapon animation only after the camera toggle works and the visibility problems are understood.

More animation notes are in `docs/gameplay-systems.md`.

## MCP setup and limitations

The project is configured to use Epic's native Unreal MCP at:

```text
http://127.0.0.1:8000/mcp
```

Project configuration lives in `.codex/config.toml`. The native MCP has been useful for:

- reading Blueprint components and properties;
- listing and reading graphs;
- inspecting objects;
- compiling Blueprints;
- broad graph-DSL operations.

The native MCP's graph writer is broad and potentially destructive for a large existing Event Graph. Its graph reader also omits or simplifies some disconnected/composite nodes. It should not be used to rewrite the entire player Event Graph just to add this small feature.

Our custom MCP repository is located at:

```text
E:\repo\unreal_engine\UE5MCPTest 5.8\Plugins\ClaudeUnrealMCP
```

Its last known pushed commit is:

```text
9dcb623
```

The custom MCP had a clean working tree and 152 registered tools when previously checked. It is not yet installed in ResidentHorrorV1. It has useful focused operations such as component/property editing, input mappings, graph reads, connecting/deleting/reconstructing nodes, but it does not yet expose a safe general-purpose arbitrary Blueprint node creator.

The agreed strategy is to use both MCP implementations:

- use Epic's native MCP as a reference and for operations it handles well;
- use and improve our custom MCP for focused, safer workflows;
- make reusable custom-MCP improvements rather than hard-coding this one game wherever practical.

See `docs/unreal-mcp.md` for the broader MCP comparison and plan.

## Proposed custom MCP improvement

This feature exposed a concrete tooling gap. A reusable, scoped camera-mode tool would be safer than replacing the player's full Event Graph.

A possible command name is `setup_camera_mode_toggle` or `setup_first_person_toggle`. It should accept parameters resembling:

```text
blueprint_path
third_person_camera_name
first_person_camera_name
parent_component_name
toggle_key or input_action_path
first_person_relative_location
first_person_relative_rotation
hide_owner_mesh
replace_existing (default false)
```

Desired behavior:

- Find or create the first-person `UCameraComponent` SCS node.
- Attach it to the requested inherited capsule/root component.
- Configure it as inactive by default.
- Add or reuse a `bIsFirstPerson` Blueprint variable.
- Find an existing toggle input only when it is safe to reuse.
- Add the minimal nodes needed to invert the state and call `SetActive` on both cameras.
- Optionally call `SetOwnerNoSee` on the player mesh.
- Refuse to overwrite connected input logic unless explicitly authorized.
- Be idempotent: running it twice must not duplicate components, variables, or logic.
- Compile the Blueprint and return compile errors and a precise mutation report.
- Avoid hard-coded ResidentHorror asset paths.

Relevant Unreal editor APIs already identified:

- `UK2Node_InputKey` exposes `FKey InputKey`, `GetPressedPin()`, and `GetReleasedPin()`.
- `UActorComponent::SetActive(bool bNewActive, bool bReset = false)` is Blueprint-callable.
- `UPrimitiveComponent::SetOwnerNoSee(bool)` is Blueprint-callable.
- Existing custom MCP code already uses `USimpleConstructionScript::CreateNode`, `USCS_Node`, `CreateNewGuid()`, and `AllocateDefaultPins()`.
- The custom MCP module already depends on `BlueprintGraph`, `Kismet`, `EnhancedInput`, and `Engine`.

Before implementing the custom command, inspect the exact UE 5.8 `USCS_Node` parent-component API and existing custom-MCP patterns for `UK2Node_CallFunction`, Blueprint variable get/set nodes, pin linking, transaction handling, compilation, and dirtying/saving assets.

If the custom MCP is changed, validate it independently before using it on the game project:

1. Verify the tool definition and handler registry contain no duplicate or missing registrations.
2. Build against Unreal Engine 5.8.
3. Run the standalone/non-unity plugin build used by that repository.
4. Commit and push the custom MCP separately.
5. Install or reference the tested revision in ResidentHorrorV1.
6. Run the command against the player Blueprint and inspect the result before saving unrelated assets.

## UI automation bridge status

Windows UI automation was initially unavailable because the Computer Use native pipe was missing. After restarting in the supported Codex desktop environment, the bridge was rechecked on 2026-08-20 and is now operational.

```text
Computer Use native pipe is unavailable:
failed to connect native pipe:
El sistema no puede encontrar el archivo especificado. (os error 2)
```

The bridge successfully enumerates and targets the live `ResidentHorrorV1 - Unreal Editor` and `BP_Resident_HorrorV1` windows. Use it only for focused visible editor operations that cannot be represented safely through Unreal MCP. Continue to prefer Unreal-native APIs and MCP for inspection, compilation, saving, and structured asset operations.

Installed applications observed:

```text
OpenAI.ChatGPT-Desktop 1.2026.43.0
OpenAI.Codex           26.810.4967.0
```

If the native-pipe failure returns, use this recovery sequence:

1. Fully exit the Codex desktop app, including its tray process.
2. Reopen the installed OpenAI Codex desktop app rather than an npm CLI or IDE terminal session.
3. Reopen this repository in a local desktop session.
4. Retry a lightweight Computer Use connection/list-apps request.
5. If it still fails, update the Codex app and use Windows Installed Apps > Codex > Advanced options > Repair.
6. Use Reset or reinstall only as a last resort because local app state may be cleared.

Do not replace the unavailable bridge with PowerShell `SendKeys`, custom mouse/keyboard helpers, or other brittle foreground automation. Prefer Unreal-native APIs and MCP for structural edits.

## Implementation result: 2026-08-20

The reversible first slice is implemented in UE 5.8:

- Added `/Game/ResidentHorrorV1/Input/Actions/IA_ToggleView` and mapped `T` once in `/Game/ResidentHorrorV1/Input/IMC_Player`. The initial `V` mapping was removed after runtime evidence showed that the template already uses `V` for character damage.
- Added `FirstPersonCamera` to `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1`, with FOV `90`, pawn-control rotation enabled, and auto activation disabled. The camera now snaps at runtime to the Manny mesh's `CameraSocket` and uses local offset `(0, 0, 0)`.
- Added `bIsFirstPerson`, pure `GetActiveGameplayCamera`, `SetCameraMode`, `CanToggleCameraMode`, and `UpdateFirstPersonCameraHeight` functions.
- The toggle keeps exactly one player camera active. The head and body stay rendered in both modes, preserving the complete character shadow.
- Camera switching is rejected while actions are disabled or the player is in ladder, inspection, teleport, or teleport-menu states.
- Standing, crouched, and animated eye height follow `CameraSocket`; the existing crouch and uncrouch flows reapply the zero socket-relative offset.
- `Cliping On` and `Cliping Off` now test whether `FirstPersonCamera` is active. First person bypasses the template's 200 cm camera-proximity weapon-clipping trace; third person retains the original response.
- The translucent-arm artifact was traced to the masked Manny material `/Game/ResidentHorrorV1/Demo/Mannequins/Materials/M_Mannequin`, not UE 5.8 First Person Rendering or the Blueprint clipping trace. Its instances default `Camera Faded Lowest Opacity` to `0`; `SetCameraMode` now applies `1` to the player mesh in first person and restores `0` in third person.
- First-person aim zoom is disabled: `SetCameraMode` normalizes the first-person camera to FOV `90`, and every `Zoom_TL` update selects FOV `90` while that camera is active. Third-person aim keeps the original `90` to `58` transition.
- Zoom, weapon shot traces, interaction/door traces, and the weapon camera-shake origin now resolve the active gameplay camera. The camera-proximity weapon-clipping trace uses the active camera only in third-person mode and is bypassed in first person.
- `BPC_Interact.GetInteractionCamera` and `BPC_Base_Weapon.GetWeaponGameplayCamera` are small pure wrappers around the player's active-camera accessor because those component graphs acquire the player through `BPFL_LibrayCharacter`.

Compiled and saved assets:

- `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1`
- `/Game/ResidentHorrorV1/Blueprints/BP_Interact/BPC_Interact`
- `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/Structure/BPC_Base_Weapon`
- `/Game/ResidentHorrorV1/Input/Actions/IA_ToggleView`
- `/Game/ResidentHorrorV1/Input/IMC_Player`

PIE validation in `/Game/ResidentHorrorV1/Maps/Map_MechanicMap` confirmed third-person startup, a clean eye-level first-person view without visible face geometry, repeated switching back to the original shoulder view, full head/body retention, and socket-following crouch/uncrouch movement. PIE stopped cleanly. No new camera-related compile or runtime errors appeared. Existing template warnings remain for duplicate Animation Blueprint slots, item actors without skeletal meshes, an unmatched switch case, and the absent Recast navmesh.

After correcting the input conflict, a focused PIE retest confirmed that `T` enters first person, `V` does not change camera mode, and a second `T` returns to third person. This preserves the original `V` action reported during runtime testing.

After the presentation revisions, an additional PIE smoke test confirmed that right-mouse aim input does not change first-person framing. The equipped-weapon animation/alignment regression remains part of the full weapon test matrix.

After the user reported hand and gun camera clipping while crouch-walking and looking down in first person, the arbitrary `6` cm local forward offset was removed. An unequipped PIE retest at the exact `CameraSocket` remained clear of face/body geometry while crouched and looking down. The reported equipped-weapon combination could not be reproduced in that spawn because no weapon was available, so it remains an explicit user/PIE validation item. If clipping persists, use the dedicated first-person arm/weapon plus `WorldSpaceRepresentation` design above instead of continuing to move the viewpoint away from the authored eye socket.

The full interaction, equipped-weapon, flashlight, inspection, ladder, teleport, save/load, and packaged-build regression matrix remains follow-up validation; those flows were routed and compiled but were not all exercised in this smoke test.

## Definition of done for the first slice

- [x] Third person remains the default and its existing camera/spring-arm defaults are intact.
- [x] The player can switch to and from a usable first-person camera with `T`.
- [x] The toggle uses Enhanced Input and preserves the template's existing runtime behavior on `V`.
- [x] All three modified Blueprints compile without errors.
- [x] The focused camera/crouch smoke test passes in `Map_MechanicMap`.
- [x] Known presentation and regression-test limitations are documented.
- [x] Project documentation records the final assets, dependencies, and validation result.
- [x] No custom MCP source change or new plugin dependency was required.
- [x] The three unrelated external-actor deletions remain outside this feature.

## Follow-up decisions

- Decide whether first-person mode should persist in save data; it currently resets to third person on spawn/load.
- Evaluate UE 5.8 First Person Rendering for arms/weapons while retaining the full-body mesh and complete character shadow.
- Retest holding a gun while crouch-walking and looking down at the exact zero-offset `CameraSocket`; if clipping remains, implement the separate first-person arm/weapon presentation rather than another camera-position workaround.
- Implement first-person-only hand-pose tuning in `/Game/ResidentHorrorV1/Demo/ABP_Character/ABP_Player`: pass in `bIsFirstPerson`, gate an additive upper-body/layered-per-bone correction on that value, preserve the existing third-person branch unchanged, apply weapon-specific grip offsets as needed, and run support-hand IK after the correction. Validate hip fire, aim, crouch, movement, reload, and steep look-down angles for each weapon class.
- Tune camera offsets, weapon alignment, recoil, reload, flashlight, lean, and animation stabilization after the full regression matrix.
- Decide whether door transitions or future cinematics require additional camera-mode guards.
- Perform an interaction/door, equipped-weapon, ladder, inspection, teleport, UI, save/load, and packaged-build validation pass before calling the broader feature production-ready.
