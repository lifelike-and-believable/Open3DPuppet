# 0007: Physics-driven modules

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-24
- **Related:** [ADR 0002](0002-puppet-module-architecture.md) (amends the solve context), [ADR 0006](0006-fingers-face-and-curves.md) (output curves), [design overview](../design/overview.md) §11

## Context

The maintainer asked (2026-10-24) for physics-driven modules. For the Wolf and the Eagle that
means two kinds of motion that a performer cannot provide directly:

- **Secondary motion** that follows from the body's movement: a tail that swings and settles,
  ears and a mane that lag a head turn, feathers that ruffle when the Eagle lands.
- **Physical interaction** with the world and with weight: a paw that stops on the ground, a body
  that reacts to a push or an impact, a region that goes limp and recovers when a puppeteer asks.

WP-1 solves kinematically: every module writes bones from the performer's current frame, and the
solve context has no notion of where the puppet is in the world, so a module cannot feel the
puppet accelerate or turn. The engine already has physics animation nodes (UE 5.7.4 source):

- `FAnimNode_RigidBody` and `FAnimNode_AnimDynamics` in AnimGraphRuntime
  (`Engine/Source/Runtime/AnimGraphRuntime/Public/BoneControllers/AnimNode_RigidBody.h`,
  `AnimNode_AnimDynamics.h`).
- `FAnimNode_RigidBodyWithControl` in the Physics Control plugin, marked Beta
  (`Engine/Plugins/Experimental/PhysicsControl/PhysicsControl.uplugin`). Its control targets and
  strengths are per-frame pins
  (`Engine/Plugins/Experimental/PhysicsControl/Source/PhysicsControl/Public/AnimNode_RigidBodyWithControl.h:362-380`).
  It drives a physics asset's bodies towards the animation with springs, which suits
  puppetry: the performer gives the intent, physics gives weight and contact.

The animation instance proxy gives the component's world transform on worker threads
(`FAnimInstanceProxy::GetComponentTransform`,
`Engine/Source/Runtime/Engine/Public/Animation/AnimInstanceProxy.h:557`).

## Decision drivers

1. Puppeteers can make a region more or less physical while performing, live.
2. Secondary motion is stable at any frame rate, and the same take re-solved gives the same
   motion.
3. The core plugin does not require a Beta engine plugin.
4. Module authors get the dynamics building blocks once, tested, as they get the retarget math.

## Options considered

### Option A: engine physics only (puppet output into RigidBody With Control)
- Pros: real collision with the world, the physics asset's limits.
- Cons: not deterministic, so a re-solved take differs; every tail and ear needs physics bodies
  and tuning in the physics asset; a Beta plugin becomes a requirement.

### Option B: dynamics inside the modules only
- Pros: deterministic, cheap, testable like the other modules, no physics asset.
- Cons: no contact with the world beyond simple shapes; no reaction to impacts.

### Option C: both, in two tiers (chosen)

## Decision

1. **Tier 1, solver dynamics (in the core plugin).** Modules may simulate inside `Solve`, keeping
   their state in the solver (ADR 0002):
   - The solve context gains the component's world transform for this frame and the last one, so
     a module can feel the puppet move, turn and stop. A jump larger than a threshold counts as a
     teleport and resets dynamics, as does a new calibration.
   - Shared building blocks in `PuppetDynamics` (as `PuppetRetarget` for math): a fixed-step
     integrator (simulation at a fixed rate, independent of frame rate, interpolated for output),
     angular spring-dampers, and a chain with length constraints, with tests.
   - Simple collision shapes (spheres and capsules on puppet bones, and a ground plane) that
     dynamic chains avoid, so a tail does not pass through the legs or the floor.
   - Every dynamic module blends between follow (pure performer or retarget result) and
     simulated with a `Physics Weight` control, which a curve can drive (ADR 0006), so a
     puppeteer can play it live.
2. **Tier 2, engine physics hand-off (an optional module).** A separate runtime module,
   `Open3DPuppetPhysics`, depends on the Physics Control plugin and is enabled only when the
   project enables that plugin (an optional plugin dependency in the `.uplugin`). It provides:
   - A **Physics Region** puppet module that writes no bones; it publishes the strength and
     damping a puppeteer wants for a named region (for example `FrontLegs`, `Tail`, `Body`) as
     output curves, driven by its controls or by a performer curve.
   - A Blueprint function library that turns those curves into the Rigid Body With Control
     node's `ControlAndModifierParameters`, so the AnimGraph reads: Puppet Solve, then Rigid Body
     With Control.
   - A documented AnimGraph setup and physics asset guidelines for the Wolf and the Eagle.
3. **Recording.** Tier 1 is deterministic, so re-solving a recorded take reproduces it. Tier 2 is
   not, so takes that use it also record the solved output pose (design overview §7).
4. **Contract change.** Item 1's world transforms in the solve context change a contract type, so
   this ADR records it. Existing modules need no change.

## Consequences

- WP-5 (tail) builds on Tier 1; ears, mane and feathers reuse the same dynamic chain.
- The core plugin stays free of Beta dependencies; projects that want contact and impacts enable
  Physics Control and the optional module.
- A Tier 2 setup needs a physics asset per character, authored outside this toolkit.

## Open questions for the maintainer

1. Which regions of the Wolf and the Eagle should be physical in the show (tail, ears, mane,
   feathers, legs on contact, whole-body falls)?
2. Do the characters already have physics assets?
3. needs-verification: Physics Control's status and the Rigid Body With Control node's pins on
   UE 5.8 (read from 5.7.4, where the plugin is Beta).
