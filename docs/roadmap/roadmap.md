# Roadmap

Work packages (WPs) in the order they are planned. One WP per pull request, branched from
`develop` (AGENTS.md). The design is in [`docs/design/overview.md`](../design/overview.md).

| WP | Scope | Depends on | Status |
|---|---|---|---|
| WP-0 | Foundation: conventions, CI checks, design overview, ADRs 0001 to 0005, host project, plugin skeleton | | In review (#1) |
| WP-1 | Solver core (performer pose, calibration, puppet pose, retarget math), module contract, Root, Chain and Two-bone limb modules, Puppet Solve AnimGraph node reading Live Link, calibration subsystem, solver tests, module authoring guide | WP-0, ADRs 0002 to 0004 | In review (#2) |
| WP-9a | UE build and test CI on the shared self-hosted runner | WP-1 | In review (#3) |
| WP-11 | Curves in the pipeline (ADR 0006): performer curves, Basic-role (curve-only) subjects, neutral calibration of curves, puppet output curves, parent-relative joint deltas | WP-1, ADR 0006 | Planned |
| WP-12 | Digits module: fingers to toes, talons and feathers; derived finger-curl curves | WP-11 | Planned |
| WP-13 | Face modules: curve to bone, curve remap, eyes | WP-11 | Planned |
| WP-4 | Digitigrade leg module (Wolf front and hind legs) | WP-1 | Planned |
| WP-5 | Tail and dynamic chain module: springs, input mix (Wolf tail, Eagle tail fan) | WP-1 | Planned |
| WP-6 | Wing module (Eagle) | WP-12 | Planned |
| WP-M1 | Wolf: puppet asset, profiles and calibration for the show's capture setup; live session at the desk | WP-4, WP-5, WP-12, WP-13 | Planned |
| WP-M2 | Eagle: puppet asset, profiles and calibration; live session at the desk | WP-5, WP-6, WP-12, WP-13 | Planned |
| WP-2 | Calibration and profiles in the editor: calibrate button, save calibrations to assets, choice of reference pose, profile auto-mapping by name | WP-1 | Planned |
| WP-3 | Recorded motion: confirm Take Recorder record and playback of Live Link subjects on 5.8; AnimSequence player as a Live Link source; re-solving a recorded take | WP-1 | Planned |
| WP-7 | Prop-driven module (Live Link Transform role) and a multi-performer sample | WP-1 | Planned |
| WP-8 | Setup assistant (finds chains, proposes modules), bone pickers bound to the target mesh, a session control panel | WP-1 | Planned |
| WP-9 | Fab readiness: packaging script, icon | WP-1 | Planned |
| WP-10 | Control Rig adapter module | WP-1 | Planned |

Work packages are listed in the order they are planned. The Wolf and Eagle (design overview §10)
set that order: curves, digits and face first, because both characters use them, then the
anatomy modules each needs. WP-2 and WP-3 can move ahead when a session needs them.

## Next

Items that need a person at the desk or a maintainer decision:

- Sign-off on ADRs 0001 to 0006 and their open questions (licence, Fab, canonical joint names,
  C++-first module contract, the show's finger and face capture systems).
- Runner access: the shared `[self-hosted, ue5, windows]` runner must serve this repository
  (move it into the organization, or register the runner for it). Until it does, nothing in WP-0
  or WP-1 has been built with Unreal Build Tool or run.
- WP-1 at the desk: a mannequin driven live through Live Link (Open3DBroadcast or a capture
  system), calibrated with the subsystem, with Root, Chain and Two-Bone Limb modules.
- Wolf and Eagle: the skeletal meshes (or their bone hierarchies and reference poses), their morph
  targets, how each is performed (one performer upright, two performers, or another way), and the
  capture systems for body, fingers and face.
