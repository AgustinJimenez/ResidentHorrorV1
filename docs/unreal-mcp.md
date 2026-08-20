# Unreal MCP

## Purpose and current setup

This project uses Unreal Engine 5.8's experimental, engine-native Model Context Protocol integration so an MCP client such as Codex can inspect and operate the live Unreal Editor.

The current project setup is:

- `ModelContextProtocol` and `EditorToolset` are enabled in `ResidentHorrorV1.uproject`.
- The server auto-starts at `http://127.0.0.1:8000/mcp`.
- Tool search is enabled, so clients initially receive the `list_toolsets`, `describe_toolset`, and `call_tool` discovery tools.
- `.codex/config.toml` registers the server as `unreal-mcp` for this repository.
- `AllToolsets` is intentionally not enabled. It activates systems this template does not currently use, including `GameFeaturesToolset`, which produces a missing `GameFeatureData` Asset Manager configuration warning.

The connection was verified on 2026-08-20 by negotiating MCP protocol `2025-06-18`, listing the live toolsets, reading the Content Browser path, checking actor selection, and querying the PIE state.

## Dual-MCP development strategy

The long-term plan is to develop ResidentHorrorV1 with two complementary MCP integrations:

| Role | MCP | Endpoint | Intended use |
|---|---|---|---|
| Stable baseline and reference | Epic UE 5.8 MCP | `http://127.0.0.1:8000/mcp` | Routine editor inspection, common actor/asset/Blueprint operations, logs, viewport work, and PIE control |
| Extensible advanced layer | `AgustinJimenez/ClaudeUnrealMCP` | Node MCP bridge to TCP `127.0.0.1:9877` | Specialized Blueprint repair/migration, animation, FBX export, domain-specific operations, and new capabilities developed as the game needs them |

Epic's MCP should remain the default for operations it handles well. It is the supported UE 5.8 baseline, has the smaller maintenance burden, and provides useful behavior and tool-design references. Our MCP should be used when it offers a meaningfully better semantic operation or when the native MCP has a demonstrated gap.

The goal is not to duplicate every Epic tool. Improve our MCP where one of the following is true:

- the official MCP cannot perform a required game-development operation;
- a generic official operation is too fragile for a repeatable workflow;
- a typed, domain-specific tool can provide stronger validation and clearer results;
- the operation is useful across Unreal projects rather than only for a one-off asset edit.

When studying Epic's MCP, use its public tool schemas, observable behavior, documentation, and legally available engine/plugin source as design references. Preserve independent naming and implementation where appropriate, and do not copy code without confirming that its license permits the intended distribution.

### Our custom MCP

The custom MCP repository is [AgustinJimenez/ClaudeUnrealMCP](https://github.com/AgustinJimenez/ClaudeUnrealMCP). The validated UE 5.8 working copy is currently located outside this project at:

`E:/repo/unreal_engine/UE5MCPTest 5.8/Plugins/ClaudeUnrealMCP`

As of commit `9dcb623af8451b3026bb355a27712e0f8e98bb1a` on 2026-08-20:

- its Node bridge exposes 152 uniquely named MCP tools;
- its C++ editor plugin listens on loopback TCP port `9877`;
- UE 5.8 project compilation and a standalone non-unity `BuildPlugin` package build pass;
- live `ping`, engine-version, asset-existence, and FBX-export smoke tests pass;
- it includes deeper Blueprint graph, migration, component, animation, material, AI, PCG, GAS, networking, Sequencer, and editor-workflow operations than the currently enabled native toolset;
- it remains an actively developed internal tool, not a production-hardened replacement for Epic's integration.

The custom plugin is not yet installed in ResidentHorrorV1. Installing it will add a native C++ editor module and several engine-plugin dependencies to this currently Blueprint-only project. That integration must be performed as a focused change with an explicit source-management strategy; do not copy a dirty working directory into `Plugins/` or commit generated `Binaries/` and `Intermediate/` content.

Known custom-MCP maintenance concerns include:

- several tools originated in the `UE5MCPTest` locomotion project and may contain project-specific assumptions;
- its write surface includes destructive asset, graph, migration, and file-export operations;
- it depends on `StructUtils`, which UE 5.8 reports as deprecated, plus Chooser, StateTree, Niagara, Enhanced Input, Editor Scripting Utilities, and Procedural Mesh Component;
- it has build and targeted runtime smoke validation but no comprehensive automated regression suite;
- UE API deprecation warnings remain in migration, Niagara, and Sequencer code and should be resolved before a future engine upgrade makes them errors.

### Routing and attribution rules

Both servers may be configured simultaneously because they use distinct endpoints, but do not let them mutate the same editor state concurrently.

- Use Epic's MCP first for ordinary read operations and routine supported edits.
- Use our MCP for a specific advanced capability or while validating a tool under development.
- Choose one MCP as the owner of each mutation before making it. Record the server and tool name when a change is difficult to review through Git.
- Finish, verify, compile, and save one MCP-driven change before switching implementations on the same asset.
- Never issue overlapping save, rename, move, delete, Blueprint-graph, level-load, or PIE-control calls from both servers.
- Keep both endpoints loopback-only. Do not expose ports `8000` or `9877` to the LAN or Internet.
- If behavior differs, treat the Unreal Editor state, Output Log, compiled asset, and PIE result as the source of truth; then capture the discrepancy as a custom-MCP issue or regression test.

### Improvement loop

When game development exposes an MCP limitation:

1. Confirm the operation is absent from the currently enabled Epic toolsets and consider whether enabling one focused official toolset solves it.
2. Document a minimal reproducible use case using an actual ResidentHorror system, while avoiding hard-coded project asset paths in a generally reusable tool.
3. Prefer a narrow typed command with explicit inputs, dry/read behavior where practical, scoped output, and actionable errors.
4. Implement the capability in the separate `ClaudeUnrealMCP` repository and add its MCP schema at the same time as its Unreal handler.
5. Validate JavaScript syntax, unique tool registration, `git diff --check`, a UE 5.8 project build, and a standalone non-unity `BuildPlugin` build.
6. Run the least-destructive live editor smoke test possible. Use disposable assets or actors for mutation tests and remove only the exact validation artifacts afterward.
7. Commit and push the custom MCP independently before updating the version used by ResidentHorrorV1.
8. Document the new tool, its safety characteristics, and a reproducible validation path here when it becomes part of the project's workflow.

## Available capabilities

The enabled `EditorToolset` provides tools for:

- editor state, viewport captures, selection, Content Browser navigation, and PIE start/stop;
- Output Log inspection and log-category verbosity;
- actor discovery, transforms, components, hierarchy, tags, and labels;
- level loading, actor placement/removal, folders, collision queries, and level instances;
- asset search, dependencies, referencers, metadata, duplication, movement, deletion, and saving;
- Blueprint creation, graph inspection/editing, variables, functions, events, nodes, compilation, and defaults;
- generic Unreal object/class reflection and property editing;
- materials, material instances, textures, static meshes, skeletal meshes, data assets, DataTables, CurveTables, and StringTables;
- sandboxed batching of registered tool calls through `ProgrammaticToolset`.

## Limitations for this template

- Unreal MCP is experimental in UE 5.8. APIs and data formats may change, and some editor functionality is incomplete.
- The editor must be open and the MCP server running. Restarting Unreal invalidates existing sessions and may require the MCP client to reconnect.
- Tool calls execute serially on Unreal's game thread. Large searches or batches can temporarily stall the editor.
- The server is unauthenticated and intended only for loopback access. Never expose port `8000` to another machine or a public network.
- Shipping toolsets advertise Tools only; they do not advertise MCP Resources or Prompts.
- Only the core `EditorToolset` family is enabled. There are no dedicated AI/Behavior Tree, UMG, Niagara, animation/Control Rig, audio, Game Features, or automation-test tools. Generic asset and Blueprint inspection may offer partial access, but not the same semantic editing support.
- MCP can start and stop PIE, but the enabled tools do not inject player keyboard or mouse input. Combat, inventory, interaction, UI navigation, and save/load flows still require manual playtesting or purpose-built automation.
- World Partition actor searches operate on the current editor scene. Actors in unloaded cells may not be visible until their cells are loaded.
- Missing vendor content cannot be recovered through MCP. In particular, the absent `/Game/WWG_ZombieLite/Blueprints/BP_ZombieLite` class must be restored from its source or its unknown actors deliberately removed.
- The project has thousands of assets and external actors. Scope searches to `/Game/ResidentHorrorV1/...` whenever possible instead of scanning all project and engine content.
- Visual captures help with spatial verification but do not replace human review of animation quality, audio, atmosphere, interaction feel, or final presentation.

## Safe operating rules

- Treat read-only inspection as the default. State the intended asset or actor changes before using mutating tools.
- Do not delete, move, rename, or broadly save assets without explicit confirmation of the exact targets.
- Before editing a Blueprint graph, read the existing graph and retain enough information to verify the change. `write_graph_dsl` populates the target graph and can cause broad changes.
- Compile every modified Blueprint, inspect compile results and the Output Log, and save only the intended assets.
- Never manually edit `Content/__ExternalActors__` or `Content/__ExternalObjects__`. Modify World Partition actors through Unreal and review all resulting Git changes.
- Do not pass an empty list to `save_assets` unless intentionally saving every dirty asset in the editor.
- Prefer a focused branch or checkpoint before material Blueprint, map, or bulk-asset changes. Unreal binary assets do not produce useful line-by-line Git diffs.
- After a mutation, verify the result using a separate read operation, targeted viewport capture, Blueprint compile, or PIE test.
- Stop PIE before persistent editor or asset modifications unless the operation specifically requires a running play session.

## Validation and troubleshooting

Use these checks in order:

1. Confirm Unreal Editor is open on this project.
2. In **Edit > Editor Preferences > Model Context Protocol**, confirm **Auto Start Server** is enabled and the endpoint is port `8000` with path `/mcp`.
3. In the Output Log, look for `LogModelContextProtocol` startup or binding errors.
4. From the Unreal console, run `ModelContextProtocol.StartServer 8000` if the server was not started.
5. Run `ModelContextProtocol.RefreshTools` after enabling or changing toolsets.
6. Confirm `.codex/config.toml` contains the `unreal-mcp` URL and reconnect or restart the local MCP client after an editor restart when necessary.
7. Call `list_toolsets`; the result should include `EditorToolset.EditorAppToolset`, `ActorTools`, `AssetTools`, `BlueprintTools`, `ObjectTools`, and `SceneTools`.
8. Use a harmless read call such as `IsPIERunning`, `GetContentBrowserPath`, or `GetSelectedActors` as the final smoke test.

For protocol-level debugging, Epic recommends the MCP Inspector against `http://127.0.0.1:8000/mcp` using Streamable HTTP.

### Visible editor automation fallback

The Windows Computer Use bridge was verified operational with the live UE 5.8 editor on 2026-08-20. It can target the main `ResidentHorrorV1 - Unreal Editor` window and Blueprint editor child windows. Use it only for narrow, visible editor operations when the native MCP reader cannot traverse a collapsed/composite graph or when an asset editor workflow is safer than broad graph serialization.

Do not use UI automation for terminal commands, broad asset saves, binary-file manipulation, or unattended destructive operations. Keep MCP or Unreal-native APIs responsible for structured inspection, compilation, verification, and precisely scoped saves.

## Adding more toolsets

Enable specialized toolsets individually when a real task requires them. Likely candidates for this template are `AIModuleToolset`, `UMGToolSet`, `NiagaraToolsets`, `AnimationAssistantToolset`, and `AutomationTestToolset`.

Evaluate each plugin's dependencies and startup warnings before keeping it enabled. Avoid enabling `AllToolsets` merely for convenience; it broadens the editor surface, adds irrelevant dependencies, and makes the available tool catalog harder to control.

## Alternative MCP implementations investigated

The following alternatives were reviewed online on 2026-08-20. Capability counts are claims made by their maintainers and have not been independently validated against this project.

### Monolith

[Monolith](https://github.com/tumourlove/monolith) is the broadest implementation found. Its maintainers report approximately 1,400 actions across more than 25 namespaces. Coverage includes Blueprints, AnimBlueprints, Behavior Trees, State Trees, EQS, navigation, AI perception, UMG/CommonUI, animation, Control Rig, IK retargeting, Motion Matching, Niagara, materials, meshes, audio/MetaSounds, Gameplay Ability System, Level Sequences, PIE input injection, profiling, navigation validation, and runtime inspection. It also provides offline engine-source and project-asset indexing.

Monolith supports UE 5.7 and 5.8 from one source tree and publishes version-specific precompiled builds suitable for Blueprint-only projects. Its namespace-dispatch design exposes a small number of MCP tools and discovers actions on demand instead of advertising every action schema at once.

Tradeoffs and risks:

- Its plugin manifest enables a large set of Unreal dependencies, including Niagara, GAS, StateTree, SmartObjects, CommonUI, Control Rig, Pose Search, IKRig, Python, and other optional systems.
- The added modules and dependencies may affect editor startup, project configuration, packaging, and migration behavior.
- Precompiled binaries are engine-version-locked; only the exact UE 5.8 release build should be tested here.
- Its HTTP server uses port `9316` and binds to all network interfaces. CORS does not prevent direct LAN requests. The project must add an inbound Windows Firewall block or disable the server when it is not in use.
- The auto-updater should remain disabled. Download releases manually and verify the release SHA-256 marker before extraction.
- Its much larger write surface increases the consequences of an incorrect tool call.

References: [Monolith overview](https://github.com/tumourlove/monolith), [plugin manifest](https://raw.githubusercontent.com/tumourlove/monolith/master/Monolith.uplugin), and [security policy](https://raw.githubusercontent.com/tumourlove/monolith/master/SECURITY.md).

### GenOrca Unreal MCP

[GenOrca Unreal MCP](https://github.com/GenOrca/unreal-mcp) is the strongest middle-ground alternative found. Its maintainers report 253 actions across 21 domain tools. It provides dedicated Behavior Tree/Blackboard, UMG, animation, Control Rig, retargeting, AnimBlueprint, Level Sequence, material, Blueprint graph, Enhanced Input, and viewport tools. It also exposes arbitrary Unreal Python as an escape hatch.

Tradeoffs and risks:

- It requires Python 3.11, `uv`, the Python Editor Script Plugin, and a separate Python MCP server.
- Arbitrary Python is flexible but less constrained than dedicated, typed tools.
- Precompiled plugin releases are engine-specific. Installing a source archive or a binary built for a different Unreal version can trigger a failed rebuild and prevent the Blueprint-only project from opening normally.
- It introduces another server and tool protocol to configure, troubleshoot, and keep synchronized with the project.

Reference: [GenOrca Unreal MCP documentation](https://github.com/GenOrca/unreal-mcp).

### Unreal MCP Toolkit

[Unreal MCP Toolkit](https://github.com/timargv/UnrealMCPToolkit) extends Epic's native UE 5.8 MCP endpoint rather than replacing it. It adds unrestricted `execute_python`, `execute_console_command`, screenshots, asset listing, Output Log access, and save-all helpers. A version-specific precompiled UE 5.8 build is available for Blueprint-only projects.

This toolkit is an escape hatch rather than a comprehensive semantic tool suite. It can reach most editor APIs through Python, but unrestricted code execution has fewer schema-level guardrails, makes intent harder to review, and can perform broad changes very quickly.

Reference: [Unreal MCP Toolkit documentation](https://github.com/timargv/UnrealMCPToolkit).

### Other reviewed projects

Other projects such as `RonildoBraga/unreal-mcp` and `DeVoe09/UnrealMCP` report roughly 100 editor commands. They were not selected as leading candidates because the alternatives above offer broader documented coverage, a more suitable native-extension approach, or a more mature release path for UE 5.8.

## MCP adoption recommendation

Use the following order when expanding automation:

1. Keep Epic's native `ModelContextProtocol` plus `EditorToolset` as the stable baseline and default MCP.
2. Integrate our `ClaudeUnrealMCP` as the controlled advanced layer after deciding how its source and builds will be versioned in this repository.
3. Enable individual Epic toolsets only when a task needs them, particularly AI, UMG, Niagara, animation, or automation testing.
4. Improve our MCP when a real game-development gap remains, using Epic's implementation as a reference and the validation loop above.
5. If both our MCP and native toolsets remain a demonstrated blocker, evaluate Monolith on a separate Git branch as the maximum-capability option.
6. Prefer GenOrca when dedicated AI/UMG/animation coverage is needed but Monolith's dependency surface is unacceptable and maintaining Python plus `uv` is reasonable.
7. Use Unreal MCP Toolkit only when a specific operation requires unrestricted Python or console execution and the risk is justified.
8. Do not add another overlapping write-capable MCP casually. The intentional Epic-plus-custom pairing follows the routing rules above; additional servers would make mutations and saves harder to attribute.

Before adopting any third-party MCP:

- create a dedicated test branch;
- close Unreal and preserve the current working baseline;
- use the exact UE 5.8 precompiled release where available;
- review its `.uplugin`, dependencies, license, security policy, and release checksum;
- verify the editor opens and the template still completes its baseline PIE smoke test;
- inspect all generated configuration and Git changes;
- test packaging before accepting the plugin into the main development branch;
- document its endpoint, tool policy, update procedure, firewall requirements, and removal procedure here.

Reference: [Epic Games — Unreal MCP in Unreal Editor](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor)
