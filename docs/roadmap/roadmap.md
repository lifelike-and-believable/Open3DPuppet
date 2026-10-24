# Roadmap

Work packages (WPs) in the order they are planned. One WP per pull request, branched from
`develop` (AGENTS.md). The design is in [`docs/design/overview.md`](../design/overview.md).

| WP | Scope | Depends on | Status |
|---|---|---|---|
| WP-0 | Foundation: conventions, CI checks, design overview, ADRs 0001 to 0005, host project, plugin skeleton | | In review (#1) |
| WP-1 | Solver core (performer pose, calibration, puppet pose, retarget math), module contract, Root, Chain and Two-bone limb modules, Puppet Solve AnimGraph node reading Live Link, calibration subsystem, solver tests, module authoring guide | WP-0, ADRs 0002 to 0004 | In review |
| WP-2 | Calibration and profiles in the editor: calibrate button, save calibrations to assets, choice of reference pose, profile auto-mapping by name | WP-1 | Planned |
| WP-3 | Recorded motion: confirm Take Recorder record and playback of Live Link subjects on 5.8; AnimSequence player as a Live Link source; re-solving a recorded take | WP-1 | Planned |
| WP-4 | Digitigrade leg module | WP-1 | Planned |
| WP-5 | Tail and dynamic chain module (springs, input mix) | WP-1 | Planned |
| WP-6 | Wing module | WP-1 | Planned |
| WP-7 | Prop-driven module (Live Link Transform role) and a multi-performer sample | WP-1 | Planned |
| WP-8 | Setup assistant (finds chains, proposes modules), bone pickers bound to the target mesh, a session control panel | WP-1 | Planned |
| WP-9 | Fab readiness: packaging script, icon, UE build and test CI on a Windows runner, zero-warning build | WP-1 | Planned |
| WP-10 | Control Rig adapter module | WP-1 | Planned |

## Next

Items that need a person at the desk or a maintainer decision:

- Sign-off on ADRs 0001 to 0005 and their open questions (licence, Fab, canonical joint names,
  C++-first module contract).
- A Windows machine or runner with UE 5.8 for building and testing the plugin. Nothing in WP-0 or
  WP-1 has been built with Unreal Build Tool or run yet.
- WP-1 at the desk: a mannequin driven live through Live Link (Open3DBroadcast or a capture
  system), calibrated with the subsystem, with Root, Chain and Two-Bone Limb modules.
- A first target creature for the end-to-end demo (for example a dog-legged biped with a tail).
