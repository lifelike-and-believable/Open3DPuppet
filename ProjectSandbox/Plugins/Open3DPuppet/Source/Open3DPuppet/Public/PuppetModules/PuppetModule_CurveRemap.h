// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_CurveRemap.generated.h"

/** One performer curve onto one puppet curve. */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetCurveMapping
{
	GENERATED_BODY()

	/** The performer curve, by canonical name (ARKit names, or curl_index_l and the other finger curls). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerCurve;

	/** The puppet curve it writes: a morph target or a material parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetCurve;

	/** The performer curve's change from neutral that writes OutputMin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	float InputMin = 0.0f;

	/** The performer curve's change from neutral that writes OutputMax. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	float InputMax = 1.0f;

	/** The first part of the input range (0 to 1) that still reads as neutral. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float DeadZone = 0.0f;

	/** The puppet curve's value at InputMin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	float OutputMin = 0.0f;

	/** The puppet curve's value at InputMax. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	float OutputMax = 1.0f;
};

/**
 * Drives puppet curves (morph targets, material parameters) from performer curves (ADR 0006): a
 * wolf's snarl from noseSneer and mouthUpperUp, an eagle's blink from eyeBlink. Mappings that write
 * the same puppet curve add up, so one puppet shape can combine several performer curves.
 */
UCLASS(meta = (DisplayName = "Curve Remap"))
class OPEN3DPUPPET_API UPuppetModule_CurveRemap : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** The mappings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curves", meta = (TitleProperty = "PuppetCurve"))
	TArray<FPuppetCurveMapping> Mappings;

	/** Time constant of the smoothing of each puppet curve, in seconds. 0 follows the performer at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "0.5", Units = "s"))
	float SmoothingTime = 0.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
