// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LiveLinkTypes.h"
#include "PuppetNaming.h"

#include "PuppetPerformerProfile.generated.h"

class USkeleton;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile", meta = (ForceInlineRow, GetKeyOptions = "GetCanonicalJointOptions", GetValueOptions = "GetSourceBoneOptions"))
	TMap<FName, FName> CanonicalToSource;

	/**
	 * Canonical curve name to the source's curve name. The canonical curves are ARKit's 52 blend
	 * shapes, as Live Link Face sends them (for example "jawOpen" to a source's "JawOpen"); a curve
	 * this does not map is looked up by its own name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile", meta = (ForceInlineRow))
	TMap<FName, FName> CurveCanonicalToSource;

	/** A skeleton that names its bones as the source does (for example an imported take's), for Map From Skeleton. */
	UPROPERTY(EditAnywhere, Category = "Auto-Map")
	TSoftObjectPtr<USkeleton> SourceSkeleton;

	/** A Live Link subject streaming from the source, for Map From Live Link Subject. */
	UPROPERTY(EditAnywhere, Category = "Auto-Map")
	FLiveLinkSubjectName SourceSubject;

	/** What the last mapping found: the naming scheme and the joints the source lacks. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Auto-Map")
	FString MappingResult;

	/**
	 * Fills Canonical To Source from Source Skeleton's bones, with the naming scheme in Project
	 * Settings > Open3DPuppet that matches them best. The curve mapping is left as it is.
	 */
	UFUNCTION(CallInEditor, Category = "Auto-Map")
	void MapFromSkeleton();

	/** As Map From Skeleton, from the bones Source Subject streams (it needs a frame from Live Link). */
	UFUNCTION(CallInEditor, Category = "Auto-Map")
	void MapFromLiveLinkSubject();

	/**
	 * Replaces Canonical To Source with the mapping from the scheme that matches the source's bones
	 * best (PuppetNaming::Match), and says what it found in Mapping Result. False, with the mapping
	 * unchanged, when no scheme names any of the bones.
	 */
	bool MapBones(TConstArrayView<FName> SourceBones, TConstArrayView<int32> SourceParents, TConstArrayView<FPuppetNamingScheme> Schemes);

	/** The canonical joint names, for the mapping's keys in the details panel. */
	UFUNCTION()
	TArray<FName> GetCanonicalJointOptions() const;

	/** Source Skeleton's bone names, for the mapping's values in the details panel; empty without a skeleton. */
	UFUNCTION()
	TArray<FName> GetSourceBoneOptions() const;
};
