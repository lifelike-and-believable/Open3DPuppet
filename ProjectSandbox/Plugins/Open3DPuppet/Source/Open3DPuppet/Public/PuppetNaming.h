// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"

#include "PuppetNaming.generated.h"

/**
 * How one capture system names its skeleton's bones: canonical joint names (the UE5 mannequin's,
 * ADR 0003) to the system's. A performer profile can be filled from the scheme that best matches
 * a source skeleton (WP-2). The defaults come with the plugin; Project Settings > Open3DPuppet
 * edits them and adds others.
 */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetNamingScheme
{
	GENERATED_BODY()

	/** The capture system or skeleton convention, shown when a profile is mapped from it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Naming")
	FString Name;

	/** Canonical joint name (for example "upperarm_l") to the system's bone name (for example "LeftArm"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Naming", meta = (ForceInlineRow))
	TMap<FName, FName> CanonicalToSource;
};

namespace PuppetNaming
{
	/**
	 * The canonical joint names modules read (ADR 0003): the UE5 mannequin's body, finger and eye
	 * bones, and `prop` for a tracked prop (WP-7). In skeleton order.
	 */
	OPEN3DPUPPET_API TArray<FName> CanonicalJoints();

	/** The schemes that come with the plugin: Xsens, OptiTrack, Rokoko, Vicon, Qualisys, Mocopi, Move One, HumanIK, Mixamo and Unity humanoid (VMC). */
	OPEN3DPUPPET_API TArray<FPuppetNamingScheme> DefaultSchemes();

	/** The best scheme for a source skeleton and the profile mapping it gives. */
	struct FMatch
	{
		/** Index into the schemes, or INDEX_NONE when none matches a single bone. */
		int32 Scheme = INDEX_NONE;
		/** Scheme, then any other scheme that matches as well and maps the same bones (systems that share names, such as Mixamo and Qualisys without fingers). */
		TArray<int32> Equivalent;
		/** Canonical joint to the source's bone, as the source spells it; joints the source names canonically are left out. */
		TMap<FName, FName> CanonicalToSource;
		/** The scheme's canonical joints the source skeleton lacks. */
		TArray<FName> Missing;
	};

	/**
	 * Picks the scheme that names the most of the source's bones, less those whose source parent
	 * contradicts the scheme (the neck not under its top of the spine, the forearm not under its
	 * upper arm); the earlier scheme wins a tie. Names match ignoring case, a namespace
	 * ("mixamorig:") and a prefix every bone shares (a skeleton name). Parents are indices into
	 * SourceBones, as Live Link gives them.
	 */
	OPEN3DPUPPET_API FMatch Match(TConstArrayView<FName> SourceBones, TConstArrayView<int32> SourceParents, TConstArrayView<FPuppetNamingScheme> Schemes);
}
