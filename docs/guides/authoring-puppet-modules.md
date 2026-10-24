# Authoring puppet modules

This guide is for anyone adding a module to the Open3DPuppet library, or writing one in their own
project or plugin. It covers the module contract ([ADR 0002](../adr/0002-puppet-module-architecture.md)),
a worked example, the shared retargeting math, the tests every module needs, and a checklist for
the pull request. Read the [design overview](../design/overview.md) first, sections 3 to 5.

## 1. Before you write one

Check whether an existing module already does the job with different settings:

| Need | Use |
|---|---|
| Any FK chain (spine, neck, a tail that copies the spine, antennae) | Chain, with `Amplification` to damp or exaggerate |
| An arm or leg with a hand or foot to place | Two-Bone Limb, choosing an `EffectorMode` |
| The pelvis and the puppet's place in the world | Root |

Write a new module when the anatomy needs behaviour these cannot express: a third limb segment
(digitigrade legs), state between frames (springs on a tail), a mapping between different body
parts (arms to wings), or a different input (a tracked prop).

## 2. The contract

A module is two pieces, in one header and one source file:

- **Settings**, a `UPuppetModule` subclass. It is instanced inside a `UPuppetAsset` and shared by
  every actor that uses that puppet. It holds the editable controls and `CreateSolver()`, and no
  per-frame state.
- **Solver**, an `FPuppetModuleSolver` subclass, usually through `TPuppetModuleSolver<YourModule>`,
  which keeps a reference to the settings as `Settings`. One solver exists per running Puppet
  Solve node. It caches indices and any state between frames.

The solver has two functions:

| Function | When | Rules |
|---|---|---|
| `bool Bind(const FPuppetBindContext&, FString& OutError)` | When the puppet's skeleton, the performer's skeleton, the calibration or the asset's settings change | Resolve every bone and joint name to an index and check that it exists. Return `false` with a sentence in `OutError` if the module cannot run; the instance logs it once and skips the module until the next bind. Precompute anything that depends only on rest poses. Size every array `Solve` will use. May allocate. |
| `void Solve(FPuppetSolveContext&)` | Every frame, on an animation worker thread | Read the performer through the context; write only the puppet bones the module claims. No allocation, no logging, no game-thread objects. Apply `Settings.Weight`. |

`FPuppetBindContext` and `FPuppetSolveContext` carry:

| Member | Meaning |
|---|---|
| `Puppet` | `FPuppetPose`: the puppet's bones by mesh index. `FindBone`, `GetChain`, `GetReferenceComponent` (rest), `GetComponent` and `GetLocal` (current), and in `Solve`: `SetComponent`, `SetComponentRotation`. |
| `Performer` | `FPuppetPerformerPose`: `FindJoint(canonical name)`, `GetParents`, `GetComponent`. |
| `Calibration` | `FPuppetPerformerCalibration`: `RestComponent[joint]`, the performer's rest pose. It always matches `Performer`'s skeleton. |
| `PuppetPelvis`, `PerformerPelvis`, `RootScale` | The pelvis on each side and the size ratio between them (puppet pelvis height over performer pelvis height). |
| `bOwnsRoot` (bind only) | Whether the module's slot owns the root (ADR 0004). Only such a module may move the puppet in the world. |
| `Alignment`, `DeltaTime` (solve only) | The calibration's facing correction, and the frame time for modules with state. |

Solve-context helpers do the alignment for you: `GetJointDelta(joint)` (the joint's rotation change
since calibration, in puppet space), `GetJointPosition(joint)` and `GetJointTranslation(joint)`
(position, and change in position since calibration). Use them rather than reading `Performer` and
`Calibration` directly, so that alignment is applied the same way everywhere.

### Rules

1. **Canonical joints.** Name performer joints the way the UE5 mannequin names its bones
   (`pelvis`, `spine_01` to `spine_05`, `neck_01`, `neck_02`, `head`, `clavicle_l`, `upperarm_l`,
   `lowerarm_l`, `hand_l`, `thigh_l`, `calf_l`, `foot_l`, `ball_l`, and `_r`; fingers
   `thumb_01_l` to `thumb_03_l`, then `index_metacarpal_l` and `index_01_l` to `index_03_l`, and
   the same for `middle`, `ring` and `pinky`). Fingers arrive as ordinary Animation-role bones, in
   the body subject or a hand subject of their own (maintainer, 2026-10-24). Performer profiles
   map other sources onto these names (ADR 0003). Use the same names as the defaults for puppet
   bones, so a mannequin-like mesh works without setup.
2. **Claim your bones.** Write only the bones your settings name. Bones you do not write keep
   their local transforms and follow their parents, which is how fingers follow a hand that a
   limb module placed.
3. **Write parents before children,** and capture any "before" state you blend against before the
   first write: writing a bone moves all its descendants.
4. **Blend with `Settings.Weight`** over the pose you received, so a puppeteer can fade the module.
5. **Use the shared math** (section 4). If you need an operation that is not there and another
   module could use it, add it to `PuppetRetarget` with its own tests.
6. **Controls are `UPROPERTY`s** with `EditAnywhere`, `BlueprintReadOnly`, a `Category`
   (`Performer` for performer joints, `Puppet` for puppet bones, `Retarget` for retargeting
   controls), a tooltip comment, and `ClampMin`/`ClampMax` where a range makes sense. The solver
   reads them every frame, so edits are live; editing them also re-binds the puppet.
7. **Changing the contract** (the virtuals above, the context structs, the pose types) needs an
   ADR. A new module never needs one.

## 3. Worked example: a jaw or ear

A module that drives one puppet bone from one performer joint, with an amplification and an
offset. It is small enough to show in full; the library's Chain module is the next step up.

`Source/Open3DPuppet/Public/PuppetModules/PuppetModule_SingleBone.h`:

```cpp
// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_SingleBone.generated.h"

/** Drives one puppet bone from one performer joint, for jaws, ears and other single bones. */
UCLASS(meta = (DisplayName = "Single Bone"))
class OPEN3DPUPPET_API UPuppetModule_SingleBone : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** The performer joint, by canonical name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerJoint = TEXT("head");

	/** The puppet bone it drives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetBone = TEXT("head");

	/** Scales the performer's rotation: below 1 damps it, above 1 exaggerates it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float Amplification = 1.0f;

	/** Added after retargeting, in the bone's component space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FRotator Offset = FRotator::ZeroRotator;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
```

`Source/Open3DPuppet/Private/PuppetModules/PuppetModule_SingleBone.cpp`:

```cpp
// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_SingleBone.h"

#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_SingleBone)

namespace PuppetModuleSingleBone
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_SingleBone>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_SingleBone>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			Joint = Context.Performer.FindJoint(Settings.PerformerJoint);
			if (Joint == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *Settings.PerformerJoint.ToString());
				return false;
			}
			Bone = Context.Puppet.FindBone(Settings.PuppetBone);
			if (Bone == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("the puppet has no bone '%s'"), *Settings.PuppetBone.ToString());
				return false;
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			const FQuat Delta = PuppetRetarget::ScaleRotation(Context.GetJointDelta(Joint), Settings.Amplification);
			const FQuat Target = Settings.Offset.Quaternion() * Delta * Context.Puppet.GetReferenceComponent(Bone).GetRotation();
			const FQuat Original = Context.Puppet.GetComponent(Bone).GetRotation();
			Context.Puppet.SetComponentRotation(Bone, FQuat::Slerp(Original, Target.GetNormalized(), Settings.Weight));
		}

	private:
		int32 Joint = INDEX_NONE;
		int32 Bone = INDEX_NONE;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_SingleBone::CreateSolver() const
{
	return MakeUnique<PuppetModuleSingleBone::FSolver>(*this);
}
```

That is the whole module: it appears in the puppet asset's module list (the list offers every
non-abstract `UPuppetModule` subclass), and the Puppet Solve node runs it.

### Modules with state

A module that remembers something between frames (a spring, a filter) keeps it in the solver:
size it in `Bind`, reset it there, and advance it in `Solve` with `Context.DeltaTime`. Never keep
it in the settings object, which every actor shares.

### Modules outside this plugin

`UPuppetModule`, `FPuppetModuleSolver` and the context types are exported (`OPEN3DPUPPET_API`), so
a game project or another plugin can define modules: add `Open3DPuppet` to the module's
`PublicDependencyModuleNames` (or private, if no public header of yours includes ours) and replace
`OPEN3DPUPPET_API` with your module's API macro. Modules that would help other users belong in the
library; open a pull request.

## 4. Shared math

`PuppetRetargetMath.h`, namespace `PuppetRetarget`:

| Function | Use it for |
|---|---|
| `RotationDelta(Current, Rest, Alignment)` | A joint's rotation change since calibration. `FPuppetSolveContext::GetJointDelta` calls it. Apply as `Delta * PuppetRest`. |
| `ScaleRotation(Delta, Amount)` | Damping or exaggerating a rotation about its own axis. |
| `ChainPositions(JointPositions, OutPositions)` | Normalized positions along a chain, for mapping chains of different lengths. Compute in `Bind` from rest poses. |
| `ChainLength(JointPositions)` | Limb and chain lengths, for reach scaling. |
| `SampleChain(Rotations, Positions, T)` | The rotation at a normalized position along a chain. |
| `CollectChain(Parents, Start, End, OutChain)` | Resolving a chain on the performer (`FPuppetPose::GetChain` does the same for the puppet). |

For two-bone IK, use `AnimationCore::SolveTwoBoneIK` (`TwoBoneIK.h`) as the Two-Bone Limb module
does. Give it a pole that rides on the upper bone (computed in `Bind` from the reference pose's
bend, or a Pole Direction setting when the reference limb is straight), not the forward-kinematics
joint alone: a nearly straight limb makes that pole colinear and the bend direction arbitrary.

## 5. Tests

Every module has solver tests in
`Source/Open3DPuppetTests/Private/Modules/PuppetModule<Name>Tests.cpp`, named
`Open3DPuppet.Modules.<Name>.<Case>` and wrapped in `#if WITH_DEV_AUTOMATION_TESTS`
([ADR 0005](../adr/0005-test-layout.md)). Follow `PuppetModuleChainTests.cpp`.

The fixtures in `Harness/PuppetTestFixtures.h` build everything in code:

- `PuppetTests::Biped()`, `Tail()`, `Scaled(...)` and `WithOffset(...)` give skeleton tables; add
  one for your anatomy (for example a digitigrade leg) next to them.
- `PuppetTests::FPuppet` holds a puppet asset, a reference skeleton and an `FPuppetInstance`.
  `Add<T>()` adds a module and returns it for setup; `Build()`, `Drive(slot, performer)`,
  `Calibrate(slot, performer)` and `Solve()` run it; `Component(bone)` and `Reference(bone)` read
  the result.
- `PuppetTests::FPerformer` is a performer as Live Link delivers it; `SetLocalRotation` and
  `SetLocalTranslation` pose it.
- `PuppetTests::TestRotation` compares rotations up to quaternion sign.

A typical test calibrates at rest, poses the performer, solves and checks the puppet:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSingleBoneFollowsTest, "Open3DPuppet.Modules.SingleBone.FollowsTheJoint", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSingleBoneFollowsTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Add<UPuppetModule_SingleBone>();
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);

	const FQuat Nod = PuppetTests::Rotation(FVector::YAxisVector, 20.0);
	Performer.SetLocalRotation(TEXT("head"), Nod);
	Puppet.Drive(0, Performer);
	Puppet.Solve();

	PuppetTests::TestRotation(*this, TEXT("head"), Puppet.Component(TEXT("head")).GetRotation(), Nod);
	return true;
}
```

Each module tests at least:

1. The calibrated rest pose maps to the puppet's reference pose.
2. A known performer motion produces the expected puppet pose, with values you can work out by
   hand (90 and 45 degree turns, round lengths). Write the reasoning in a comment.
3. Each retargeting control changes the result as its tooltip says.
4. `Weight` below 1 blends over the input.
5. A missing bone or joint: `Bind` fails with a clear message, and the bones stay as the input.
   Declare the warning with `AddExpectedMessagePlain(..., ELogVerbosity::Warning)`.

Write the tests first and watch them fail (rule TEST-004). Raise `plugin` in
`Build/automation-test-floors.json` by the number of tests you add.

## 6. Pull request checklist

- [ ] Header in `Public/PuppetModules/`, source in `Private/PuppetModules/`, class `UPuppetModule_<Name>`, display name set.
- [ ] Copyright line on both files (rule OP-002).
- [ ] `Bind` checks every name and sizes every array; `Solve` does not allocate, log or touch game-thread objects.
- [ ] Every control has a category, a tooltip comment and a sensible clamp.
- [ ] Tests from section 5, the floor raised, the UE build and the `Open3DPuppet` tests run with their output in the PR.
- [ ] The module added to the design overview's table (§5.2) and to `CHANGELOG.md`; its roadmap work package marked done.
