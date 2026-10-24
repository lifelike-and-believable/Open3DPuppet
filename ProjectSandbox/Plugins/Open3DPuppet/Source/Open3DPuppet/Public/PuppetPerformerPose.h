// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Math/Transform.h"
#include "UObject/NameTypes.h"

class UPuppetPerformerProfile;

/**
 * One performer's pose, as a Live Link Animation-role subject delivers it: bone names, parent
 * indices and local transforms, plus component-space transforms and a lookup by canonical joint
 * name (ADR 0003). The skeleton comes from the network or a file, so SetSkeleton validates it.
 */
class OPEN3DPUPPET_API FPuppetPerformerPose
{
public:
	/**
	 * Sets the skeleton. Parent indices must be INDEX_NONE or another bone of the skeleton, and the
	 * parents must not form a cycle; Live Link does not require parents to precede children.
	 * Canonical joint names resolve through Profile (optional), then by the bone's own name.
	 * On failure the pose is empty and OutError says why. Allocates.
	 */
	bool SetSkeleton(TConstArrayView<FName> InNames, TConstArrayView<int32> InParents, const UPuppetPerformerProfile* Profile, FString& OutError);

	/** True when the skeleton is the one SetSkeleton was last given (names and parents). */
	bool HasSkeleton(TConstArrayView<FName> InNames, TConstArrayView<int32> InParents) const;

	/** Sets one local transform per bone and computes component space. False on a count mismatch. */
	bool SetLocalTransforms(TConstArrayView<FTransform> InLocal);

	bool HasSkeleton() const { return Names.Num() > 0; }
	bool HasTransforms() const { return bHasTransforms; }
	int32 Num() const { return Names.Num(); }
	TConstArrayView<FName> GetBoneNames() const { return Names; }
	TConstArrayView<int32> GetParents() const { return Parents; }
	const FTransform& GetComponent(int32 Bone) const { return Component[Bone]; }
	TConstArrayView<FTransform> GetComponentTransforms() const { return Component; }

	/** The bone index of a canonical joint (for example "upperarm_l"), or INDEX_NONE. */
	int32 FindJoint(FName CanonicalJoint) const;

private:
	TArray<FName> Names;
	TArray<int32> Parents;
	/** Bone indices with every parent before its children. */
	TArray<int32> EvaluationOrder;
	TMap<FName, int32> CanonicalToBone;
	TArray<FTransform> Local;
	TArray<FTransform> Component;
	bool bHasTransforms = false;
};
