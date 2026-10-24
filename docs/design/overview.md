# Open3DPuppet design overview

- **Status:** Draft for maintainer review (2026-10-24)
- **Decisions:** [ADR 0001](../adr/0001-platform-and-engine-scope.md) to [ADR 0008](../adr/0008-animation-sources-and-handoffs.md)
- **Plan:** [`docs/roadmap/roadmap.md`](../roadmap/roadmap.md)

## 1. Goal

Make it as simple as possible to take a skeletal mesh, attach a set of puppet modules to it, and
start puppeteering it from live motion capture or recorded motion. The toolkit targets creatures
and characters whose proportions or anatomy differ from the performer's: wings, tails, dog legs,
long or short limbs, extra limbs, and puppets driven by more than one performer at once.

Three properties follow from that goal and shape every decision below:

1. **Modular.** A puppet is a list of modules, each responsible for one region of the skeleton.
   New anatomy means a new module, not a new rig.
2. **Retargeting lives in the module.** Each module owns the controls that map the performer's
   motion onto its region (scale, offsets, amplification, contact handling). Those controls are
   what a puppeteer or technical director adjusts during a session.
3. **Source-agnostic.** Performers arrive through Live Link, so Vicon, OptiTrack, Xsens, Rokoko,
   Open3DBroadcast and recorded takes all look the same to a module.

## 2. Vocabulary

| Term | Meaning |
|---|---|
| Performer | A person, or a recorded take of one, whose motion arrives as a Live Link subject with the Animation role. |
| Canonical joint | A performer joint named the way the UE5 mannequin names it (`pelvis`, `spine_01`, `upperarm_l`, ...). Modules address performer joints only by these names (ADR 0003). |
| Curve | A named float value. Performers send them (facial expressions, from a face subject) and puppets receive them (morph targets, material parameters) (ADR 0006). |
| Performer profile | An optional `UPuppetPerformerProfile` data asset that maps a source's own bone names to canonical joints. A source that already uses mannequin names needs none. |
| Calibration | A performer's rest pose, captured once while the performer stands in the reference pose (T-pose by default), plus the rotation that aligns the performer's facing with the puppet's. |
| Puppet | A `UPuppetAsset`: the target skeletal mesh, the performer slots, and an ordered list of modules. |
| Performer slot | A named role in a puppet, for example `Body`, or `Front` and `Back` for a two-person quadruped. |
| Cast | The binding of each performer slot to a Live Link subject (and optional profile), set on the AnimGraph node, so one puppet asset works with any performer. |
| Puppet module | A `UPuppetModule`: configuration for one region of the puppet, and a factory for the solver that drives it. |
| Solver | The per-instance object a module creates. It caches bone indices and any state (springs, filters) and runs every frame. |

## 3. Frame pipeline

Every evaluation of the `Puppet Solve` AnimGraph node runs the same four stages, on the animation
worker thread:

```
 Live Link subject(s) ──► 1. Source ──► 2. Normalize ──► 3. Module solve ──► 4. Output pose
   (live or recorded)     evaluate      profile +         modules in         local-space pose
                          per slot      calibration       list order         to the AnimGraph
```

1. **Source.** For each performer slot, evaluate the bound Live Link subject
   (`ILiveLinkClient::EvaluateFrame_AnyThread` with `ULiveLinkAnimationRole`), giving bone names,
   parents and local transforms. A slot with no data this frame keeps its last pose; a slot that
   has never had data (or is not calibrated) leaves its modules' bones in the pose the node
   received. Each change of slot state is logged once.
2. **Normalize.** Build an `FPuppetPerformerPose`: canonical joint lookup through the profile, and
   component-space transforms. Pair it with the slot's calibration. Everything after this stage
   works in calibrated, canonical performer space.
3. **Module solve.** Start from the puppet's reference pose and run each enabled module's solver
   in list order. A module reads the performer channels it declares and writes the puppet bones
   it claims. Order matters by design: the root module runs first, limbs run after the spine they
   hang from, and secondary modules (tails, ears) run last so they see the final body pose.
4. **Output.** The solved pose is converted to local space and returned to the AnimGraph, so it
   can be blended, layered and recorded like any other pose. Bones no module claims keep their
   reference-pose local transforms and follow their parents.

## 4. Retargeting model

Modules share one small set of operations, implemented once in the solver core:

- **Rotation delta (FK).** For a puppet bone mapped to a performer joint, the puppet bone's
  component-space rotation is the performer joint's rotation change since calibration, applied to
  the puppet bone's own rest rotation:
  `Q_puppet = A · (Q_performer · Q_performerRest⁻¹) · A⁻¹ · Q_puppetRest`, where `A` is the
  calibration's alignment. This is independent of how either skeleton orients its bone axes,
  which is what lets any capture skeleton drive any mesh.
- **Chain resampling.** A performer chain and a puppet chain need not have the same number of
  bones. Each puppet bone sits at a normalized position along its chain (by rest length), and its
  delta is interpolated (slerp) between the performer deltas at the neighbouring positions. Three
  performer spine joints can drive a five-bone spine or a twelve-bone tail.
- **Translation scaling.** Positions are mapped with scale factors derived from rest proportions
  (puppet hip height over performer hip height by default) and overridable per module.
- **End-effector placement.** Limb modules can follow the performer's FK, or place the end
  effector (hand, foot, paw) with two-bone IK at a target computed in one of several modes:
  scaled reach (relative to the limb root, scaled by limb length), pelvis-relative (relative to the
  pelvis, scaled like the root, which keeps feet planted when the puppet is taller or shorter), or
  absolute (the performer's position, for contact with props and other performers).
- **Weights.** Every module has a weight that blends its result over the pose it received, so a
  region can be faded between performer control and the reference pose (or an upstream pose).

## 5. Puppet modules

### 5.1 Contract

A module is two pieces (ADR 0002; the [authoring guide](../guides/authoring-puppet-modules.md) has the details and a worked example):

- `UPuppetModule` (a `UObject`, instanced inside the puppet asset): the editable settings, the
  performer slot it reads, and `CreateSolver()`. It is shared by every actor that uses the
  puppet, so it holds no per-frame state.
- `FPuppetModuleSolver` (plain C++): created once per AnimGraph node instance. `Bind()` resolves
  bone names to indices against the puppet's reference skeleton and the performer's joints and
  reports what is missing; `Solve()` runs every frame, reads the performer pose and calibration
  through `FPuppetSolveContext`, and writes the puppet pose. `Solve()` must not allocate.

Retargeting controls are ordinary `UPROPERTY` members of the module, read by the solver each
frame, so a change in the details panel shows up live in the viewport.

### 5.2 Library roadmap

| Module | Drive | Retarget controls | Work package |
|---|---|---|---|
| Root (pelvis) | Performer pelvis | Horizontal and vertical scale (auto from hip heights), height offset | WP-1 |
| Chain (spine, neck, head, generic FK) | Any performer chain | Weight, chain resampling, amplification | WP-1 |
| Two-bone limb (arm, leg) | Performer limb | FK, scaled reach, pelvis-relative or absolute IK; pole direction; stretch limits | WP-1 |
| Digitigrade leg (3-bone) | Performer leg | Knee-to-hock ratio, heel lift, stride scale, foot contact | WP-4 |
| Tail / dynamic chain | Pelvis motion plus springs, or a performer chain or prop | Stiffness, damping, amplitude, input mix, physics weight | WP-5 |
| Wing | Performer arm (shoulder to wing root, elbow to fold) | Fold/spread curve, flap amplification, feather fan | WP-6 |
| Prop-driven | Tracked rigid body (Live Link Transform role) | Mapping space, filtering | WP-7 |
| Digits (fingers, toes, talons, feathers) | Performer fingers, relative to the hand | Finger-to-digit mapping, curl and spread gains, chain resampling | WP-12 |
| Curve to bone | A performer curve (face, finger curl) | Source range to bone rotation or translation range, dead zone, smoothing | WP-13 |
| Curve remap | Performer curves (ARKit's 52 blend shapes by default; other sources through the profile) | Per-curve remap, combination, smoothing to puppet curves (morph targets) | WP-13 |
| Eyes | Eye-look curves or eye bones | Range limits, convergence | WP-13 |
| Physics Region (optional module) | Its controls or a performer curve | Strength and damping of a region in Rigid Body With Control | WP-15 |
| Control Rig adapter | A Control Rig asset with a known interface | The rig's own controls | WP-10 |

## 6. Several performers

A puppet declares its performer slots, and each module names the slot it reads (ADR 0004). The
cast on the AnimGraph node binds slots to Live Link subjects. Exactly one slot owns the root:
the root module reads it, and the other slots' modules work in rotations, or in positions relative
to their own performer's pelvis, so two performers standing apart on the stage can still drive
the front and back of one creature. Examples:

- A two-person quadruped: `Front` drives the spine front half, neck, head and front legs; `Back`
  owns the root and drives the hind legs and tail.
- A dragon: `Body` drives the biped core; a second performer's arms drive the wings.
- A creature with a puppeteered tail: `Body` plus a `Tail` slot bound to a tracked rod (WP-7).

## 7. Live and recorded motion

Live: any Live Link source, including Open3DBroadcast's receiver.

Recorded: a cast entry can name an AnimSequence of human motion instead of a Live Link subject
(ADR 0008, WP-3), so a recorded take or mocap delivered as FBX drives the puppet through the same
modules as a live performer, and can be solved again later with different module settings, which
matters most for creatures, where the mapping is usually tuned after the shoot. Takes recorded as
Live Link subjects with Take Recorder play back into Live Link from Sequencer
(**needs-verification** on UE 5.8; ADR 0003 open question 2). The solved puppet pose can be
recorded too, as an ordinary animation. Section 12 covers clips authored on the creature itself,
and handing control between a clip and a performer.

## 8. Setting up a puppet (target workflow)

1. Create a Puppet asset (Data Asset > Puppet Asset) and pick the skeletal mesh.
2. Add modules and set their bones. (WP-8 adds a setup assistant that finds chains by name and
   topology and proposes modules.)
3. In the character's Animation Blueprint, add the **Puppet Solve** node, choose the puppet, and
   bind each performer slot to a Live Link subject.
4. Have the performer stand in the reference pose and call **Calibrate Performer** on the Puppet
   Calibration subsystem (from Blueprint now; an editor button comes in WP-2).
5. Puppeteer. Adjust module controls live in the details panel; WP-8 adds a control panel for
   session use.

## 9. Out of scope for now

- Platforms other than Win64 and engines before 5.8 (ADR 0001).

## 10. First characters: Wolf and Eagle

The first puppets are the Wolf and the Eagle from *Mamaawi Together Through the Fire*
(maintainer, 2026-10-24). They set the order of the module work (roadmap) and are the end-to-end
tests of the toolkit. The mappings below are a starting proposal, to be confirmed with the
maintainer and the meshes; each line names the module that carries it.

### Wolf

| Puppet | Performer | Module |
|---|---|---|
| Pelvis, place in the world | Pelvis | Root |
| Spine, neck, head | Spine, neck, head | Chain |
| Front legs | Arms (shoulder, elbow, wrist to the paw) | Digitigrade leg (WP-4) |
| Hind legs | Legs | Digitigrade leg (WP-4) |
| Front paw toes | Fingers | Digits (WP-12) |
| Tail | Pelvis motion with springs; a hand gesture or a second performer can take it over | Tail (WP-5) |
| Jaw, lips (snarl) | `jawOpen`, `noseSneer`, `mouthUpperUp` | Curve to bone, curve remap (WP-13) |
| Ears | Brow curves | Curve to bone (WP-13) |
| Eyes, eyelids | Eye-look and blink curves | Eyes, curve to bone (WP-13) |

### Eagle

| Puppet | Performer | Module |
|---|---|---|
| Pelvis, place in the world | Pelvis | Root |
| Spine, neck, head | Spine, neck, head | Chain |
| Wings | Arms (shoulder to wing root, elbow and wrist to the fold) | Wing (WP-6) |
| Primary feathers, alula | Finger spread and curl, thumb | Digits (WP-12) |
| Legs | Legs | Two-bone limb or digitigrade leg |
| Talons | A finger-curl or face curve chosen by the puppeteer | Curve to bone (WP-13) |
| Tail fan | Pelvis motion, or finger spread | Tail (WP-5), Digits (WP-12) |
| Beak | `jawOpen` | Curve to bone (WP-13) |
| Crest and head feathers | Brow curves | Curve to bone (WP-13) |
| Eyes, eyelids | Eye-look and blink curves | Eyes, curve to bone (WP-13) |

## 11. Addendum: physics-driven modules

Added 2026-10-24 at the maintainer's request; the decision is
[ADR 0007](../adr/0007-physics-driven-modules.md). Physics gives a puppet what a performer cannot
provide directly: secondary motion that follows from the body's movement (a tail that swings
and settles, ears and a mane that lag a head turn, feathers that ruffle on landing), and weight
and contact (a paw that stops on the ground, a body that reacts to a push, a region that goes
limp and recovers on cue).

It comes in two tiers:

```
 performers ──► Puppet Solve ───────────────────────────────► (Rigid Body With Control) ──► output
                 modules, in order:                              optional, Tier 2: bodies of the
                 kinematic (retarget) and dynamic (Tier 1)       physics asset follow the solved
                 Physics Region modules publish curves ───────►  pose with the strengths the
                                                                 curves ask for
```

- **Tier 1, solver dynamics, in the core plugin.** Dynamic modules simulate inside Puppet Solve
  with shared, tested building blocks (`PuppetDynamics`): a fixed-step integrator, angular
  spring-dampers, length-constrained chains, and collision with spheres and capsules on the
  puppet's own bones and a ground plane. The solve context gives the component's world motion,
  so a tail swings when the wolf turns or stops. Each dynamic module has a **Physics Weight**
  between following the retargeted pose and simulating, and a curve can drive it, so it is
  playable live. It is deterministic, so re-solving a take reproduces it.
- **Tier 2, engine physics hand-off, in the optional `Open3DPuppetPhysics` module.** For contact
  with the world and reactions to impacts, Puppet Solve feeds the engine's Rigid Body With Control
  node (Physics Control plugin). Physics Region modules publish each region's strength and
  damping as curves, and a function library turns them into the node's control parameters. A
  puppeteer can make the wolf's legs heavy and grounded, or let the whole body go limp and pull
  it back. Enabled only in projects that enable Physics Control, so the core plugin does not
  depend on it. Takes that use it also record the solved pose, because engine physics is not
  deterministic.

For the first characters:

| Character | Tier 1 | Tier 2 |
|---|---|---|
| Wolf | Tail, ears, mane or ruff | Paws and legs on contact with the ground; reactions and falls |
| Eagle | Tail fan, primary and covert feathers, crest | Landing and perching contact; wing reactions |

Writing a dynamic module follows the authoring guide, with two additions that land with WP-14:
the solver keeps its simulation state and resets it in `Bind` and on teleports, and its tests
step the solver with a fixed `DeltaTime` and check settling, stability at different frame rates,
and the Physics Weight blend.

## 12. Animation sources and hand-offs

Added 2026-10-24; the decision is [ADR 0008](../adr/0008-animation-sources-and-handoffs.md). In
*Mamaawi* the Wolf runs in and the Eagle flies in on an AnimSequence, then pass to the performer.

- **Two kinds of clip.** A creature clip (authored on the Wolf or Eagle skeleton) plays into the
  Puppet Solve node's input pose. A performer clip (human motion) is a cast source like a Live
  Link subject and is retargeted through the modules.
- **Control blend per slot.** From 0, the input pose (the clip), to 1, the performer. It scales
  the weights of the slot's modules, so the body can take over before the tail.
- **Transitions.** `UPuppetPerformanceComponent` moves a slot's blend over a duration with an
  easing curve, towards the performer or back to the animation. Triggers call it: a Sequencer cue,
  an operator's button, or a performer gesture (a held pose, a mark on the stage, a curve over a
  threshold). The show's choice is still open, so all three are planned.
- **Anchoring.** At a hand-off to the performer, the root is anchored where the clip left the
  creature, facing the same way, and the performer's movement continues from there: no snap to the
  performer's stage position.

```
 Wolf run clip ──► input pose ──┐
                                ├─ control blend 0 ──(transition, anchor captured)──► 1
 performer (Live Link) ─► modules┘     clip                                          performer
```
