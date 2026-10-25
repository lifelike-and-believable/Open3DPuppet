# Changelog

The one changelog for the Open3DPuppet plugin. Each release gets a `## [x.y.z] - date` section.
Until then changes collect under `## Unreleased`.

## Unreleased

### Planned

- **Fingers, face and the first characters** (ADR 0006): curves in the pipeline, Digits, face and
  eye modules, and the Wolf and Eagle of *Mamaawi Together Through the Fire* as the first puppets
  (design overview §10, roadmap WP-11 to WP-13, WP-M1, WP-M2).
- **Physics-driven modules** (ADR 0007): solver dynamics in the core plugin and an optional
  hand-off to the engine's Rigid Body With Control node (design overview §11, roadmap WP-14,
  WP-15).
- **Animation sources and hand-offs** (ADR 0008): AnimSequences of human motion as a cast source,
  creature clips through the input pose, per-slot control blends with timed transitions, and root
  anchoring that converges on the performer's position, for entrances such as the Wolf's run-in and the Eagle's fly-in (design overview §12,
  roadmap WP-3, WP-16, WP-17).

### Fixed

- **Two-Bone Limb weight** (found in WP-4): with a weight below 1, including every hand-off fade
  (ADR 0008), the module blended the lower and end bones from where blending the upper bone had
  moved them, not from their solved transforms, so the hand or foot landed in the wrong place. It
  now reads every solved transform before blending. 1 new test (95).

### Added

- **Wing module** (WP-6): the Eagle's wings from the performer's arms. The wing root flaps with the
  upper arm relative to the chest (`FlapAmplification`); the elbow bend, through
  `FoldStartAngle` to `FoldEndAngle`, folds the forearm and hand towards `ForearmFold` and
  `HandFold` (set per mesh) and closes the listed `Feathers` like a fan, the last by the whole
  `FeatherFold`. 8 new tests (126).
- **Dynamic Chain module** (WP-5, ADR 0007): a tail, ears, a mane or feathers that swing with the
  puppet's motion and settle. The chain follows the pose it receives, optionally mixed with a
  performer chain (`InputMix`, for a hand gesture or a second performer taking the tail over), and
  simulates on it with `PuppetDynamics` at a fixed step: `Stiffness` towards that pose, `Damping`,
  `Gravity`, `Amplitude` on the swing, a `PhysicsWeight` a performer curve can play, and collision
  with spheres on bones and the ground. 10 new tests (118).
- **Solver dynamics** (WP-14, ADR 0007, Tier 1): the solve context carries the puppet component's
  world transform this frame and last, and `bTeleport` (the first frame, or a jump beyond the
  asset's new `TeleportDistance`, 200 cm by default); `FPuppetInstance::Solve` takes the component
  transform, which the Puppet Solve node passes. `PuppetDynamics` gives dynamic modules their
  building blocks: a fixed-step integrator, an angular spring-damper, a length-constrained chain,
  collision with spheres, capsules and the ground, the component's motion between frames, and a
  Physics Weight control that a performer curve can drive. The authoring guide has a section on
  dynamic modules. 13 new tests (108).
- **Digitigrade Leg module** (WP-4): a three-segment leg (thigh, shin, metatarsal, paw), such as
  the Wolf's hind and front legs, from a performer's leg or arm. The paw follows the performer's
  ankle relative to the hip, scaled by leg length, with `StrideScale` on horizontal moves; the
  metatarsal keeps its reference angle, flexed with the performer's knee (`KneeToHockRatio`) and
  raised by `HeelLift`; two-bone IK places the thigh and shin at the hock; within `ContactHeight`
  of the ground the paw stays level. 8 new tests (94).
- **Face modules** (WP-13, ADR 0006): **Curve to Bone** turns and moves one puppet bone from one
  performer curve (input range, dead zone, smoothing; a jaw, beak, ears, eyelids, a talon from a
  finger curl). **Curve Remap** writes puppet curves (morph targets, material parameters) from
  performer curves, each mapping with its input and output range and dead zone; mappings onto the
  same puppet curve add up; smoothing per curve. **Eyes** turns the puppet's eyes from ARKit's
  eye-look curves or the performer's eye joints, with Max Yaw and Max Pitch and a Convergence that
  lets both eyes share one gaze. New shared math: `PuppetRetarget::RemapCurve` and
  `SmoothTowards`. 20 new tests (86).
- **Derived finger-curl curves** (WP-12, ADR 0009): a performer with finger bones has a curve
  per finger, `curl_<finger>_<side>` (`curl_index_l` and so on), found with `FindCurve` like any
  source curve: the unsigned bend of the finger's three joints relative to their parents since
  calibration, summed, over 270 degrees (0 as calibrated, 1 for a fist). Curve modules can use
  hand gestures as controls. A source curve of the same name wins. Calibrations capture source
  curves only. `FPuppetPerformerPose` gains `UpdateDerivedCurves`, `GetSourceCurveNames` and
  `GetSourceCurveValues`. 5 new tests (66).
- **Digits module** (WP-12, ADR 0006): `UPuppetModule_Digits` drives puppet fingers, toes, talons
  or feathers from the performer's fingers, relative to the hand, so a digit curls the same way
  whatever the hand or paw is doing. Each entry in `Digits` maps a finger chain onto a digit chain
  of any length (the finger's whole bend is spread along the digit) with a `CurlGain` and a
  `SpreadGain`; `SpreadAxis` (the palm's normal) separates spread from curl. The defaults map a
  mannequin's left hand onto the same names. 8 new tests (61).
- **Hand-offs between an animation and the performer** (WP-16, ADR 0008): each performer slot
  has a control blend (0 the Puppet Solve node's input pose, 1 the performer) that scales every
  module weight of the slot; modules blend with the new `FPuppetSolveContext::Weight`.
  `UPuppetPerformanceComponent` on the puppet's actor runs timed, eased transitions per slot
  (`TransitionToPerformer`, `TransitionToAnimation`, `SetControlBlend`, `Recentre`, an event when
  a transition ends; slots can start on the animation). A hand-off to the performer or a manual
  re-centre captures a hand-off offset (a yaw turn and a horizontal move, in the solve context as
  `HandoffOffset`) so the puppet continues from where the animation left it; the Root module
  converges it on the performer over `ConvergenceTime`, by default only while the performer moves,
  or holds it. The context helpers apply the offset, so every module turns with the puppet.
  11 new tests (53).
- **Curves in the pipeline** (WP-11, ADR 0006): performers carry curves from Live Link (curve
  names from static data, values per frame), looked up by canonical name with an optional
  mapping in the performer profile (`CurveCanonicalToSource`). Curve-only (Basic-role) subjects,
  such as a face, can be cast into a slot. Calibration captures each curve's neutral value.
  Modules read a curve's change from neutral (`FPuppetSolveContext::GetCurveDelta`) and write
  output curves (`FPuppetPose::SetCurve`), which the Puppet Solve node writes to the pose's
  curves. `GetJointDeltaInParent` gives a joint's rotation relative to its parent, for fingers.
  8 new tests (42).
- **Licence:** MIT, as Open3DBroadcast (maintainer, 2026-10-24), at the repository root and the
  plugin root; `Config/FilterPlugin.ini` puts it in the plugin package.
- **UE build and test CI** (WP-9a): `.github/workflows/ue-build-test.yml` builds the plugin with
  RunUAT BuildPlugin on UE 5.8 (warnings in plugin sources fail it) and runs the automation
  tests against the package, on the self-hosted runner Open3DBroadcast uses.
  `Build/Scripts/Build-Plugin.ps1` and `Run-AutomationTests.ps1` come from Open3DBroadcast.
- **Solver core and first modules** (WP-1, ADRs 0002 to 0004): `UPuppetAsset` (performer slots,
  root slot, ordered modules), the module contract (`UPuppetModule`, `FPuppetModuleSolver`,
  `FPuppetBindContext`, `FPuppetSolveContext`), the shared retargeting math (`PuppetRetarget`),
  `FPuppetInstance`, and the Root, Chain and Two-Bone Limb modules. The **Puppet Solve** AnimGraph
  node reads each performer slot from a Live Link Animation-role subject; `UPuppetPerformerProfile`
  maps other naming schemes to the canonical (mannequin) joint names; the Puppet Calibration
  subsystem captures and holds each performer's calibration. 34 automation tests. The guide
  `docs/guides/authoring-puppet-modules.md` explains how to add modules.
- **Foundation** (WP-0): coding-agent instructions (`AGENTS.md`, rules in `.claude/rules/`),
  CI checks for copyright headers, editor dependencies in runtime modules and Markdown links,
  the design overview, ADRs 0001 to 0005, the roadmap, the `ProjectSandbox` host project and an
  empty `Open3DPuppet` plugin with runtime, editor and test modules (UE 5.8, Win64).
