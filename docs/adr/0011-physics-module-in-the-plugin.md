# 0011: The physics module ships in the plugin

- **Status:** Accepted (maintainer decision 2026-10-27)
- **Date:** 2026-10-27
- **Related:** [ADR 0007](0007-physics-driven-modules.md) (Tier 2, decision item 2), [ADR 0010](0010-control-rig-adapter.md) (the Control Rig module, packaged the same way), roadmap WP-15

## Context

ADR 0007 puts the Tier 2 physics hand-off in a separate runtime module, `Open3DPuppetPhysics`,
which depends on the Physics Control plugin and is "enabled only when the project enables that
plugin (an optional plugin dependency in the `.uplugin`)". The intent was that a project using
Open3DPuppet does not get Physics Control unless it asks for it.

An optional plugin dependency does not do that. Both the build and the editor enable an optional
dependency whenever the plugin is installed, and skip it only when it is missing (UE 5.8.4):

- UnrealBuildTool: `Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:5878`
  returns no plugin for an optional reference only when the plugin is not found; otherwise it
  enables it like any other reference.
- The plugin manager: `Engine/Source/Runtime/Projects/Private/PluginManager.cpp:2476-2546` ignores
  an optional reference only when the plugin is not found, or, outside Shipping, when the target
  did not build it.

Physics Control ships with the engine (`Engine/Plugins/Animation/PhysicsControl/`, no longer
marked Beta in 5.8.4), so an optional dependency on it enables it in every project that enables
Open3DPuppet. A module in the same plugin cannot be loaded only when another plugin is enabled.

## Options considered

### Option A: the module in the plugin, with a plain dependency on Physics Control (chosen)
- Pros: one plugin and one Fab product; the same packaging as the Control Rig module (ADR 0010);
  CI, the plugin package and the Fab source zip need no new steps.
- Cons: Physics Control is enabled in every project that enables Open3DPuppet.

### Option B: a second plugin, `Open3DPuppetPhysics`
- Pros: truly opt-in; projects that do not enable it never load Physics Control.
- Cons: a second `.uplugin`, its own packaging, CI build and Fab listing (or a bundle).

### Option C: an optional dependency, as ADR 0007 wrote
- Behaves as Option A, while the `.uplugin` suggests otherwise.

## Decision

1. `Open3DPuppetPhysics` is a runtime module of the Open3DPuppet plugin, with the same platforms
   and target types as `Open3DPuppetControlRig`.
2. The `.uplugin` lists Physics Control as a plain (not optional) plugin dependency.
3. The core module `Open3DPuppet` still does not depend on Physics Control; only
   `Open3DPuppetPhysics` does. This replaces the packaging part of ADR 0007 decision item 2; the
   rest of that item (the Physics Region module, the function library, the setup guide) stands.

## Consequences

- Every project that enables Open3DPuppet also enables Physics Control. Its runtime module loads,
  but nothing simulates unless an AnimGraph uses Rigid Body With Control.
- If a later engine version gives modules a way to load only with another plugin, or Option B
  becomes worth its cost (for example a Fab buyer who must not load Physics Control), a new ADR
  can move the module out.
