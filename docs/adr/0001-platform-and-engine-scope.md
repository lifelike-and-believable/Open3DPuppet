# 0001: Platform and engine scope

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-24
- **Related:** Open3DBroadcast ADR 0001 (Win64) and ADR 0014 (engine versions), whose conventions this project follows

## Context

Open3DPuppet is a new plugin with no users yet. The maintainer set the scope on 2026-10-24: "same
conventions as Open3DBroadcast, can target Win64 UE5.8+, source-agnostic via Live Link".
Open3DBroadcast supports UE 5.7 and 5.8; this plugin starts at 5.8, so it can use 5.8 APIs without
version guards from day one.

## Decision drivers

1. Users on the current engine can install it (Fab).
2. One source tree, no per-engine branches.
3. The same toolchain and runner as Open3DBroadcast.

## Options considered

### Option A: UE 5.7 and 5.8, as Open3DBroadcast
- Pros: wider reach.
- Cons: version guards from the first line of code, for a plugin no one uses yet.

### Option B: UE 5.8 and later, Win64
- Pros: no guards now; matches the maintainer's direction.
- Cons: users who stay on 5.7 for a production cannot use it.

## Decision

1. **Engines:** UE 5.8 and later. When a new minor version ships, it is added (and, following
   Open3DBroadcast's policy, the oldest dropped once two are supported) in its own ADR, with
   per-engine Target.cs settings under `#if UE_5_<n>_OR_LATER` (rule OP-001).
2. **Platform:** Win64 only. Every module in the `.uplugin` has `"PlatformAllowList": [ "Win64" ]`
   (rule OP-005). Animation code is platform-neutral; the limit is what CI can test.
3. **Distribution:** the plugin is written to be Fab-ready (copyright headers, no dependency on
   plugins that do not ship with the engine, zero warnings). Fab packaging is WP-9.
4. **Target.cs:** `BuildSettingsVersion.V7` and `EngineIncludeOrderVersion.Unreal5_8`.

## Consequences

- No engine-version compatibility header is needed until a second engine version is supported.
- CI needs a Windows runner with UE 5.8 for the build and tests (WP-9); until then, plugin builds
  are verified at the desk.

## Open questions for the maintainer

1. Licence and copyright holder: the headers say "Lifelike & Believable. All Rights Reserved.",
   as in Open3DBroadcast. Confirm, and say whether the repository needs a `LICENSE` file.
2. Fab: is the plugin meant for Fab like Open3DBroadcast, or distributed another way?
