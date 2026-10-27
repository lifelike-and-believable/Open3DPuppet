# 0010: Control Rig adapter

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-27
- **Related:** [ADR 0002](0002-puppet-module-architecture.md) (the module contract; Consequences name the adapter as the route for artists who would rather not write C++), [ADR 0003](0003-performer-input-and-calibration.md) (canonical joints, calibration), [ADR 0006](0006-fingers-face-and-curves.md) (curves), roadmap WP-10

## Context

The design overview's module table lists a Control Rig adapter: "A Control Rig asset with a known
interface", retargeted through "The rig's own controls" (`docs/design/overview.md`, §5.2). ADR
0002 chose a C++ module contract and left Control Rig to a module (WP-10), so that the core does
not depend on it. Nothing yet defines what the rig is given and what is read back. That is
what this ADR decides.

What is true today (`develop` at `d50cbaa`, UE 5.8.4 sources):

- A module's solver is created and bound on the animation worker thread:
  `FAnimNode_PuppetSolve::Evaluate_AnyThread` calls `FPuppetInstance::Initialize`, which calls
  `CreateSolver`, and the instance binds a slot's modules on demand inside `Solve`
  (`ProjectSandbox/Plugins/Open3DPuppet/Source/Open3DPuppet/Private/AnimNode_PuppetSolve.cpp`).
- Epic's own Control Rig AnimGraph node creates a rig instance on the worker thread when its rig
  is switched at runtime: under an `FGCScopeGuard`, then clears the `Async` internal flag so the
  object can be garbage collected, then `Initialize(true)` and `RequestInit()`
  (`Engine/Plugins/Animation/ControlRig/Source/ControlRig/Private/AnimNode_ControlRig.cpp:358`).
- It runs the rig on the worker thread under the rig's evaluate mutex
  (`Engine/Plugins/Animation/ControlRig/Source/ControlRig/Private/AnimNode_ControlRigBase.cpp:239`).
- In 5.8 a Control Rig is referenced by `FControlRigAssetStrongReference`, which holds either a
  blueprint rig class (the legacy path) or a `UControlRigRuntimeAsset`, and makes an instance with
  `CreateInstance` (`Engine/Plugins/Animation/ControlRig/Source/ControlRig/Public/ControlRigAssetReference.h:20`).
  The engine node's property is this struct with `DisallowCreateNew`
  (`Engine/Plugins/Animation/ControlRig/Source/ControlRig/Public/AnimNode_ControlRig.h:61`).
  The Control Rig editor registers its picker for the struct globally, not only for that node,
  so a module property of this type gets the same picker
  (`Engine/Plugins/Animation/ControlRig/Source/ControlRigEditor/Private/ControlRigEditorModule.cpp:202`).
- A native rig (a C++ `UControlRig` subclass with its own `Execute_Internal`) runs without a VM:
  the engine's FK Control Rig is one
  (`Engine/Plugins/Animation/ControlRig/Source/ControlRig/Public/Rigs/FKControlRig.h:41`).
- The engine sets a rig's initial bone transforms by name, on bones imported from a mesh, then
  asks for construction to run again (`UControlRig::SetBoneInitialTransformsFromRefSkeleton`,
  `Engine/Plugins/Animation/ControlRig/Source/ControlRig/Private/ControlRig.cpp:3801`).

## Decision drivers

1. A technical artist can drive part of a puppet from a rig they build in the Control Rig editor,
   with no C++.
2. The rig sees performer motion the way every other module does: through calibration and the
   hand-off offset, in the puppet's orientation, so it works with every capture source.
3. The adapter is one module among the others: it starts from the pose the modules before it
   left, writes only its own bones, and blends by its weight.
4. The core module does not depend on Control Rig (ADR 0002).
5. The module contract (ADR 0002, rule OP-006) stays as it is.

## Options considered

### Option A: performer data into the rig's controls (chosen)
- Pros: controls are what a rig author already builds and limits, and a rig can be tried in its
  editor by moving those controls by hand. They are elements of the rig's hierarchy, so the
  adapter finds them by name once and writes them each frame by index. A native rig can declare
  them, so the adapter is testable without assets.
- Cons: a rig made for animators may need controls added or renamed for puppetry.

### Option B: performer data into the rig's public variables
- Pros: the engine's Control Rig node exposes variables as pins, so the pattern is familiar.
- Cons: variables are found by property name and type, and are written through reflection.
  A native rig's own properties are not variables (`URigVMHost::GetExternalVariablesImpl` skips
  native properties), so the adapter could only be tested with a rig asset.

### Option C: the whole performer pose as rig bones
- Pros: no mapping to set up.
- Cons: the rig would have to contain a second skeleton (the performer's), and would receive raw
  performer transforms, without calibration or the hand-off offset (driver 2).

## Decision

1. **Module.** `UPuppetModule_ControlRig` ("Control Rig") in a new runtime module,
   `Open3DPuppetControlRig`, in the same plugin. It depends on the engine's `ControlRig` and
   `RigVM` modules; the plugin lists the `ControlRig` plugin. The core module is unchanged.
2. **Rig.** A `FControlRigAssetStrongReference`, picked like the engine node's. The rig's
   hierarchy holds bones named as the puppet's (a rig made on the puppet's mesh).
3. **Instance.** Each solver makes its own rig instance in `Bind`, following the engine node's
   pattern for creating one on a worker thread, and holds it with a `TStrongObjectPtr`. The
   module, which is shared configuration, holds none.
4. **Rest pose.** In `Bind`, every imported bone of the rig that the puppet has gets its initial
   transform from the puppet's rest pose (`FPuppetPose::GetReferenceComponent`, so a puppet's Rest
   Pose clip applies), and the rig's construction event runs once. Controls placed by
   construction start at the puppet's rest pose.
5. **Inputs.** A list of entries. Each one drives one rig control:
   - **Joint Rotation:** the control's initial rotation turned by the joint's rotation since
     calibration (`GetJointDelta`). The control must be a transform, Euler transform, or rotator
     control.
   - **Joint Transform:** the same rotation, plus the joint's move since calibration
     (`GetJointTranslation`), times the puppet's root scale and the entry's Scale. The control
     must be a transform or Euler transform control.
   - **Curve:** the curve's change from neutral (`GetCurveDelta`) times Scale. The control must be
     a float control.
   Joint data goes into the control's global transform; the rig decides what the control does.
6. **Pose in.** Before the rig runs, each rig bone that the puppet has takes the puppet's current
   component-space transform, so the rig starts from what the earlier modules and the input pose
   left.
7. **Output.** A list of puppet bones. After the forward solve, each listed bone takes the rig
   bone's global transform, parents before children, blended over the pose by the solve context's
   weight. Bones not listed are left untouched, so the adapter writes only its region.
8. **Per frame.** `Solve` locks the rig's evaluate mutex as the engine node does, writes the
   bones and controls through element indices cached in `Bind`, runs the forward solve event, and
   reads the output bones.
9. **Errors.** `Bind` fails, and the module is skipped, when no rig is set, the rig cannot be
   instanced, a control or output bone is missing, a control's type does not suit its input, or
   a performer joint or curve is missing.
10. **Contract.** No change to the module contract: `Bind` may allocate, and `Solve` reads the
    context's helpers like any module.

## Consequences

- A technical artist can drive a region of a puppet (a tail, a face, a prop) from a rig built in
  the Control Rig editor, alongside the C++ modules.
- The adapter's own per-frame code does not allocate. Whether a rig's forward solve allocates
  depends on the rig and on RigVM, which this plugin does not control. Profile a rig with Unreal
  Insights before a show.
- A rig made for animators may need puppetry controls added.
- The `ControlRig` plugin, enabled by default in the engine, is now a plugin dependency.

## Open questions for the maintainer

1. Is a list of control inputs (joint rotation, joint transform, curve) the right interface?
   Should rig variables also be inputs (Option B)?
2. Should rig curves also be outputs, written to puppet curves (morph targets)? Not in this
   version.
3. needs-verification: running construction once in `Bind`, then only the forward solve each
   frame. A rig whose construction depends on per-frame data would need the construction event
   again; the adapter does not run it again until the module binds again.
4. needs-verification: the adapter was tested with a native rig built in code. A rig asset made
   in the Control Rig editor has not been run through it; that needs a person at the desk. The
   adapter runs the Forwards Solve event only (`Execute`), where the engine node runs the rig's
   event queue (`Evaluate_AnyThread`), which also runs the Pre and Post Forwards Solve events: a
   rig that uses those would not run them here.
5. ADR 0002 question 2 (the Modular Control Rig and IK Retargeter assessments, from the 5.7
   design): this ADR read the 5.8 Control Rig runtime but not Modular Control Rig's assembly
   tools or the IK Retargeter, so it does not close that question.
