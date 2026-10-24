# 0002: Puppet module architecture

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-24
- **Related:** [ADR 0003](0003-performer-input-and-calibration.md), [ADR 0004](0004-multiple-performers.md), [design overview](../design/overview.md) §3 to §5

## Context

The toolkit's premise is that a user attaches puppet modules to a skeletal mesh and puppeteers it
at once, and that technical artists can add modules for new anatomy. Something has to own the
module list, evaluate the performers and run the modules each frame. UE 5.7 (read from
`lifelike-and-believable/UnrealEngine`, branch `release`, 5.7.4) offers three engine mechanisms
that could carry it:

- **Modular Control Rig** (rig modules attached to a skeleton through connectors).
- **IK Retargeter** (chain mapping between two IK Rigs, with a stack of retarget operations).
- **`ULiveLinkRetargetAsset`**, the extension point of the stock Live Link Pose node:
  `BuildPoseFromAnimationData(DeltaTime, SkeletonData, FrameData, OutPose)`
  (`Engine/Source/Runtime/LiveLinkAnimationCore/Public/LiveLinkRetargetAsset.h:41`), chosen per
  node through `TSubclassOf<ULiveLinkRetargetAsset> RetargetAsset`
  (`Engine/Source/Runtime/LiveLinkAnimationCore/Public/AnimNode_LiveLinkPose.h:44`).

## Decision drivers

1. Several performers can drive one puppet (ADR 0004).
2. Modules are easy to write, test and ship as a library; the contract stays stable.
3. Retargeting controls are live: an edit shows up on the next frame.
4. Runs on the animation worker threads, no allocation per frame.
5. The solved pose is an ordinary AnimGraph pose (blend, layer, record).

## Options considered

### Option A: Modular Control Rig as the core
- Pros: the "attach modules to a skeleton" idea already exists; technical artists know Control Rig.
- Cons: the modular rig asset and its connector rules have changed between engine versions, so the
  plugin would follow Epic's churn; performer input and calibration still have to come from
  outside; multi-performer input means passing several Live Link poses into one rig; unit testing
  rig modules from C++ is awkward.

### Option B: IK Retargeter as the core
- Pros: chain mapping, IK goals and retarget ops already exist.
- Cons: one source skeleton to one target skeleton, so several performers do not fit; the source
  must be a skeletal mesh, not a Live Link subject; creature-specific behaviour (springs, wing
  folding) would be custom ops anyway.

### Option C: A `ULiveLinkRetargetAsset` subclass in the stock Live Link Pose node
- Pros: no AnimGraph node of our own.
- Cons: one subject per node (`LiveLinkSubjectName`, `AnimNode_LiveLinkPose.h:32`), so several
  performers do not fit; the retarget asset is chosen by class, so per-puppet settings have no
  natural home.

### Option D: Own orchestration (chosen)
A `UPuppetAsset` data asset holds the target mesh, the performer slots and an ordered list of
instanced `UPuppetModule` objects. One AnimGraph node, `FAnimNode_PuppetSolve`, evaluates each
slot's Live Link subject and runs the modules' solvers.
- Pros: meets every driver; small, testable contract; Control Rig and IK Retargeter can still be
  used inside a module (WP-10) without the core depending on them.
- Cons: more code of our own (the solver core, the node, an editor graph node).

## Decision

Option D.

1. **Asset.** `UPuppetAsset : UDataAsset` with `TargetMesh` (`TSoftObjectPtr<USkeletalMesh>`, for
   the editor; the solve uses the mesh it runs on), `PerformerSlots` (`TArray<FName>`, at least
   one), `RootSlot`, and `Modules` (`TArray<TObjectPtr<UPuppetModule>>`, `Instanced`).
2. **Module contract.**
   - `UPuppetModule` (`Abstract`, `EditInlineNew`, `DefaultToInstanced`): `bEnabled`,
     `PerformerSlot`, `Weight`, and `virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const`.
     It is configuration only: one instance is shared by every actor that uses the puppet.
   - `FPuppetModuleSolver`: `virtual bool Bind(const FPuppetBindContext&, FString& OutError)`,
     called when bones are cached and when a performer's skeleton changes; `virtual void
     Solve(FPuppetSolveContext&)`, called every frame on a worker thread; it must not allocate.
     The solver keeps a `const` pointer to its module and reads the settings each frame, so edits
     are live.
   - `FPuppetSolveContext` gives the solver the puppet pose (`FPuppetPose`, with local and
     component-space access), the slot's `FPuppetPerformerPose` and calibration, the root scale,
     and the delta time.
3. **Shared operations** (rotation delta, chain resampling, translation scaling, two-bone IK)
   live in `PuppetRetargetMath`, used by every module, so a new module composes them rather than
   reimplementing them.
4. **Order.** Modules run in list order on one pose; the asset's order is the author's.
5. **Node.** `FAnimNode_PuppetSolve` (runtime module) and `UAnimGraphNode_PuppetSolve` (editor
   module). The node has an input pose; bones no module writes pass through from it, and a module
   with weight below 1 blends over it.
6. **Changing the contract** (the virtuals above, `FPuppetSolveContext`, the pose types) needs a
   new ADR (rule OP-006).

## Consequences

- Writing a module is a C++ task: a settings class and a solver, plus tests. The authoring guide
  (`docs/guides/authoring-puppet-modules.md`, WP-1) walks through one. A Control Rig adapter
  module (WP-10) is the route for artists who would rather not write C++.
- Bones are named with `FName` in WP-1; a bone picker bound to the target mesh is WP-8.

## Open questions for the maintainer

1. Is a C++-first module contract acceptable, with the Control Rig adapter later (WP-10)?
2. needs-verification: the Modular Control Rig and IK Retargeter assessments above are from the
   5.7 design and from general knowledge of their history; they were not re-read against 5.8.
