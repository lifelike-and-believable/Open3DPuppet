// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformerCalibration.h"

#include "PuppetPerformerPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetPerformerCalibration)

FPuppetPerformerCalibration FPuppetPerformerCalibration::Capture(const FPuppetPerformerPose& Pose, const FRotator& Alignment)
{
	FPuppetPerformerCalibration Calibration;
	Calibration.BoneNames = Pose.GetBoneNames();
	Calibration.RestComponent = Pose.GetComponentTransforms();
	Calibration.Alignment = Alignment;
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
