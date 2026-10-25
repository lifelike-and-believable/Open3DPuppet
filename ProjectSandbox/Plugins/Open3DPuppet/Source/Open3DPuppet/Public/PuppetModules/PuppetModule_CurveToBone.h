// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_CurveToBone.generated.h"

/**
 * Turns and moves one puppet bone from one performer curve (ADR 0006): a jaw or beak from jawOpen,
 * ears or a crest from brow curves, eyelids from blinks, a talon from a finger curl (ADR 0009).
 * The curve's change from neutral, remapped through the input range, sets how much of Rotation and
 * Translation the bone takes from its reference pose.
 */
UCLASS(meta = (DisplayName = "Curve to Bone"))
class OPEN3DPUPPET_API UPuppetModule_CurveToBone : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** The performer curve, by canonical name (ARKit names, or curl_index_l and the other finger curls). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerCurve = TEXT("jawOpen");

	/** The puppet bone it drives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetBone = TEXT("jaw");

	/** The curve's change from neutral that leaves the bone at its reference pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	float InputMin = 0.0f;

	/** The curve's change from neutral that gives the full Rotation and Translation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	float InputMax = 1.0f;

	/** The first part of the input range (0 to 1) that still reads as neutral, so small twitches do not move the bone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float DeadZone = 0.0f;

	/** The bone's turn at InputMax, about its own axes, from its reference rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FRotator Rotation = FRotator(-20.0, 0.0, 0.0);

	/** The bone's move at InputMax, in its parent's space, from its reference position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FVector Translation = FVector::ZeroVector;

	/** Time constant of the smoothing, in seconds: the bone covers 63% of a change in this time. 0 follows the curve at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "0.5", Units = "s"))
	float SmoothingTime = 0.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
