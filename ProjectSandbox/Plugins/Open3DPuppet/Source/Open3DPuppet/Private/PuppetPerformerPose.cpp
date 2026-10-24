// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformerPose.h"

#include "Algo/StableSort.h"
#include "PuppetPerformerProfile.h"

bool FPuppetPerformerPose::SetSkeleton(TConstArrayView<FName> InNames, TConstArrayView<int32> InParents, const UPuppetPerformerProfile* Profile, FString& OutError)
{
	Names.Reset();
	Parents.Reset();
	EvaluationOrder.Reset();
	CanonicalToBone.Reset();
	Local.Reset();
	Component.Reset();
	bHasTransforms = false;

	const int32 NumBones = InNames.Num();
	if (NumBones == 0)
	{
		OutError = TEXT("the performer skeleton has no bones");
		return false;
	}
	if (InParents.Num() != NumBones)
	{
		OutError = FString::Printf(TEXT("the performer skeleton has %d bone names but %d parent indices"), NumBones, InParents.Num());
		return false;
	}

	TArray<int32> Depths;
	Depths.SetNumZeroed(NumBones);
	for (int32 Bone = 0; Bone < NumBones; ++Bone)
	{
		const int32 Parent = InParents[Bone];
		if (Parent != INDEX_NONE && (Parent < 0 || Parent >= NumBones || Parent == Bone))
		{
			OutError = FString::Printf(TEXT("bone %d (%s) has an invalid parent index %d"), Bone, *InNames[Bone].ToString(), Parent);
			return false;
		}
		int32 Depth = 0;
		for (int32 Ancestor = Parent; Ancestor != INDEX_NONE; Ancestor = InParents[Ancestor])
		{
			if (++Depth > NumBones)
			{
				OutError = FString::Printf(TEXT("the parents of bone %d (%s) form a cycle"), Bone, *InNames[Bone].ToString());
				return false;
			}
		}
		Depths[Bone] = Depth;
	}

	Names = InNames;
	Parents = InParents;
	EvaluationOrder.SetNum(NumBones);
	for (int32 Bone = 0; Bone < NumBones; ++Bone)
	{
		EvaluationOrder[Bone] = Bone;
	}
	Algo::StableSortBy(EvaluationOrder, [&Depths](int32 Bone) { return Depths[Bone]; });

	for (int32 Bone = 0; Bone < NumBones; ++Bone)
	{
		CanonicalToBone.FindOrAdd(Names[Bone], Bone);
	}
	if (Profile)
	{
		for (const TPair<FName, FName>& Mapping : Profile->CanonicalToSource)
		{
			const int32 Bone = Names.IndexOfByKey(Mapping.Value);
			if (Bone != INDEX_NONE)
			{
				CanonicalToBone.Add(Mapping.Key, Bone);
			}
		}
	}

	Local.SetNum(NumBones);
	Component.SetNum(NumBones);
	return true;
}

bool FPuppetPerformerPose::HasSkeleton(TConstArrayView<FName> InNames, TConstArrayView<int32> InParents) const
{
	if (Names.Num() != InNames.Num() || Parents.Num() != InParents.Num())
	{
		return false;
	}
	for (int32 Bone = 0; Bone < Names.Num(); ++Bone)
	{
		if (Names[Bone] != InNames[Bone] || Parents[Bone] != InParents[Bone])
		{
			return false;
		}
	}
	return true;
}

bool FPuppetPerformerPose::SetLocalTransforms(TConstArrayView<FTransform> InLocal)
{
	if (InLocal.Num() != Names.Num() || Names.Num() == 0)
	{
		return false;
	}
	for (const int32 Bone : EvaluationOrder)
	{
		Local[Bone] = InLocal[Bone];
		const int32 Parent = Parents[Bone];
		Component[Bone] = Parent == INDEX_NONE ? Local[Bone] : Local[Bone] * Component[Parent];
	}
	bHasTransforms = true;
	return true;
}

int32 FPuppetPerformerPose::FindJoint(FName CanonicalJoint) const
{
	const int32* Bone = CanonicalToBone.Find(CanonicalJoint);
	return Bone ? *Bone : INDEX_NONE;
}

void FPuppetPerformerPose::SetCurveNames(TConstArrayView<FName> InCurveNames, const UPuppetPerformerProfile* Profile)
{
	CurveNames = InCurveNames;
	CurveValues.SetNumZeroed(CurveNames.Num());
	bHasCurveValues = false;

	CanonicalToCurve.Reset();
	for (int32 Curve = 0; Curve < CurveNames.Num(); ++Curve)
	{
		CanonicalToCurve.FindOrAdd(CurveNames[Curve], Curve);
	}
	if (Profile)
	{
		for (const TPair<FName, FName>& Mapping : Profile->CurveCanonicalToSource)
		{
			const int32 Curve = CurveNames.IndexOfByKey(Mapping.Value);
			if (Curve != INDEX_NONE)
			{
				CanonicalToCurve.Add(Mapping.Key, Curve);
			}
		}
	}
}

bool FPuppetPerformerPose::HasCurveNames(TConstArrayView<FName> InCurveNames) const
{
	if (CurveNames.Num() != InCurveNames.Num())
	{
		return false;
	}
	for (int32 Curve = 0; Curve < CurveNames.Num(); ++Curve)
	{
		if (CurveNames[Curve] != InCurveNames[Curve])
		{
			return false;
		}
	}
	return true;
}

bool FPuppetPerformerPose::SetCurveValues(TConstArrayView<float> InValues)
{
	if (InValues.Num() != CurveNames.Num())
	{
		return false;
	}
	for (int32 Curve = 0; Curve < InValues.Num(); ++Curve)
	{
		CurveValues[Curve] = InValues[Curve];
	}
	bHasCurveValues = InValues.Num() > 0;
	return true;
}

int32 FPuppetPerformerPose::FindCurve(FName CanonicalCurve) const
{
	const int32* Curve = CanonicalToCurve.Find(CanonicalCurve);
	return Curve ? *Curve : INDEX_NONE;
}
