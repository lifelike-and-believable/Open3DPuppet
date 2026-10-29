// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_Fan.generated.h"

/**
 * Spreads a fan of bones, such as the Eagle's tail feathers (design overview §10), from the bend
 * of performer joints, averaged: with the elbows, the tail opens as the wings spread. Each
 * feather turns from its reference pose (the fan closed) by its place from the middle of the fan
 * outwards: the middle stays, the two edges turn the full Spread Angle, one each way.
 */
UCLASS(meta = (DisplayName = "Fan"))
class OPEN3DPUPPET_API UPuppetModule_Fan : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Performer joints, by canonical name, whose bends are averaged: the elbows (the forearms' bends) by default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	TArray<FName> PerformerJoints = { TEXT("lowerarm_l"), TEXT("lowerarm_r") };

	/** The fan's bones, in order across the fan from one edge to the other; they need not be listed from the middle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	TArray<FName> Feathers;

	/** The average bend, in degrees, at or below which the fan is fully open. The Wing's Fold Start Angle by default, so the tail opens with the wings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float OpenBend = 20.0f;

	/** The average bend at or above which the fan is closed (at its reference pose). The Wing's Fold End Angle by default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float ClosedBend = 120.0f;

	/** How far each edge feather turns when the fan is fully open, the first one way and the last the other; the feathers between turn by their place. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "90.0", Units = "deg"))
	float SpreadAngle = 30.0f;

	/** The axis the feathers spread about, in each feather's own space. Set for the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FVector SpreadAxis = FVector::ZAxisVector;

	/** Time constant of the smoothing, in seconds: the fan covers 63% of a change in this time. 0 follows the bend at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "0.5", Units = "s"))
	float SmoothingTime = 0.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
