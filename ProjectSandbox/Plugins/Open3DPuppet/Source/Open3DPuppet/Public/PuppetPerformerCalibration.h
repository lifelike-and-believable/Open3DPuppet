// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "UObject/ObjectMacros.h"

#include "PuppetPerformerCalibration.generated.h"

class FPuppetPerformerPose;

/**
 * A performer's rest pose, captured while the performer stands in the reference pose, and the
 * rotation that turns the performer's facing into the puppet's (ADR 0003). Retargeting measures
 * the performer's motion as rotation and translation changes from this rest pose.
 */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetPerformerCalibration
{
	GENERATED_BODY()

	/** The performer skeleton's bone names when the calibration was captured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	TArray<FName> BoneNames;

	/** Component-space rest transform of each bone, in the order of BoneNames. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	TArray<FTransform> RestComponent;

	/** Rotation from the performer's component space to the puppet's (for example a yaw when the performer faced another way while calibrating). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	FRotator Alignment = FRotator::ZeroRotator;

	/** A calibration of Pose's current transforms. */
	static FPuppetPerformerCalibration Capture(const FPuppetPerformerPose& Pose, const FRotator& Alignment);

	/** True when this calibration was captured from a skeleton with Pose's bone names, in the same order. */
	bool Matches(const FPuppetPerformerPose& Pose) const;
};
