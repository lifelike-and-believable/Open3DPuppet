// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetPose.h"

/** Posing a bone as an offset from its reference pose, for modules driven by curves rather than by a performer joint. */
namespace PuppetBoneOffset
{
	/** The bone's reference transform relative to its parent's. */
	inline FTransform ReferenceLocal(const FPuppetPose& Pose, int32 Bone)
	{
		const int32 Parent = Pose.GetParent(Bone);
		return Parent == INDEX_NONE ? Pose.GetReferenceComponent(Bone) : Pose.GetReferenceComponent(Bone).GetRelativeTransform(Pose.GetReferenceComponent(Parent));
	}

	/**
	 * Sets Bone to Reference (from ReferenceLocal) turned by Rotation about its own axes and moved by
	 * Translation in its parent's space, under its parent as it is now, blended over the bone's
	 * current transform by Weight. Keeps the bone's current scale.
	 */
	inline void Apply(FPuppetPose& Pose, int32 Bone, const FTransform& Reference, const FQuat& Rotation, const FVector& Translation, float Weight)
	{
		FTransform Local = Reference;
		Local.SetRotation((Reference.GetRotation() * Rotation).GetNormalized());
		Local.AddToTranslation(Translation);
		const int32 Parent = Pose.GetParent(Bone);
		FTransform Target = Parent == INDEX_NONE ? Local : Local * Pose.GetComponent(Parent);
		const FTransform& Original = Pose.GetComponent(Bone);
		Target.SetRotation(FQuat::Slerp(Original.GetRotation(), Target.GetRotation(), Weight).GetNormalized());
		Target.SetTranslation(FMath::Lerp(Original.GetTranslation(), Target.GetTranslation(), static_cast<double>(Weight)));
		Target.SetScale3D(Original.GetScale3D());
		Pose.SetComponent(Bone, Target);
	}
}
