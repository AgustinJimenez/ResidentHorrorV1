# Template gameplay systems

## Purpose and evidence level

This document is the functional inventory for the ResidentHorrorV1 store template. Use it to understand what the template appears designed to provide, locate the relevant Unreal assets, plan extensions, and record runtime validation as development proceeds.

The catalog was created from the assets present in this checkout on 2026-08-20. Asset names and paths establish that an implementation exists, but they do not prove that every feature is connected, complete, or working after the UE 5.7 to 5.8 conversion. Blueprint graphs, map instances, and Play In Editor behavior remain the source of truth.

Status terminology:

- **Configured** means project settings or input assets explicitly select the system.
- **Asset-backed** means the relevant Blueprints, interfaces, data assets, widgets, or animations exist.
- **PIE-verified** means the complete player-facing flow has been exercised successfully in the converted UE 5.8 checkout.
- **Needs verification** means the assets exist but the end-to-end behavior has not yet been recorded here.

At initial publication, the systems below are asset-backed but not comprehensively PIE-verified.

## Feature summary

| System | Template capability indicated by current assets | Initial status |
|---|---|---|
| Player | Third-person movement, camera look, jump, sprint, crouch, lean, zoom, interaction, inventory, flashlight, weapons, and pause input | Configured; needs verification |
| Interaction | Reusable interaction component/interface and master interactable base | Asset-backed; needs verification |
| Inventory and items | Slot-based inventory, drag UI, item actions/descriptions, containers, inventory expansion, inspection, item notifications, and data-driven item definitions | Asset-backed; needs verification |
| Weapons | Pistol, two shotgun variants, AS VAL, ammo pickups, firing, reloading, aiming/zoom, projectiles, magazines, casings, recoil effects, and damage interfaces | Asset-backed; needs verification |
| Health | Reusable health component, damage/healing actors, herbs, first-aid spray, and death UI | Asset-backed; needs verification |
| Doors and keys | Wood and metal doors, keyed variants, door states, collision customization, and opening animations | Asset-backed; needs verification |
| Enemies and AI | Master enemy, AI controller, Blackboard, Behavior Tree, patrol routes, perception/state data, attack, damage, death, focus, and movement tasks | Asset-backed with a known missing zombie dependency; needs verification |
| Save/load | Local and global SaveGame classes, save component/interface, slots, in-game save UI, load menu, and saving alert | Asset-backed; needs verification |
| UI and menus | HUD, prompts, inventory, pause, settings, instructions, credits, death screen, teleport UI, main menu camera/game mode/controller, and settings save | Asset-backed; needs verification |
| Animation | Player locomotion, crouch, leaning, stopping, weapon, door, ladder, attack, hit, death, flashlight, and AI mannequin animations | Asset-backed; needs verification |
| Audio and atmosphere | Physical-surface footsteps, weapon/character/UI audio, dialogue/world sound, ambient/combat music triggers, camera shake, and light flicker | Asset-backed; needs verification |
| Traversal and utilities | Ladders, teleportation, item inspection, laser, open-URL utility, and character function library | Asset-backed; needs verification |

## Player framework and controls

Primary assets:

- Character: `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1`
- Player controller: `/Game/ResidentHorrorV1/Character/BP_Character/Game/BP_PlayerControllerResidentHorror`
- Game mode: `/Game/ResidentHorrorV1/Character/BP_Character/Game/BP_PlayerModeResidentHorror`
- Player reference interface: `/Game/ResidentHorrorV1/Character/BP_Character/Game/BPI_PlayerReference`
- Game instance: `/Game/ResidentHorrorV1/Blueprints/BP_Core/ResidentHorrorV1_GameInstance`
- Player Animation Blueprint: `/Game/ResidentHorrorV1/Demo/ABP_Character/ABP_Player`
- Input Mapping Context: `/Game/ResidentHorrorV1/Input/IMC_Player`

Enhanced Input actions exist for move, look, jump, sprint, crouch, left/right lean, interact, inventory, fire, reload, zoom, flashlight, pause, and any-key handling. Camera-shake assets support idle, walking, running, jumping, being hit, and weapon fire.

The character folder also contains drop-item data structures and `BP_ResidentHorror_DropItemManager`, indicating that dropping inventory items is intended to be part of the player flow.

Validation path:

1. Start PIE in `/Game/ResidentHorrorV1/Maps/Map_MechanicMap`.
2. Confirm the expected pawn and controller are active despite the known default-game-mode config reference issue.
3. Test movement, look, jump, sprint, crouch, both lean directions, zoom, and camera behavior.
4. Open and close inventory and pause UI, toggle the flashlight, and exit PIE cleanly.

### Optional first-person view

Implemented and smoke-tested on 2026-08-20; detailed investigation and follow-up validation are tracked in `AGENT_TASKS/001-adding_first_person_view.md`.

The first slice preserves the existing third-person shoulder camera and adds the inactive `FirstPersonCamera` to `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1`. When camera mode is set, the first-person camera attaches to the Manny mesh's `CameraSocket` on `neck_01`, near eye level, with zero relative offset. `/Game/ResidentHorrorV1/Input/Actions/IA_ToggleView` is mapped to `T` in `/Game/ResidentHorrorV1/Input/IMC_Player`; `V` remains reserved for the template's existing runtime damage/debug behavior. `bIsFirstPerson`, `SetCameraMode`, and the pure `GetActiveGameplayCamera` function keep exactly one camera active and provide the shared origin for interaction/door traces, weapon traces, third-person weapon clipping, and weapon camera shake. Third-person aim retains its `90` to `58` FOV zoom; first-person aim holds FOV at `90`.

Verified player dimensions relevant to the initial camera placement are capsule half-height `90`, capsule radius `34`, standing eye height `58`, crouched eye height `32`, and skeletal-mesh relative Z `-93`. The selected `CameraSocket` is attached to `neck_01`; this deliberately inherits body animation for a more physical viewpoint, so animation jitter and weapon alignment require continued tuning.

UE 5.8 First Person Rendering is a presentation follow-up, not a substitute for camera-mode switching. It may later be used for dedicated first-person arm and weapon primitives with independent first-person FOV and anti-clipping scale. Do not mark the single full-body mesh as `FirstPerson`: first-person primitives do not provide the required world shadow. A polished setup that preserves the realistic full-body shadow needs separate first-person presentation geometry plus a `WorldSpaceRepresentation` duplicate for shadows and reflections.

The first-person camera uses local socket offset `(0, 0, 0)` after snapping to `CameraSocket`; standing and crouched height now follow the animated skeleton naturally. An earlier `(6, 0, 0)` forward offset was removed after crouch-walking with an equipped gun and looking down exposed hand/weapon camera clipping. The toggle is blocked when normal actions are disabled and during ladder, inspection, teleport, or teleport-menu states. The full head and body remain rendered in first person, preserving complete shadow geometry. The Manny masked material `/Game/ResidentHorrorV1/Demo/Mannequins/Materials/M_Mannequin` contains a built-in camera-distance dither controlled by `Camera Faded Lowest Opacity`; `SetCameraMode` overrides it to `1` in first person and restores `0` in third person so nearby arms remain opaque. `SetCameraMode` also normalizes first-person FOV to `90`, and `StartZoom` selects `90` whenever `FirstPersonCamera` is active so an in-progress third-person zoom cannot carry over. The template's separate `Cliping On` and `Cliping Off` camera-proximity weapon response is bypassed while `FirstPersonCamera` is active and remains unchanged in third person. First-person arms/weapons and animation stabilization remain future presentation work.

Focused PIE validation in `Map_MechanicMap` confirmed third-person startup, repeated `T` switching, clean eye-level first-person framing without visible face geometry, socket-following crouch/uncrouch movement, full head/body retention, unchanged first-person framing on right-mouse aim input, and clean PIE exit. After removing the forward offset, an unequipped crouch/look-down retest remained clear of face and body geometry. The exact reported equipped-gun, crouch-walk, look-down combination still requires validation with a weapon available in PIE. `V` no longer toggles the camera, preserving its existing action. The routed interaction, equipped-weapon, flashlight, ladder, inspection, teleport, save/load, and packaged-build flows still require a complete regression pass.

First-person hand-pose tuning reuses the existing `Hand Cliping Rotation` path and extends it with `Hand Cliping Location`. `/Game/ResidentHorrorV1/Character/BP_Character/BP_Resident_HorrorV1` exposes instance-editable `FirstPersonHandPoseRotation`, default `(Pitch=0, Yaw=6, Roll=7.705933)`, and `FirstPersonHandPoseTranslation`, default `(X=-8.435, Y=17.298, Z=16.190)`, under `Camera|First Person`. While `FirstPersonCamera` is active, `Cliping On` interpolates both runtime values toward those targets at speed `10`. `/Game/ResidentHorrorV1/Demo/ABP_Character/ABP_Player` applies both values additively in component space to `upperarm_r` and `upperarm_l` after the weapon and flashlight pose selections. Returning to third person interpolates translation and rotation back to zero and preserves the original wall-clipping flow.

The `6` degree yaw is deliberately a first tuning value rather than a baked animation decision. Translation provides lateral/vertical/depth control that rotation alone could not. The correction is downstream of the current weapon-pose selections, so it remains active for an equipped pistol instead of being bypassed by `Using Weapon?`. If shared upper-arm transforms cannot cover pistol, shotgun, rifle, hip-fire, aim, reload, crouch, movement, and steep look-down states cleanly, the next stage is a first-person-only additive/layered-per-bone pose in `ABP_Player`, weapon-specific grip offsets, and support-hand IK after the correction.

#### FPV weapon-pose lab

`/Game/ResidentHorrorV1/Maps/Dev/Map_FPV_PoseLab` is an isolated development map duplicated from Unreal's basic template map. It contains only the template floor/lighting/player start plus `/Game/ResidentHorrorV1/Blueprints/Dev/BP_FPV_PoseLabHarness`, and its World Settings override uses `/Game/ResidentHorrorV1/Character/BP_Character/Game/BP_PlayerModeResidentHorror`. It does not modify or depend on actors in `Map_MechanicMap` and must not be configured as a shipping startup map.

On PIE startup, `BP_FPV_PoseLabHarness` acquires the normal player pawn, spawns and equips `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/PickUp/HandGun/BP_Pistol`, crouches the player, enables first person, and continuously adds movement input along a `600` cm lane. It reverses at `-300` and `+300` cm so the genuine crouch-walk animation remains observable instead of freezing a single frame. It also creates `/Game/ResidentHorrorV1/Blueprints/Dev/WBP_FPV_PoseTuner`, enables the mouse cursor, and uses Game-and-UI input. Instance-editable harness values are `AutoWalk`, `WalkRadius`, and `WalkInputScale`; defaults are `true`, `300`, and `0.5`.

Pose-tuning workflow:

1. Open `Map_FPV_PoseLab` and start PIE.
2. Confirm the pistol appears automatically and the view moves back and forth while crouched.
3. Use the six labeled rows: translation X/Y/Z (`-75` to `75`), then rotation pitch/yaw/roll (`-45` to `45` degrees). Each row places its label to the left of a compact, half-width thick dark track with a green handle. The heading updates continuously with the exact vector and rotator values; hovering a slider shows its axis and range. Yaw starts at `6`; the other sliders start at `0`. Press `Reset` to restore translation `(0,0,0)` and rotation `(0,6,0)` while PIE remains active.
4. Test the values across the full movement cycle, then look down and test aim/reload separately. Slider changes update the player and Animation Blueprint live.
5. Record the selected values before stopping PIE; runtime slider changes are intentionally transient. Copy the final vector and rotator into `FirstPersonHandPoseTranslation` and `FirstPersonHandPoseRotation` under `Camera|First Person` in `BP_Resident_HorrorV1` Class Defaults.

PIE validation confirmed `bIsFirstPerson=true`, `bIsCrouched=true`, an equipped runtime `BP_Pistol`, the default `(Pitch=0, Yaw=6, Roll=0)` reaching `Hand Cliping Rotation`, and repeated forward/back lane traversal. The template's pre-existing unmatched-switch warning still appears at PIE startup; no pose-lab compile or runtime error was introduced.

Live lab trials at `(Pitch=-4, Yaw=12, Roll=0)` and `(Pitch=-8, Yaw=20, Roll=0)` motivated the added translation channel. The initial slider implementation propagated values correctly but placed its two Modify Bone nodes before `Blend Poses by Bool (Using Weapon?)`; an equipped pistol selected the alternate pose and bypassed both nodes. The nodes now run after the weapon/flashlight selections. PIE verification at the Y-axis extremes moved the weapon completely out of view near `-73` and brought the pistol plus both hands clearly into view near `+73`, confirming that the corrected path responds across the full `-75..75` range.

## Interaction framework

Primary assets:

- Component: `/Game/ResidentHorrorV1/Blueprints/BP_Interact/BPC_Interact`
- Interface: `/Game/ResidentHorrorV1/Blueprints/BP_Interact/BPI_Interact`
- Reusable base: `/Game/ResidentHorrorV1/Blueprints/BP_Interact/BP_Master_Interact/BP_Master_Interact`
- Prompt widget: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Interact/WBP_Interact`

The framework appears intended to provide a common player-to-world interaction path for pickups, containers, doors, save points, ladders, teleporters, and other usable actors. The custom `Interact` collision channel is a likely dependency.

When adding an interactable, extend the existing base or interface unless its contract demonstrably cannot support the new behavior. Verify prompt visibility, range/trace behavior, repeated use, and behavior while inventory or pause UI is open.

## Inventory, items, and combinations

Primary assets:

- Inventory component: `/Game/ResidentHorrorV1/Blueprints/BP_Inventory/BPC_Inventory`
- Item definition base: `/Game/ResidentHorrorV1/Blueprints/BP_Inventory/DA_BaseInteract`
- Item and slot structures: `/Game/ResidentHorrorV1/Blueprints/BP_Inventory/S_Item_Structure` and `/Game/ResidentHorrorV1/Blueprints/BP_Inventory/S_SlotStruct`
- Inventory window: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Inventory/WBP_InventoryWindow`
- Slot widget: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Inventory/WBP_InventorySlot`
- Action widget: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Inventory/WBP_InventoryActions`
- Description widget: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Inventory/WBP_ItemDescription`
- Inspection UI: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Inventory/WBP_InspectUI`

Asset-backed capabilities include:

- slot-based storage and a drag operation through `BP_Invent_Drag` and `WBP_DragUi`;
- item descriptions and context actions;
- pickup notification and add-slot notification widgets;
- normal and stored containers based on `BP_MasterContainer`;
- an inventory-space item through `BP_SpaceInventory` and `DA_MaleSpaceInventory`;
- item inspection through `BP_Base_Inspect` and `WBP_InspectUI`;
- data assets for herbs, first-aid spray, ladder, containers, keys, weapons, and ammunition;
- pistol-part combination assets `BP_BUTT`, `BP_Gun_handle`, `DA_Butt`, and `DA_Gun_Handle`.

Do not assume the exact combination recipes, stack limits, slot expansion rules, or drop behavior from asset names alone. Read the inventory component and item data assets before changing those rules.

Validation path:

1. Pick up several different item types and confirm slot allocation and notifications.
2. Drag or rearrange items and exercise every offered context action.
3. Inspect and drop an item, then recover it if supported.
4. Test normal/stored containers and inventory-space expansion.
5. Test a valid and invalid combination attempt.
6. Save, reload, and confirm inventory, quantities, equipment, and container state as applicable.

## Weapons and combat

Core contracts:

- Weapon base: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/BP_Base_Weapon`
- Pickup base: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/BP_Base_PickUpWeapon`
- Weapon component: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/Structure/BPC_Base_Weapon`
- Weapon interfaces: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/Structure/BPI_Weapon` and `BPI_DamageWeapon`
- Projectile: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/BP_BaseWeapon/BP_ProjectTile/BP_Projecttile`

Weapon implementations:

- Pistol: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/PickUp/HandGun/BP_Pistol`
- Tactical short shotgun: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/PickUp/Shotgun/BP_ShotgunTacticalShort`
- 12-pump shotgun: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/PickUp/2PumpShotgun/BP_12PumpShotgun`
- AS VAL: `/Game/ResidentHorrorV1/Blueprints/BP_Weapon/PickUp/Aks74/BP_AS_VAL`

Matching inventory data assets and ammunition pickups exist for the pistol, both shotgun naming variants, and AS VAL. Supporting assets include physical and standard magazines, pistol/shotgun casings, weapon camera shakes, laser and flashlight actors, and weapon-specific sound/FX customization data.

The animation library includes pistol, shotgun, and rifle/AS VAL aiming, fire, and reload sequences or montages. The presence of both generic weapon animations and character weapon animations means montage ownership and skeleton compatibility must be checked before replacing either set.

Validation path:

1. Pick up and equip each weapon independently.
2. Confirm aiming/zoom, fire rate, hit or projectile behavior, damage, recoil/camera shake, casing or magazine effects, and empty-ammo behavior.
3. Verify reload from partial and empty states, including ammo accounting between weapon and inventory.
4. Switch, drop, recover, and inspect weapons where supported.
5. Save/load while armed and confirm equipped weapon, magazine, reserve ammunition, and animation state.

## Health, healing, damage, and death

Primary assets:

- Health component: `/Game/ResidentHorrorV1/Blueprints/BP_HealthComponent/HealthComponent`
- Test actors: `/Game/ResidentHorrorV1/Blueprints/BP_HealthComponent/Debug/BP_Damage`, `BP_Healing`, and `BP_debughealthComponent`
- Healing item base: `/Game/ResidentHorrorV1/Blueprints/BP_Items/Herb/BP_BaseHealth`
- Green, red, and yellow herb assets under `/Game/ResidentHorrorV1/Blueprints/BP_Items/Herb`
- First-aid spray: `/Game/ResidentHorrorV1/Blueprints/BP_Items/Spray/BP_Spray`
- Death UI: `/Game/ResidentHorrorV1/Blueprints/WBP/WBP_Player/WBP_ScreenDied`

The asset set indicates reusable health, damage, healing, maximum-health UI, healing consumables, and a death flow. Herb colors may represent different effects or combination rules, but those effects must be confirmed from Blueprint graphs and data assets.

Verify damage clamping, healing at maximum health, item consumption, repeated damage, enemy and weapon damage paths, death animation/UI, input state after death, and restart/load behavior.

## Doors, locks, and keys

Primary assets:

- Door interface: `/Game/ResidentHorrorV1/Blueprints/BP_Door/Structure/BPI_Door`
- Door state enum: `/Game/ResidentHorrorV1/Blueprints/BP_Door/Structure/E_EnumDoorState`
- Door data base: `/Game/ResidentHorrorV1/Blueprints/BP_Door/Structure/DA_BaseDoor`
- Wood and metal implementations under `/Game/ResidentHorrorV1/Blueprints/BP_Door/BaseDoor`
- Key actors under `/Game/ResidentHorrorV1/Blueprints/BP_Door/Key`
- Key data assets: `/Game/ResidentHorrorV1/Blueprints/BP_Inventory/Data_Assets/Items/DoorDataAsset/Key/DA_KeyWoodDoor` and `DA_KeyMetalDoor`

Door customization data assets cover key behavior and character/factory collision for wood and metal variants. Player door animations include right-side opening and left/right push montages.

Verify unlocked use, locked feedback, wrong key, correct key, key consumption policy, approach direction, repeated use, collision during animation, interruption, and saved door state.

## Enemies and AI

Primary assets:

- Enemy base: `/Game/ResidentHorrorV1/Blueprints/BP_MasterAI/BP_MasterEnemy`
- AI controller: `/Game/ResidentHorrorV1/Blueprints/BP_MasterAI/AI/Structure/AI_MasterAI`
- Behavior Tree: `/Game/ResidentHorrorV1/Blueprints/BP_MasterAI/AI/Structure/BT_MasterAI`
- Blackboard: `/Game/ResidentHorrorV1/Blueprints/BP_MasterAI/AI/Structure/BB_MasterAI`
- Patrol route: `/Game/ResidentHorrorV1/Blueprints/BP_MasterAI/AI/Structure/BP_PatrolRoute`
- AI settings: `/Game/ResidentHorrorV1/Blueprints/BP_MasterAI/AI/Structure/S_AISettings`

The AI package includes enums for state, sense, and movement speed; patrol-route and enemy-retrieval interfaces; and Behavior Tree tasks for passive state, random locations, patrol movement, speed changes, focus/clear focus, default attack, local damage animation, death, and patrol-route checks. Example zombie Blueprints and mannequin animation assets provide test implementations.

The current map also contains three external actors whose missing base class is `/Game/WWG_ZombieLite/Blueprints/BP_ZombieLite`. Do not count those unknown actors as working enemies. Restore the vendor dependency or deliberately replace/remove the actors before treating the mechanic map as a complete AI demonstration.

Validation path:

1. Confirm a known existing example enemy possesses with `AI_MasterAI` and starts its Behavior Tree.
2. Verify navigation and patrol-route behavior with rebuilt navigation if required.
3. Test sight/sense acquisition, focus, chase speed, losing the target, return/passive behavior, and alert propagation if present.
4. Test attack timing and player damage, enemy hit reactions, repeated damage, death, collision after death, and save/load state.
5. Inspect Output Log and Behavior Tree debugging for failing tasks or missing Blackboard keys.

## Save and load

Primary assets:

- Save component: `/Game/ResidentHorrorV1/Blueprints/BP_SaveLoad/Blueprints/SaveLoadComponent`
- Save interface: `/Game/ResidentHorrorV1/Blueprints/BP_SaveLoad/Blueprints/BPI_SavemResidentHorror`
- Game save classes: `BP_SaveGameResidentHorrorV1`, `BP_SaveGameResidentHorrorGlobal`, and `BP_SaveGameActo`
- Structures: `/Game/ResidentHorrorV1/Blueprints/BP_SaveLoad/Structure/S_SaveLoad` and `S_LoadSlots`
- Widgets: `WBP_SaveGameSlots`, `WBP_SaveIngame`, `WBP_SaveLoadMenu`, and `WBP_SavingAlert`

The split between main, global, and actor save classes suggests multiple persistence scopes. Confirm the actual contract before adding fields: which objects implement the save interface, how actor identity is established, whether deleted/picked-up actors remain absent, and which settings or progression values are global.

Minimum validation should cover creating multiple slots, overwriting, loading from the in-game and menu flows, invalid/empty slots, inventory and health restoration, equipped weapon/ammo, door/container/pickup state, player transform, enemy state as intended, and graceful behavior when a save is from an older schema.

## HUD, pause, settings, and main menu

Player-facing widgets under `/Game/ResidentHorrorV1/Blueprints/WBP` include:

- interaction prompt and maximum-health display;
- HUD and item notifications;
- inventory window, slots, drag UI, actions, descriptions, and inspection;
- pause menu, general menu, configuration/settings, instructions, credits, death screen, and teleport warnings;
- save/load widgets described above.

The main-menu system under `/Game/ResidentHorrorV1/Blueprints/BP_MainMenu` includes menu objects, a menu camera, menu-specific game mode and player controller, graphical-setting enum, and settings SaveGame. Its map is `/Game/ResidentHorrorV1/Maps/Main_Menu/Main_Menu`, but the project currently starts in `Map_MechanicMap` rather than the main menu.

Verify keyboard and mouse focus, controller navigation if intended, pause/resume, settings application and persistence, resolution/fullscreen behavior, return-to-menu flow, new/load game routing, credits/instructions, and absence of gameplay input while modal UI is active.

## Animation coverage

The main player Animation Blueprint is `/Game/ResidentHorrorV1/Demo/ABP_Character/ABP_Player`. Supporting blend spaces and sequences cover forward/backward unarmed locomotion, crouched movement, leaning, airborne lean, directional stopping, walking, and running.

Dedicated animation groups exist for:

- door opening and pushing;
- ladder entry, exit, idle, up, and down, with left/right-hand variants;
- pistol, shotgun, and rifle aiming, fire, and reload;
- flashlight idle and directional additive poses;
- character/enemy attacks, hit reactions, death, idle, walk, and run;
- Manny, Quinn, and UE4 mannequin AI examples and retargeting assets.

Animation validation must include graph compilation, skeleton compatibility, montage slots, notifies, root motion policy, locomotion transitions, upper/lower-body blending, weapon alignment, door and ladder synchronization, hit/death interruption, and recovery from UI or weapon state changes. Several Manny pose assets are already known to be out of date with their source animations after conversion.

### Animation architecture decision

Decision recorded on 2026-08-20: retain and stabilize the current Animation Blueprint architecture for the first playable version rather than immediately replacing it with GASP Motion Matching.

`ABP_Player` is a conventional, hand-authored system built from nested state machines, Blend Spaces, an Aim Offset, cached poses, additive poses, layered bone blending, montage slots, Modify Bone nodes, root rotation, and `/Game/ResidentHorrorV1/Demo/Mannequins/Rigs/CR_Mannequin_AdvancedFootIK`. Its state hierarchy covers grounded movement, idle, stopping, crouching, turn-in-place, jump, fall, and land. Blueprint functions calculate gait, direction, play rate, start position, aim offset, lean, turn-in-place, hand sway, flashlight offset, and weapon pose.

The `GaspAnimation` folder and the asset named `AS_ALS_StanceVariation_Injured1` are animation-content sources, not evidence that the playable character uses the GASP Motion Matching or ALS frameworks. The active player Animation Blueprint currently has no Motion Matching node, Pose Search database, or Chooser-driven locomotion graph.

This architecture is considered sufficient for deliberate survival-horror movement and the current pistol, shotgun, rifle, flashlight, door, ladder, damage, and death flows. It is also easier to tune deterministically than a new Motion Matching implementation. Preserve it until a demonstrated gameplay or animation-quality requirement justifies replacement.

Expansion rules:

- Keep gameplay authority in the character, components, and gameplay systems. The Animation Blueprint should consume state and produce poses rather than own inventory, combat, or interaction decisions.
- Prefer the existing state enums, montage slots, additive layers, and data-driven weapon state when adding a small variation.
- Verify skeleton compatibility, montage slots, notifies, blend behavior, root motion, and interruption/recovery for every new animation.
- Do not add unrelated gameplay branches directly to the top-level AnimGraph when a function, montage, linked layer, or focused sub-state machine can own them.
- Maintain one clear pose pipeline: base locomotion, weapon/traversal/damage layers, montage and additive overlays, then foot IK/final-pose processing.

Refactor `ABP_Player` incrementally into Linked Anim Layers or smaller linked Animation Blueprints when one or more of these triggers occurs:

- adding a weapon requires duplicating substantial locomotion or transition logic;
- traversal, injury, combat, or equipment logic begins to cross several unrelated state machines;
- changes routinely break another animation mode or make compile/debug cycles difficult;
- a second playable character needs to share locomotion while providing different upper-body or traversal behavior;
- automated inspection can no longer identify the owner of a final-pose contribution reliably.

Likely future layer boundaries are base locomotion, weapon/aiming poses, upper-body actions, traversal/doors, damage/injury/death, flashlight/lean additives, and foot IK/post-processing.

Reconsider Motion Matching only if the game requires higher-fidelity omnidirectional locomotion, owns a sufficiently large and consistent animation library, and can absorb the Pose Search, Chooser, plugin, tuning, and migration costs. Treat that as a separate prototype and architecture decision, not an incremental edit to the production AnimGraph.

## Audio, footsteps, and atmosphere

Primary systems:

- Footsteps: `/Game/ResidentHorrorV1/Blueprints/BP_Footstep/BP_FootstepResidentHorror`
- Music manager: `/Game/ResidentHorrorV1/Blueprints/BP_MusicAmbient/BP_MusicManager`
- Ambient/combat triggers: `BP_MusicTrigger_AMBIENT` and `BP_MusicTrggerCOMBAT`
- Dialogue and world effects: `/Game/ResidentHorrorV1/Blueprints/BP_SoundWorld/BP_DialogueGameplay` and `BP_SoundEffectsWorld`
- Light flicker: `/Game/ResidentHorrorV1/Blueprints/BP_LightFlicker/BP_LightFlicker`

Physical materials exist for metal, water, `Concret`, grass, blood, and wood. The spelling `Concret` matches the configured physical surface and must remain consistent unless every reference is deliberately migrated.

Audio content is organized into character, weapon, casing, UI, voice, metal, wood, slush, cue, and attenuation groups. Validate footsteps against every physical surface, walk/run cadence, weapon and casing spatialization, dialogue triggering, overlapping ambient/combat zones, transition and loop behavior, pause/settings volume control, and light/audio atmosphere after save/load.

## Traversal and supporting utilities

- Ladder behavior: `/Game/ResidentHorrorV1/Blueprints/BP_Ladder/BP_Zone_LoopLadder`, with action/zone enums and the ladder montage library.
- Teleportation: `/Game/ResidentHorrorV1/Blueprints/BP_Teleport/BP_Teleport`, plus teleport and warning widgets.
- Flashlight: `/Game/ResidentHorrorV1/Blueprints/BP_FlashLight/BP_Flashlight` and failure variant `BP_Fail_Flashlight`.
- Item inspection: `/Game/ResidentHorrorV1/Blueprints/BP_InspectItem/BP_Base_Inspect`.
- Laser: `/Game/ResidentHorrorV1/Blueprints/Bp_Laser/BP_Laser`.
- Shared character helpers: `/Game/ResidentHorrorV1/Blueprints/BP_FunctionLibray/BPFL_LibrayCharacter`.
- External-link utility: `/Game/ResidentHorrorV1/Blueprints/BP_Url/BP_OpenLink` and `DA_OpenURL`.

Treat `BP_OpenLink` as menu/store utility behavior, not a core gameplay dependency. Validate and constrain destination URLs before shipping.

## Data-driven customization

Customization assets under `/Game/ResidentHorrorV1/Blueprints/BP_CustomDataAssets` cover character, inventory, menu, doors and collision, ladders, weapon FX/SFX, and footstep SFX. Prefer editing or extending these data assets when the intended variation is content or tuning rather than behavior.

Before adding a new parallel system, check whether the relevant base data asset already exposes the needed configuration. Document any new required field, owner, default value, and migration behavior.

## Known gaps and verification priorities

The following issues affect interpretation of the template's advertised capabilities:

- The configured `GlobalDefaultGameMode` references an absent `BP_PlayerMode`; the likely replacement `BP_PlayerModeResidentHorror` exists but must be confirmed in PIE before changing shared config.
- Three map actors depend on the missing `WWG_ZombieLite` content and are not valid evidence of a working zombie system.
- The Recast navmesh is recreated on UE 5.8 load because its serialized tile count differs; AI results are unreliable until navigation is verified or rebuilt.
- Several pose assets report stale source-animation data.
- Asset presence does not establish that the main mechanic map contains a demonstration instance or that menu/save routing reaches it.

Recommended validation order:

1. Player spawn, movement, camera, interaction prompt, inventory, pause, and clean PIE exit.
2. Pickup, inventory manipulation, inspection, containers, combination, and drop/recovery.
3. Health, healing, damage, death, and restart/load.
4. Each weapon's pickup, equip, aim, fire, reload, damage, ammo, drop, and save/load behavior.
5. Wood/metal doors with no key, wrong key, correct key, animation/collision, and persistence.
6. Existing example AI patrol, detection, attack, damage reaction, death, navigation, and persistence.
7. Save slots and restoration of player plus world state.
8. Main menu, settings, pause, credits/instructions, and input focus.
9. Ladder, teleport, flashlight, footsteps, music transitions, dialogue, and atmosphere.
10. Package and launch a Development build to catch editor-only assumptions after the core flows pass in PIE.

As each flow is verified, update the summary status and record the map, actor or asset, exact steps, expected result, UE version, and any warnings observed.
