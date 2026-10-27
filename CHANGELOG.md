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

### Changed

- **Live module tuning** (WP-8): a number edited in a module (Weight, Amplification, Stiffness, a
  digit's gain), on the module or through the puppet asset's Modules, applies on the next frame
  without binding the puppet again (`PuppetEditing::IsLiveEdit`). Before, every module edit
  re-initialised the puppet, resetting its hand-off offset, dynamics and smoothing, so tuning
  during a session reset the puppet. Names, vectors, enums, flags, list changes and the asset's
  own settings still re-bind it. The Puppet Solve node also keeps each slot's status (slot,
  source, subject or clip, state, control blend), copied on the game thread in `PreUpdate`, for
  the session panel (`FAnimNode_PuppetSolve::GetSlotStatus`). 3 new tests (160).
- **CI in one workflow with a status comment:** `ci.yml` (was `ue-build-test.yml`) now also runs
  the source checks (was `plugin-checks.yml`), and its last job posts a new comment on the PR
  each time it finishes, through rules-for-robots' `rfr-ci-status.yml`, so a session waiting to
  merge on green is woken. Drafts get no comment.

### Fixed

- **Two-Bone Limb weight** (found in WP-4): with a weight below 1, including every hand-off fade
  (ADR 0008), the module blended the lower and end bones from where blending the upper bone had
  moved them, not from their solved transforms, so the hand or foot landed in the wrong place. It
  now reads every solved transform before blending. 1 new test (95).

### Added

- **Setup assistant** (WP-8): **Propose Modules** on a puppet asset fills its empty module list
  for Target Mesh (`PuppetSetup::ProposeModules`). The mesh's bones are named through the
  naming schemes (`PuppetNaming::Match`), and the asset gets Root with Pelvis Bone set, a spine
  Chain (from `spine_01` to the top of the spine, or to where the neck hangs), a neck and head
  Chain, a Two-Bone Limb per arm and per leg (legs Pelvis Relative), Digits per hand with
  fingers, Eyes, and a Dynamic Chain for each leftover chain of two or more bones (a tail, ears),
  leaving out IK, twist and corrective bones. Puppet bones keep the mesh's names; performer
  joints are canonical. It leaves an asset that has modules alone. Setup Result reports what it
  proposed. 3 new tests (157).
- **Bone pickers** (WP-8, ADR 0002): in the details panel, every module's puppet bone fields are
  dropdowns of the puppet asset's Target Mesh bones (the canonical joints when no mesh is set),
  performer joint fields list the canonical joints (`PuppetNaming::CanonicalJoints`), and
  Performer Slot and Root Slot list the asset's slots. A performer profile's mapping offers the
  canonical joints as keys and Source Skeleton's bones as values (without one, the naming
  schemes' bone names and the values already mapped). Curve names stay free text.
  `Open3DPuppet.Pickers.Coverage` checks every library module's bone and joint fields by
  reflection. 4 new tests (154).
- **Prop Chain and props as performers** (WP-7, ADR 0004): a Live Link Transform-role subject
  (one tracked rigid body, such as a puppeteer's rod) is read as a performer with one joint,
  `prop`, so it is cast, calibrated (Capture, Calibrate Performer) and mapped like any performer.
  `UPuppetModule_PropChain` drives a chain such as a tail from it, or from any performer joint:
  **Shape** `Bend` (the base keeps its rest, the tip turns fully with the prop) or `Rigid`;
  **Space** `Parent` (the prop's motion adds to the body's) or `Stage` (the tip points where the
  prop points); **Amplification**; **Smoothing**, exponential, so frame-rate independent. The
  [multi-performer sample](docs/guides/multi-performer-sample.md) sets up a body and a
  puppeteered tail on two slots, and its tests check it. 9 new tests (150).
- **Puppet rest pose** (WP-2, ADR 0003): a puppet asset's `RestPose` (an AnimSequence on the
  puppet's skeleton) and `RestPoseTime` set the pose modules retarget from, for a mesh modelled in
  another pose than performers calibrate in, such as an A-pose mesh for T-pose performers. Its
  bone rotations are used, by bone name, and bone lengths stay the mesh's
  (`FPuppetClipSampler::SampleOnto`). Without it, modules retarget from the mesh's reference pose
  as before. `FPuppetPose::GetReferenceComponent` returns the rest pose. 2 new tests (141).
- **Naming schemes and profile auto-mapping** (WP-2, ADR 0003): Project Settings > Plugins >
  Open3DPuppet > Naming Schemes maps canonical joints to capture systems' bone names, starting
  with defaults for the UE5 mannequin, Xsens, OptiTrack / Motive, Rokoko, Vicon, Qualisys, Sony
  Mocopi, Move One, HumanIK / MotionBuilder, Mixamo and Unity humanoid (VMC), taken from the UE
  5.8 IKRig auto-characterizer's template skeletons and VMCLiveLink's humanoid bone list; edit
  them or add others there. A performer profile's **Map From Skeleton** and **Map From Live Link
  Subject** buttons fill its bone mapping from the scheme that fits the source best: names match
  ignoring case, a namespace (`mixamorig:`) and a prefix every bone shares, and a scheme loses a
  point for each joint whose parent contradicts it, so Xsens and Mixamo (which share names) are
  told apart by the top of the spine. The profile reports the scheme, or the schemes that map the
  same bones, and the joints the source lacks. 4 new tests (139).
- **Saved calibrations** (WP-2, ADR 0003): `UPuppetCalibrationAsset` keeps a performer's
  calibration in an asset. **Capture** (a button in its details panel, and callable from
  Blueprint) calibrates its Live Link `Subject` with its `Alignment` and saves the result;
  **Apply** makes the saved calibration the subject's current one. A cast entry's
  `SavedCalibration` is used while the subject has no calibration in the Puppet Calibration
  subsystem, so a show can start without calibrating; a calibration captured since takes
  precedence. 3 new tests (135).
- **Performer clips as a cast source** (WP-3, ADR 0008): a cast entry's `Source` is a Live Link
  subject or an `Animation`, an AnimSequence of human motion on its own skeleton (a recorded take,
  mocap delivered as FBX), which the Puppet Solve node samples on the animation worker thread and
  gives the slot as a Live Link subject's bones would, so profiles, calibration and every module
  work unchanged and a take can be solved again with different module settings. Playback has
  `PlayRate` (negative plays backwards), `bLoop` and `StartTime`; `bUseTime` with `Time` shows a
  set time instead, so Sequencer or a Blueprint can scrub the take through the Cast pin. The clip
  is calibrated from a frame (`CalibrationTime`, the first by default) or its skeleton's reference
  pose, with `CalibrationAlignment`. Bones only: the clip's curves are not read. Bones keep the
  take's own translations whatever the skeleton's translation retargeting modes; the sequence's
  Force Root Lock still pins its root. Additive sequences are rejected with a warning.
  `FPuppetClipSampler` does the sampling; the test harness builds skeletons and AnimSequences in
  code. 6 new tests (132).
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
