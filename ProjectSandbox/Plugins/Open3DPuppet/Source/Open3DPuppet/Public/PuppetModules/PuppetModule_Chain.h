// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_Chain.generated.h"

/**
 * Drives a chain of puppet bones (spine, neck, head, or any FK chain) from a chain of performer
 * joints. The chains need not have the same number of bones: each puppet bone follows the
 * performer's rotation at the same relative position along the chain (design overview §4).
 */
UCLASS(meta = (DisplayName = "Chain (FK)"))
class OPEN3DPUPPET_API UPuppetModule_Chain : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** First performer joint of the chain, by canonical name (for example spine_01). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerStart = TEXT("spine_01");

	/** Last performer joint of the chain (for example spine_05); a descendant of PerformerStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	FName PerformerEnd = TEXT("spine_05");

	/** First puppet bone of the chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetStart = TEXT("spine_01");

	/** Last puppet bone of the chain; a descendant of PuppetStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetEnd = TEXT("spine_05");

	/** Scales the performer's rotations: below 1 damps the motion, above 1 exaggerates it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float Amplification = 1.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
