// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPose.h"

#include "PuppetRetargetMath.h"
#include "ReferenceSkeleton.h"

void FPuppetPose::Initialize(const FReferenceSkeleton& RefSkeleton, TConstArrayView<FTransform> RestLocal)
{
	const int32 NumBones = RefSkeleton.GetNum();
	Names.SetNum(NumBones);
	Parents.SetNum(NumBones);
	Local = RefSkeleton.GetRefBonePose();
	Component.SetNum(NumBones);
	for (int32 Bone = 0; Bone < NumBones; ++Bone)
	{
		Names[Bone] = RefSkeleton.GetBoneName(Bone);
		Parents[Bone] = RefSkeleton.GetParentIndex(Bone);
	}
	PropagateFrom(0);
	ReferenceComponent = Component;
}

void FPuppetPose::SetLocalTransforms(TConstArrayView<FTransform> InLocal)
{
	check(InLocal.Num() == Local.Num());
	for (int32 Bone = 0; Bone < Local.Num(); ++Bone)
	{
		Local[Bone] = InLocal[Bone];
	}
	PropagateFrom(0);
}

int32 FPuppetPose::FindBone(FName Name) const
{
	return Names.IndexOfByKey(Name);
}

void FPuppetPose::SetComponent(int32 Bone, const FTransform& Transform)
{
	const int32 Parent = Parents[Bone];
	Component[Bone] = Transform;
	Local[Bone] = Parent == INDEX_NONE ? Transform : Transform.GetRelativeTransform(Component[Parent]);
	PropagateFrom(Bone + 1);
}

void FPuppetPose::SetComponentRotation(int32 Bone, const FQuat& Rotation)
{
	const int32 Parent = Parents[Bone];
	const FQuat ParentRotation = Parent == INDEX_NONE ? FQuat::Identity : Component[Parent].GetRotation();
	Local[Bone].SetRotation((ParentRotation.Inverse() * Rotation).GetNormalized());
	PropagateFrom(Bone);
}

bool FPuppetPose::GetChain(int32 Start, int32 End, TArray<int32>& OutChain) const
{
	return PuppetRetarget::CollectChain(Parents, Start, End, OutChain);
}

void FPuppetPose::PropagateFrom(int32 FirstChanged)
{
	// A reference skeleton orders parents before children, so one forward pass is enough.
	for (int32 Bone = FirstChanged; Bone < Local.Num(); ++Bone)
	{
		const int32 Parent = Parents[Bone];
		Component[Bone] = Parent == INDEX_NONE ? Local[Bone] : Local[Bone] * Component[Parent];
	}
}

void FPuppetPose::SetCurve(FName Name, float Value)
{
	const int32 Curve = CurveNames.IndexOfByKey(Name);
	if (Curve == INDEX_NONE)
	{
		CurveNames.Add(Name);
		CurveValues.Add(Value);
	}
	else
	{
		CurveValues[Curve] = Value;
	}
}

const float* FPuppetPose::FindCurve(FName Name) const
{
	const int32 Curve = CurveNames.IndexOfByKey(Name);
	return Curve == INDEX_NONE ? nullptr : &CurveValues[Curve];
}

void FPuppetPose::ResetCurves()
{
	CurveNames.Reset();
	CurveValues.Reset();
}
