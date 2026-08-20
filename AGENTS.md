# ResidentHorrorV1 Contributor Guide

## Project overview

ResidentHorrorV1 is a freshly cloned, store-distributed survival-horror template. It is a Windows-targeted, Blueprint-only project with no `Source/` module. Gameplay is organized around a third-person player, interactions, inventory, weapons, doors, health, AI, save/load, and menu/UI systems.

The template was originally authored for Unreal Engine 5.7. This checkout was first opened and converted in Unreal Engine 5.8, and `ResidentHorrorV1.uproject` now has an `EngineAssociation` of `5.8`. Preserve the freshly cloned state as the baseline when deciding whether behavior is original template behavior or a local regression.

Treat this file as a living map of the repository. Update it when a system, startup map, important asset path, required plugin, or validation procedure changes.

## Getting started

- Open `ResidentHorrorV1.uproject` with Unreal Engine 5.8 for continued work on this converted checkout. Unreal Engine 5.7 is the template's original authoring version.
- The expected editor and game startup map is `/Game/ResidentHorrorV1/Maps/Map_MechanicMap`.
- `DefaultEngine.ini` currently points the default game mode at `/Game/ResidentHorrorV1/Character/BP_Character/Game/BP_PlayerMode`, but that asset is absent. The existing likely replacement is `BP_PlayerModeResidentHorror`; see Known Issues before changing the config.
- The configured game instance is `/Game/ResidentHorrorV1/Blueprints/BP_Core/ResidentHorrorV1_GameInstance`.
- Use Play In Editor from `Map_MechanicMap` for the primary smoke test.
- This repository uses Git LFS. Run `git lfs install` once per workstation and make sure LFS assets are fully pulled before opening the project.

## Repository layout

```text
Config/                         Project, engine, input, editor, and Niagara settings
Content/ResidentHorrorV1/       Template gameplay and presentation assets
  Audio/                        Music, ambience, dialogue, and sound effects
  Blueprints/                   Most gameplay systems and UI
  Character/                    Player Blueprint, game framework, and animations
  Demo/                         Environment, mannequin, mesh, material, and demo assets
  Input/                        Enhanced Input mapping context and input actions
  Maps/                         Gameplay and main-menu maps
  ParticleEffects/              Niagara/Cascade systems and supporting materials
  Textures/                     UI, item, weapon, environment, and effect textures
Content/__ExternalActors__/     World Partition actor packages; editor-managed
Content/__ExternalObjects__/    Editor-managed external object packages
docs/                           Project documentation, including Unreal MCP guidance
ResidentHorrorV1.uproject       Engine association and project plugin state
```

Generated folders (`Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, and `Build/`) are ignored and must not be committed.

## Gameplay architecture

The following map is based on current asset paths and names. Blueprint graphs remain the source of truth.

The detailed feature inventory, asset anchors, known gaps, and system-by-system PIE validation paths are maintained in `docs/gameplay-systems.md`. Update that document when a gameplay capability is added, removed, verified, or found to be incomplete.

- Player/game framework: `Character/BP_Character/BP_Resident_HorrorV1`, `BP_PlayerControllerResidentHorror`, `BP_PlayerModeResidentHorror`, and `ResidentHorrorV1_GameInstance`.
- Input: `Input/IMC_Player` and actions for movement, look, jump, sprint, crouch, lean, interact, inventory, fire, reload, zoom, flashlight, pause, and any-key handling.
- Interaction: `Blueprints/BP_Interact/BPC_Interact` and `BPI_Interact`, with `BP_Master_Interact` as the apparent reusable base.
- Inventory/items: `Blueprints/BP_Inventory`, item/weapon data assets, `Blueprints/BP_Items`, and `Blueprints/WBP/WBP_Inventory`.
- Weapons: `Blueprints/BP_Weapon`, including base weapon/pickup logic, ammo, projectiles, magazines, casings, pistol, shotgun variants, and AS VAL assets.
- Health: `Blueprints/BP_HealthComponent/HealthComponent`, healing/damage debug actors, herb items, and first-aid spray.
- Doors/keys: `Blueprints/BP_Door`, including wood and metal variants, door interfaces/state, and matching key data assets.
- AI: `Blueprints/BP_MasterAI`, with a master enemy, AI controller, behavior tree, blackboard, patrol route, state/sense enums, and behavior-tree tasks.
- Save/load: `Blueprints/BP_SaveLoad`, including save-game classes, component/interface, slot structures, and save/load widgets.
- UI/menu: `Blueprints/WBP` and `Blueprints/BP_MainMenu`. A separate map exists at `/Game/ResidentHorrorV1/Maps/Main_Menu/Main_Menu`, but it is not the configured default map.
- Supporting systems: flashlight, ladders, item inspection, footsteps by physical surface, ambient/combat music, world audio/dialogue, teleportation, camera shake, and light flicker.

Important data-driven customization lives under `Blueprints/BP_CustomDataAssets`. Prefer extending existing base Blueprints, interfaces, components, structures, and data assets instead of duplicating their logic.

## Engine and platform settings

- Target: Windows desktop, maximum graphics preset.
- Renderer: DirectX 11 / Shader Model 5.
- Lumen global illumination and reflections are enabled.
- Nanite and Virtual Shadow Maps are disabled.
- Motion blur and auto exposure are disabled.
- Custom collision channels are `Interact`, `WeaponTrace`, and `Enemy`.
- Footstep-related physical surfaces are `Metal`, `Water`, `Concret`, `Grass`, `Blood`, and `Wood`. `Concret` is the configured spelling; changing it may break asset references.
- Packaging is configured for Shipping, distribution, compressed Pak/IoStore output, English localization, and map-only cooking.
- The `.uproject` enables Unreal Engine 5.8's experimental `ModelContextProtocol` and `EditorToolset` plugins. Read `docs/unreal-mcp.md` before using or changing the integration.
- The `.uproject` explicitly disables WMFCodecs, ElectronicNodes, SkeletalMeshModelingTools, and Fab. Do not introduce a required plugin dependency without updating the project file and this guide.

## Unreal MCP

The repository-scoped MCP setup, planned Epic-plus-custom dual-MCP strategy, available tool families, project-specific limitations, safety rules, and troubleshooting procedure are documented in `docs/unreal-mcp.md`. Consult that file before using MCP to mutate Blueprints, assets, actors, or maps. Keep the document synchronized with `.codex/config.toml`, `ResidentHorrorV1.uproject`, the custom MCP revision when installed, and the enabled Unreal toolsets.

## Working safely with Unreal assets

- `.uasset` and `.umap` files are binary. Edit them in Unreal Editor, never as text.
- Move, rename, or delete assets in the Content Browser so Unreal can create and fix redirectors and references.
- Never manually edit or reorganize `__ExternalActors__` or `__ExternalObjects__`. They are managed with their World Partition maps.
- A change to `Map_MechanicMap` may legitimately modify many external actor files. Before committing, review the complete file list and confirm that every changed actor was intended.
- Avoid saving unrelated dirty assets when closing the editor. Keep commits scoped by system when practical.
- After moving assets, fix up redirectors in the affected Content Browser folder, save dependent assets, and verify references before committing.
- Do not commit files from ignored generated folders. Configuration changes that should be shared belong in `Config/`, not per-user files under `Saved/`.
- Keep binary assets under Git LFS. Before committing a new binary format, check `.gitattributes` and add an LFS rule if needed.

## Validation checklist

For Blueprint or asset changes:

1. Compile every modified Blueprint and resolve compile errors.
2. Save all intentionally changed assets.
3. Open `Map_MechanicMap` and check the Output Log for new errors or warnings.
4. Run Play In Editor and exercise the changed flow plus its immediate dependencies.
5. At minimum, verify player spawn, movement/look, interaction, inventory/pause UI, and exiting PIE cleanly for cross-cutting changes.
6. For AI changes, verify navigation, patrol/detection, attack/damage, and death behavior.
7. For inventory/weapon changes, verify pickup, slot behavior, equip/fire/reload, ammo accounting, drop/inspect, and save/load as applicable.
8. For map changes, inspect World Partition/external actor diffs and rebuild navigation or lighting only when the change requires it.
9. Review `git status --short` before committing; generated or unrelated saved assets should not appear.

There are no repository-native automated tests or C++ build targets at present. A clean Blueprint compile and targeted PIE session are therefore the baseline validation.

## Known issues observed on UE 5.8 startup

These warnings were observed during the first Unreal Engine 5.8 startup on 2026-08-20 and were not corrected during repository exploration. Because the template was authored for 5.7, separate version-conversion warnings from missing vendor content or pre-existing template issues before making changes:

- `Map_MechanicMap` has three external actors whose base class is missing: `/Game/WWG_ZombieLite/Blueprints/BP_ZombieLite`. They may appear as unknown actors until the missing content is restored or the actors are deliberately removed in the editor.
- `DefaultEngine.ini` references a missing `BP_PlayerMode` asset for `GlobalDefaultGameMode`; the repository instead contains `BP_PlayerModeResidentHorror` in the same folder. Confirm map overrides and intended behavior in PIE before correcting the shared setting.
- Several Manny pose assets under `Content/ResidentHorrorV1/Demo/Mannequins/Rigs/Poses` report being out of date with their source animations.
- The map's Recast navmesh reports a serialized `maxTiles` size mismatch and is recreated at load time. Rebuilding navigation in the intended engine version may be necessary.
- The config still identifies the project as `Third Person BP Game Template`, contains redirects to `/Script/Survival_Horror`, and has external actor remnants under older content roots. Treat these as migration history until references are audited.

When addressing any of these, make a focused change and re-open the project to confirm the warning is gone without introducing missing references.

## Documentation expectations

When adding or substantially changing a system, document:

- its main Blueprint, component, interface, and data-asset paths;
- where it is initialized or owned;
- the input, collision channel, gameplay tag, save data, and UI dependencies it introduces;
- a short, reproducible PIE validation path;
- any required engine/project setting or plugin.

Keep exact asset paths in backticks so they can be found quickly in the Unreal Content Browser.
