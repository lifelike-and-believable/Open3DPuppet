# 0004: Several performers on one puppet

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-24
- **Related:** [ADR 0002](0002-puppet-module-architecture.md), [ADR 0003](0003-performer-input-and-calibration.md), [design overview](../design/overview.md) §6

## Context

Some puppets need more than one performer: a two-person quadruped, a dragon whose wings are
played by a second performer, a tail puppeteered with a tracked rod. Performers stand in
different places on the stage, so their world positions cannot both drive the puppet's root.

## Decision drivers

1. One puppet asset works for one performer or several without rebuilding it.
2. The puppet's world placement is never ambiguous.
3. A module does not need to know how many performers there are.

## Decision

1. **Slots.** `UPuppetAsset::PerformerSlots` names the roles (`Body` by default). Each module
   names the slot it reads in `UPuppetModule::PerformerSlot`.
2. **Cast.** The Puppet Solve node's `Cast` binds each slot to a Live Link subject and an
   optional performer profile. It is pinnable, so a Blueprint can recast at run time.
3. **Root ownership.** `UPuppetAsset::RootSlot` owns the root: only modules on that slot may write
   world-space position (the Root module). Modules on other slots use rotation deltas, or
   positions relative to their own performer's pelvis, mapped onto the puppet bone they hang
   from. The shared math helpers take the slot's own pose, so this holds without extra code in
   each module.
4. **Calibration per performer.** Each performer is calibrated on its own (ADR 0003), with its own
   alignment, so performers need not face the same way on the stage.

## Consequences

- A second performer is added by adding a slot and pointing modules at it.
- Contact between performers (a hand on the other performer's shoulder) is not preserved, because
  each slot is relative to its own pelvis. A module that needs it can read both slots; that is
  left until a puppet needs it.

## Open questions for the maintainer

1. Should a slot be able to fall back to another slot when its performer drops out (for example
   the tail following the body when the tail puppeteer stops)?
