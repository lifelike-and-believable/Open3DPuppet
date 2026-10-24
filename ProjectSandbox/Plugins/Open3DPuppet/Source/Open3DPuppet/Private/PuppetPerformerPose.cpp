// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformerPose.h"

#include "Algo/StableSort.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerProfile.h"
#include "PuppetRetargetMath.h"

namespace PuppetPerformerPose
{
	const TCHAR* const Fingers[] = { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
	const TCHAR* const Sides[] = { TEXT("l"), TEXT("r") };
	/** The bend that makes a curl of 1: each of a finger's three joints bent 90 degrees. */
	constexpr double FullCurl = 3.0 * UE_HALF_PI;
}

bool FPuppetPerformerPose::SetSkeleton(TConstArrayView<FName> InNames, TConstArrayView<int32> InParents, const UPuppetPerformerProfile* Profile, FString& OutError)
{
	Names.Reset();
	Parents.Reset();
	EvaluationOrder.Reset();
	CanonicalToBone.Reset();
	Local.Reset();
	Component.Reset();
	bHasTransforms = false;
	RebuildDerivedCurves();

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
	RebuildDerivedCurves();
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
	NumSourceCurves = CurveNames.Num();
	CurveValues.Reset();
	CurveValues.SetNumZeroed(NumSourceCurves);
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
	RebuildDerivedCurves();
}

bool FPuppetPerformerPose::HasCurveNames(TConstArrayView<FName> InCurveNames) const
{
	if (NumSourceCurves != InCurveNames.Num())
	{
		return false;
	}
	for (int32 Curve = 0; Curve < NumSourceCurves; ++Curve)
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
	if (InValues.Num() != NumSourceCurves)
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

TConstArrayView<FName> FPuppetPerformerPose::GetSourceCurveNames() const
{
	return TConstArrayView<FName>(CurveNames.GetData(), NumSourceCurves);
}

TConstArrayView<float> FPuppetPerformerPose::GetSourceCurveValues() const
{
	return TConstArrayView<float>(CurveValues.GetData(), NumSourceCurves);
}

void FPuppetPerformerPose::RebuildDerivedCurves()
{
	for (int32 Curve = NumSourceCurves; Curve < CurveNames.Num(); ++Curve)
	{
		CanonicalToCurve.Remove(CurveNames[Curve]);
	}
	CurveNames.SetNum(NumSourceCurves);
	CurveValues.SetNum(NumSourceCurves);
	FingerCurls.Reset();

	for (const TCHAR* Side : PuppetPerformerPose::Sides)
	{
		for (const TCHAR* Finger : PuppetPerformerPose::Fingers)
		{
			const FName Name(*FString::Printf(TEXT("curl_%s_%s"), Finger, Side));
			if (CanonicalToCurve.Contains(Name))
			{
				continue;
			}
			FFingerCurl Curl;
			bool bHasJoints = true;
			for (int32 Joint = 0; Joint < 3; ++Joint)
			{
				Curl.Joints[Joint] = FindJoint(*FString::Printf(TEXT("%s_%02d_%s"), Finger, Joint + 1, Side));
				bHasJoints &= Curl.Joints[Joint] != INDEX_NONE;
			}
			if (bHasJoints)
			{
				FingerCurls.Add(Curl);
				CanonicalToCurve.Add(Name, CurveNames.Add(Name));
				CurveValues.Add(0.0f);
			}
		}
	}
}

void FPuppetPerformerPose::UpdateDerivedCurves(const FPuppetPerformerCalibration& Calibration)
{
	if (!bHasTransforms || Calibration.RestComponent.Num() != Names.Num())
	{
		return;
	}
	for (int32 Index = 0; Index < FingerCurls.Num(); ++Index)
	{
		double Bend = 0.0;
		for (const int32 Joint : FingerCurls[Index].Joints)
		{
			const int32 Parent = Parents[Joint];
			const FQuat Current = Component[Joint].GetRotation();
			const FQuat Rest = Calibration.RestComponent[Joint].GetRotation();
			FQuat Delta = Parent == INDEX_NONE
				? PuppetRetarget::RotationDelta(Current, Rest, FQuat::Identity)
				: PuppetRetarget::RotationDeltaInParent(Current, Rest, Component[Parent].GetRotation(), Calibration.RestComponent[Parent].GetRotation(), FQuat::Identity);
			// The shortest arc, so a small bend reads as small whichever sign the quaternion has.
			if (Delta.W < 0.0)
			{
				Delta = -Delta;
			}
			Bend += Delta.GetAngle();
		}
		CurveValues[NumSourceCurves + Index] = static_cast<float>(Bend / PuppetPerformerPose::FullCurl);
	}
}
