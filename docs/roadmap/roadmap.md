# Roadmap

Work packages (WPs) in the order they are planned. One WP per pull request, branched from
`develop` (AGENTS.md). The design is in [`docs/design/overview.md`](../design/overview.md).

| WP | Scope | Depends on | Status |
|---|---|---|---|
| WP-0 | Foundation: conventions, CI checks, design overview, ADRs 0001 to 0005, host project, plugin skeleton | | Done (#1) |
| WP-1 | Solver core (performer pose, calibration, puppet pose, retarget math), module contract, Root, Chain and Two-bone limb modules, Puppet Solve AnimGraph node reading Live Link, calibration subsystem, solver tests, module authoring guide | WP-0, ADRs 0002 to 0004 | Done (#2) |
| WP-9a | UE build and test CI on the shared self-hosted runner | WP-1 | Done (#3) |
| WP-11 | Curves in the pipeline (ADR 0006): performer curves, Basic-role (curve-only) subjects, neutral calibration of curves, puppet output curves, parent-relative joint deltas | WP-1, ADR 0006 | Done (#8) |
| WP-12 | Digits module: fingers to toes, talons and feathers; derived finger-curl curves | WP-11 | Done (#13, #14; ADR 0009) |
| WP-13 | Face modules: curve to bone, curve remap, eyes | WP-11 | Done (#15) |
| WP-14 | Solver dynamics (ADR 0007, Tier 1): world motion in the solve context, `PuppetDynamics` (fixed-step integrator, spring-dampers, constrained chains, collision shapes), Physics Weight | WP-11 | Done (#18) |
| WP-4 | Digitigrade leg module (Wolf front and hind legs) | WP-1 | Done (#16) |
| WP-5 | Tail and dynamic chain module: springs, input mix (Wolf tail, ears, mane; Eagle tail fan, feathers) | WP-14 | Done (#19) |
| WP-15 | Engine physics hand-off (ADR 0007, Tier 2): optional `Open3DPuppetPhysics` module, Physics Region module, Rigid Body With Control parameters from curves, setup guide. The Wolf and Eagle need physics assets first (none exist yet) | WP-11 | Planned |
| WP-6 | Wing module (Eagle) | WP-12 | Done (#20) |
| WP-16 | Hand-offs (ADR 0008): control blend per slot, `UPuppetPerformanceComponent` transitions with easing, root anchoring that converges on the performer's position (by time, while moving), manual re-centre | WP-1 | Done (#9) |
| WP-17 | Hand-off triggers: Sequencer cue, operator input (input action, Remote Control), performer gesture | WP-16 | Planned |
| WP-3 | Performer clips as a cast source (ADR 0008): AnimSequence playback (rate, loop, start, time pin), calibration from a frame, re-solving a recorded take. Not needed for the show's entrances, which are creature clips | WP-1 | Done (#21) |
| WP-M1 | Wolf: puppet asset, profiles and calibration for the show's capture setup, run-in entrance (a clip authored on the Wolf) and hand-off; live session at the desk | WP-4, WP-5, WP-12, WP-13, WP-15, WP-16 | Planned |
| WP-M2 | Eagle: puppet asset, profiles and calibration, fly-in entrance (a clip authored on the Eagle) and hand-off; live session at the desk | WP-5, WP-6, WP-12, WP-13, WP-15, WP-16 | Planned |
| WP-2 | Calibration and profiles in the editor: calibrate button, save calibrations to assets, choice of reference pose, profile auto-mapping by name | WP-1 | Done (#24, #25, #26) |
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

- WP-6 at the desk: a performer flapping and folding the Eagle's wings; set the fold poses and
  feather fold for the mesh, and tune the fold range and flap amplification.
- WP-5 at the desk: the Wolf's tail, ears and mane swinging as the performer walks, turns and
  stops; tune Stiffness, Damping and Amplitude per chain, and a curve playing Physics Weight.
- WP-4 at the desk: a performer walking and crouching driving a digitigrade leg, hind legs from
  the legs and front legs from the arms; tune the default Knee To Hock Ratio (0.5), Heel Lift and
  Contact Height on the Wolf.
- WP-13 at the desk: Live Link Face driving a jaw with Curve to Bone, a morph target with Curve
  Remap and the eyes with Eyes; tune the default ranges, smoothing and eye limits per creature.
- WP-12 at the desk: a glove or suit with finger bones driving a mannequin hand with the Digits
  module, and its `curl_*` curves reading 0 flat and about 1 in a fist, and the Wolf's paw toes once the mesh exists; tune the default gains.
- Physics assets for the Wolf and the Eagle, before WP-15 (ADR 0007; none exist yet).
- ADR 0008: which hand-off triggers the show uses (question 1).
- WP-2 at the desk: Capture a performer into a calibration asset, restart, and drive the puppet
  from it as the cast entry's Saved Calibration; Map From Live Link Subject on each capture system
  the show uses (the default naming schemes are the IKRig templates' names, not checked against
  live streams; correct them in Project Settings where they differ); an A-pose mesh with a T-pose
  Rest Pose clip.
- WP-3 at the desk: a recorded take (an FBX mocap take on a mannequin skeleton) cast into the
  Body slot of a puppet, calibrated from its first frame, playing, looping and scrubbed from
  Sequencer through the Cast pin, then handed to a live performer with a transition.
- WP-16 at the desk: a creature clip into a performer hand-off and back, and a manual re-centre,
  with a live performer; tune the default convergence time and moving speed.
- WP-11 at the desk: Live Link Face (ARKit curves) cast into a curve-only slot, its neutral
  calibrated, and output curves reaching the puppet's morph targets.
- WP-1 at the desk: a mannequin driven live through Live Link (Open3DBroadcast or a capture
  system), calibrated with the subsystem, with Root, Chain and Two-Bone Limb modules.
- Wolf and Eagle: the skeletal meshes (or their bone hierarchies and reference poses), their morph
  targets, and how each is performed (one performer upright, two performers, or another way).
