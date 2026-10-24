// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModule.h"

#include "PuppetAsset.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule)

FQuat FPuppetSolveContext::GetJointDelta(int32 Joint) const
{
	return PuppetRetarget::RotationDelta(Performer.GetComponent(Joint).GetRotation(), Calibration.RestComponent[Joint].GetRotation(), Alignment);
}

FQuat FPuppetSolveContext::GetJointDeltaInParent(int32 Joint) const
{
	const int32 Parent = Performer.GetParents()[Joint];
	if (Parent == INDEX_NONE)
	{
		return GetJointDelta(Joint);
	}
	return PuppetRetarget::RotationDeltaInParent(Performer.GetComponent(Joint).GetRotation(), Calibration.RestComponent[Joint].GetRotation(),
		Performer.GetComponent(Parent).GetRotation(), Calibration.RestComponent[Parent].GetRotation(), Alignment);
}

float FPuppetSolveContext::GetCurve(int32 Curve) const
{
	return Performer.GetCurveValues()[Curve];
}

float FPuppetSolveContext::GetCurveDelta(int32 Curve) const
{
	return GetCurve(Curve) - (CurveNeutral.IsValidIndex(Curve) ? CurveNeutral[Curve] : 0.0f);
}

FVector FPuppetSolveContext::GetJointPosition(int32 Joint) const
{
	return Alignment.RotateVector(Performer.GetComponent(Joint).GetTranslation());
}

FVector FPuppetSolveContext::GetJointTranslation(int32 Joint) const
{
	return Alignment.RotateVector(Performer.GetComponent(Joint).GetTranslation() - Calibration.RestComponent[Joint].GetTranslation());
}

#if WITH_EDITOR
void UPuppetModule::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (UPuppetAsset* Asset = GetTypedOuter<UPuppetAsset>())
	{
		Asset->MarkChanged();
	}
}
#endif
