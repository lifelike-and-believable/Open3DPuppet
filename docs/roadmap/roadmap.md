# Roadmap

Work packages (WPs) in the order they are planned. One WP per pull request, branched from
`develop` (AGENTS.md). The design is in [`docs/design/overview.md`](../design/overview.md).

| WP | Scope | Depends on | Status |
|---|---|---|---|
| WP-0 | Foundation: conventions, CI checks, design overview, ADRs 0001 to 0005, host project, plugin skeleton | | Done (#1) |
| WP-1 | Solver core (performer pose, calibration, puppet pose, retarget math), module contract, Root, Chain and Two-bone limb modules, Puppet Solve AnimGraph node reading Live Link, calibration subsystem, solver tests, module authoring guide | WP-0, ADRs 0002 to 0004 | Done (#2) |
| WP-9a | UE build and test CI on the shared self-hosted runner | WP-1 | Done (#3) |
| WP-11 | Curves in the pipeline (ADR 0006): performer curves, Basic-role (curve-only) subjects, neutral calibration of curves, puppet output curves, parent-relative joint deltas | WP-1, ADR 0006 | Planned |
| WP-12 | Digits module: fingers to toes, talons and feathers; derived finger-curl curves | WP-11 | Planned |
| WP-13 | Face modules: curve to bone, curve remap, eyes | WP-11 | Planned |
| WP-14 | Solver dynamics (ADR 0007, Tier 1): world motion in the solve context, `PuppetDynamics` (fixed-step integrator, spring-dampers, constrained chains, collision shapes), Physics Weight | WP-11 | Planned |
| WP-4 | Digitigrade leg module (Wolf front and hind legs) | WP-1 | Planned |
| WP-5 | Tail and dynamic chain module: springs, input mix (Wolf tail, ears, mane; Eagle tail fan, feathers) | WP-14 | Planned |
| WP-15 | Engine physics hand-off (ADR 0007, Tier 2): optional `Open3DPuppetPhysics` module, Physics Region module, Rigid Body With Control parameters from curves, setup guide. The Wolf and Eagle need physics assets first (none exist yet) | WP-11 | Planned |
| WP-6 | Wing module (Eagle) | WP-12 | Planned |
| WP-3 | Performer clips as a cast source (ADR 0008): AnimSequence playback (rate, loop, start, time pin), calibration from a frame, re-solving a recorded take | WP-1 | Planned |
| WP-16 | Hand-offs (ADR 0008): control blend per slot, `UPuppetPerformanceComponent` transitions with easing, root anchoring | WP-1 | Planned |
| WP-17 | Hand-off triggers: Sequencer cue, operator input (input action, Remote Control), performer gesture | WP-16 | Planned |
| WP-M1 | Wolf: puppet asset, profiles and calibration for the show's capture setup, run-in entrance and hand-off; live session at the desk | WP-3, WP-4, WP-5, WP-12, WP-13, WP-15, WP-16 | Planned |
| WP-M2 | Eagle: puppet asset, profiles and calibration, fly-in entrance and hand-off; live session at the desk | WP-3, WP-5, WP-6, WP-12, WP-13, WP-15, WP-16 | Planned |
| WP-2 | Calibration and profiles in the editor: calibrate button, save calibrations to assets, choice of reference pose, profile auto-mapping by name | WP-1 | Planned |
| WP-7 | Prop-driven module (Live Link Transform role) and a multi-performer sample | WP-1 | Planned |
| WP-8 | Setup assistant (finds chains, proposes modules), bone pickers bound to the target mesh, a session control panel | WP-1 | Planned |
| WP-9 | Fab readiness as Open3DBroadcast: packaging script (`fab-package.py`), exclusion lists, icon | WP-1 | Planned |
| WP-10 | Control Rig adapter module | WP-1 | Planned |

Work packages are listed in the order they are planned. The Wolf and Eagle (design overview §10)
set that order: curves, digits and face first, because both characters use them, then the
anatomy modules each needs, and the entrances (clip, then hand-off to the performer). WP-2 can move
ahead when a session needs it.

## Next

Items that need a person at the desk or a maintainer decision:

- Physics assets for the Wolf and the Eagle, before WP-15 (ADR 0007; none exist yet).
- Sign-off on ADR 0008, and which hand-off triggers the show uses (its open question 1).
- WP-1 at the desk: a mannequin driven live through Live Link (Open3DBroadcast or a capture
  system), calibrated with the subsystem, with Root, Chain and Two-Bone Limb modules.
- Wolf and Eagle: the skeletal meshes (or their bone hierarchies and reference poses), their morph
  targets, and how each is performed (one performer upright, two performers, or another way).
