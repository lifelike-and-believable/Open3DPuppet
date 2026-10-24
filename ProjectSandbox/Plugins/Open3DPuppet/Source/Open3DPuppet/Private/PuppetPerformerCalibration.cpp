// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformerCalibration.h"

#include "PuppetPerformerPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetPerformerCalibration)

FPuppetPerformerCalibration FPuppetPerformerCalibration::Capture(const FPuppetPerformerPose& Pose, const FRotator& InAlignment)
{
	FPuppetPerformerCalibration Calibration;
	Calibration.BoneNames = Pose.GetBoneNames();
	Calibration.RestComponent = Pose.GetComponentTransforms();
	// Derived curves are measured from the calibration itself, so their neutral is 0 (ADR 0009).
	Calibration.CurveNames = Pose.GetSourceCurveNames();
	Calibration.RestCurveValues = Pose.GetSourceCurveValues();
	Calibration.Alignment = InAlignment;
	return Calibration;
}

bool FPuppetPerformerCalibration::Matches(const FPuppetPerformerPose& Pose) const
{
	const TConstArrayView<FName> PoseNames = Pose.GetBoneNames();
	if (BoneNames.Num() != PoseNames.Num() || RestComponent.Num() != PoseNames.Num())
	{
		return false;
	}
	for (int32 Bone = 0; Bone < BoneNames.Num(); ++Bone)
	{
		if (BoneNames[Bone] != PoseNames[Bone])
		{
			return false;
		}
	}
	return true;
}

float FPuppetPerformerCalibration::GetRestCurve(FName CurveName) const
{
	const int32 Curve = CurveNames.IndexOfByKey(CurveName);
	return RestCurveValues.IsValidIndex(Curve) ? RestCurveValues[Curve] : 0.0f;
}
