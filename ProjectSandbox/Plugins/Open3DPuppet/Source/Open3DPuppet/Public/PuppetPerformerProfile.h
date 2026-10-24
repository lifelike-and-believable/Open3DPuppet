// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PuppetPerformerProfile.generated.h"

/**
 * Maps canonical joint names to a capture source's own bone names (ADR 0003). Puppet modules name
 * performer joints the way the UE5 mannequin names its bones; a source that uses other names
 * needs a profile. A canonical joint the profile does not map is looked up by its own name.
 */
UCLASS(BlueprintType)
class OPEN3DPUPPET_API UPuppetPerformerProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Canonical joint name (for example "upperarm_l") to the source's bone name (for example "LeftArm"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile", meta = (ForceInlineRow))
	TMap<FName, FName> CanonicalToSource;

	/**
	 * Canonical curve name to the source's curve name. The canonical curves are ARKit's 52 blend
	 * shapes, as Live Link Face sends them (for example "jawOpen" to a source's "JawOpen"); a curve
	 * this does not map is looked up by its own name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile", meta = (ForceInlineRow))
	TMap<FName, FName> CurveCanonicalToSource;
};
