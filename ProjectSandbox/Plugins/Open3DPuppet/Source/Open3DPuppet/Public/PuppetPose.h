// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Math/Transform.h"
#include "UObject/NameTypes.h"

struct FReferenceSkeleton;

/**
 * The puppet's pose while modules solve it: local and component-space transforms for every bone
 * of the target mesh's reference skeleton, indexed like the reference skeleton (mesh bone index).
 *
 * Writing a bone's component-space transform keeps the local transforms of its descendants, so
 * bones no module writes follow their parents.
 */
class OPEN3DPUPPET_API FPuppetPose
{
public:
	/**
	 * Sizes the pose for RefSkeleton and sets it to the reference pose. RestLocal, one local
	 * transform per bone, is the pose modules retarget from (GetReferenceComponent): the pose the
	 * performers' calibration pose corresponds to on this puppet. Empty means the reference pose.
	 * Allocates.
	 */
	void Initialize(const FReferenceSkeleton& RefSkeleton, TConstArrayView<FTransform> RestLocal = {});

	/** Sets every bone's local transform (one per bone, in mesh bone order) and recomputes component space. */
	void SetLocalTransforms(TConstArrayView<FTransform> InLocal);

	int32 Num() const { return Parents.Num(); }
	bool IsValidBone(int32 Bone) const { return Parents.IsValidIndex(Bone); }

	/** INDEX_NONE when the skeleton has no bone with that name. */
	int32 FindBone(FName Name) const;
	FName GetBoneName(int32 Bone) const { return Names[Bone]; }
	int32 GetParent(int32 Bone) const { return Parents[Bone]; }

	const FTransform& GetLocal(int32 Bone) const { return Local[Bone]; }
	const FTransform& GetComponent(int32 Bone) const { return Component[Bone]; }
	/** The bone in the rest pose modules retarget from (the reference pose unless the puppet asset sets a rest pose). */
	const FTransform& GetReferenceComponent(int32 Bone) const { return ReferenceComponent[Bone]; }
	TConstArrayView<FTransform> GetLocalTransforms() const { return Local; }

	/** Sets the bone's component-space transform; its descendants keep their local transforms. */
	void SetComponent(int32 Bone, const FTransform& Transform);

	/** Sets the bone's component-space rotation and keeps its local translation and scale. */
	void SetComponentRotation(int32 Bone, const FQuat& Rotation);

	/**
	 * Fills OutChain with the bones from Start down to End, Start first. False (and OutChain empty)
	 * when End is not Start or a descendant of it.
	 */
	bool GetChain(int32 Start, int32 End, TArray<int32>& OutChain) const;

	/** Writes an output curve (a morph target or material parameter), adding it if new. */
	void SetCurve(FName Name, float Value);

	/** The output curve's value, or null when no module wrote it this frame. */
	const float* FindCurve(FName Name) const;

	TConstArrayView<FName> GetCurveNames() const { return CurveNames; }
	TConstArrayView<float> GetCurveValues() const { return CurveValues; }

	/** Forgets this frame's output curves; keeps their memory. */
	void ResetCurves();

private:
	/** Recomputes component space for every bone after FirstChanged; parents precede children. */
	void PropagateFrom(int32 FirstChanged);

	TArray<FName> Names;
	TArray<int32> Parents;
	TArray<FTransform> Local;
	TArray<FTransform> Component;
	TArray<FTransform> ReferenceComponent;
	TArray<FName> CurveNames;
	TArray<float> CurveValues;
};
