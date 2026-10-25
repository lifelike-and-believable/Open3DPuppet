// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_Wing.generated.h"

/**
 * Drives a wing from the performer's arm (design overview §10, the Eagle): the wing root flaps
 * with the upper arm, relative to the chest, and the wing folds as the performer bends the elbow.
 * Folding turns the forearm and hand towards their fold poses and closes the feathers like a fan.
 */
UCLASS(meta = (DisplayName = "Wing"))
class OPEN3DPUPPET_API UPuppetModule_Wing : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Performer upper arm, by canonical name: its turn relative to the chest flaps the wing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerUpper = TEXT("upperarm_l");

	/** Performer forearm: its bend at the elbow folds the wing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerLower = TEXT("lowerarm_l");

	/** Puppet wing root (humerus). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetUpper = TEXT("upperarm_l");

	/** Puppet wing forearm; a child of PuppetUpper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetLower = TEXT("lowerarm_l");

	/** Puppet wing hand; a child of PuppetLower. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetHand = TEXT("hand_l");

	/** Feather bones, in fan order: the first stays as it is, the last closes by the full Feather Fold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	TArray<FName> Feathers;

	/** Scales the flap (the upper arm's turn relative to the chest): below 1 calms it, above 1 exaggerates it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float FlapAmplification = 1.0f;

	/** The performer's elbow bend, in degrees, at which the wing starts to fold; below it the wing is spread. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float FoldStartAngle = 20.0f;

	/** The elbow bend at which the wing is fully folded. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float FoldEndAngle = 120.0f;

	/** The forearm's turn when fully folded, about its own axes, from its reference rotation. Set for the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FRotator ForearmFold = FRotator(0.0, -120.0, 0.0);

	/** The hand's turn when fully folded, about its own axes, from its reference rotation. Set for the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FRotator HandFold = FRotator(0.0, 140.0, 0.0);

	/** The last feather's turn when fully folded, about its own axes; the feathers before it turn by their share of the fan. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FRotator FeatherFold = FRotator(0.0, -20.0, 0.0);

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
