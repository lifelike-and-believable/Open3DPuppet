# 0005: Test layout

- **Status:** Accepted (maintainer sign-off 2026-10-24)
- **Date:** 2026-10-24
- **Related:** Open3DBroadcast ADR 0006, whose layout this follows; rules OP-003, OP-004

## Context

Puppet modules are mostly math on poses. That math can be tested without a running performer,
an animation blueprint or content assets, if the tests can build a skeleton and a performer pose
in code.

## Decision drivers

1. Every module in the library is tested, so authors can change the shared math with confidence.
2. Tests never ship in a packaged game.
3. Tests run headless in CI (`-nullrhi`), without content.

## Decision

1. **Module.** `Open3DPuppetTests`, type `Editor` in the `.uplugin`. Runtime modules never depend
   on it. Tests live in `Source/Open3DPuppetTests/Private/<Area>/`.
2. **Names.** `Open3DPuppet.<Area>.<Unit>.<Case>`, wrapped in `#if WITH_DEV_AUTOMATION_TESTS`,
   with flags `EditorContext | EngineFilter`.
3. **White-box access.** Through the owning module's `Public/Testing/*.h`, never `Private/`.
4. **Fixtures.** `PuppetTestSkeletons.h` in the test module builds reference skeletons (a
   mannequin-like biped, a long tail, a digitigrade leg) with `FReferenceSkeletonModifier` and no
   `USkeleton`, and builds performer poses from bone names, parents and transforms, the same data
   a Live Link Animation subject carries.
5. **What every module tests.** At least: the calibrated rest pose maps to the puppet's reference
   pose; a known performer rotation produces the expected puppet rotation; each retargeting
   control changes the result as documented; `Bind` reports a missing bone instead of crashing.
6. **Floor.** `Build/automation-test-floors.json` holds the minimum test count; a PR that adds
   tests raises it.

## Consequences

- AnimGraph node behaviour (Live Link evaluation, the cast) is tested through the solver core it
  calls, not through an animation blueprint; a live check with a performer is a desk item.
