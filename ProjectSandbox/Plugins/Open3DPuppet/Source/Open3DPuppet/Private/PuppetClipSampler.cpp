// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetClipSampler.h"

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/Skeleton.h"
#include "BonePose.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"

bool FPuppetClipSampler::SetSequence(const UAnimSequence* InSequence, FString& OutError)
{
	*this = FPuppetClipSampler();
	OutError.Reset();
	if (!InSequence)
	{
		return true;
	}
	const USkeleton* Skeleton = InSequence->GetSkeleton();
	if (!Skeleton || Skeleton->GetReferenceSkeleton().GetNum() == 0)
	{
		OutError = TEXT("the sequence has no skeleton");
		return false;
	}
	if (InSequence->IsValidAdditive())
	{
		OutError = TEXT("the sequence is additive; a performer clip must be a full pose");
		return false;
	}

	// The sequence's own skeleton, every bone, as PoseSearch samples an asset
	// (Engine/Plugins/Animation/PoseSearch/Source/Runtime/Private/PoseSearchAssetSamplerLibrary.cpp).
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	TArray<FBoneIndexType> RequiredBones;
	for (int32 Bone = 0; Bone < RefSkeleton.GetNum(); ++Bone)
	{
		RequiredBones.Add(static_cast<FBoneIndexType>(Bone));
		Names.Add(RefSkeleton.GetBoneName(Bone));
		Parents.Add(RefSkeleton.GetParentIndex(Bone));
	}
	BoneContainer.InitializeTo(RequiredBones, UE::Anim::FCurveFilterSettings(UE::Anim::ECurveFilterMode::DisallowAll), *Skeleton);
	// The take's own translations: a bone in Skeleton retargeting mode would otherwise take its
	// reference translation (Engine/Source/Runtime/Engine/Private/Animation/AnimationDecompression.cpp).
	BoneContainer.SetDisableRetargeting(true);
	ReferencePose = RefSkeleton.GetRefBonePose();
	PlayLength = InSequence->GetPlayLength();
	Sequence = InSequence;
	return true;
}

bool FPuppetClipSampler::Sample(double Time, TArray<FTransform>& OutLocal) const
{
	if (!Sequence)
	{
		return false;
	}

	// The pose, curves and attributes live on the animation stack, not the heap.
	FMemMark Mark(FMemStack::Get());
	FCompactPose Pose;
	Pose.SetBoneContainer(&BoneContainer);
	FBlendedCurve Curve;
	Curve.InitFrom(BoneContainer);
	UE::Anim::FStackAttributeContainer Attributes;
	FAnimationPoseData PoseData(Pose, Curve, Attributes);
	Sequence->GetAnimationPose(PoseData, FAnimExtractContext(FMath::Clamp(Time, 0.0, PlayLength)));

	OutLocal.SetNumUninitialized(Names.Num(), EAllowShrinking::No);
	for (const FCompactPoseBoneIndex Bone : Pose.ForEachBoneIndex())
	{
		OutLocal[BoneContainer.GetSkeletonIndex(Bone)] = Pose[Bone];
	}
	return true;
}

bool FPuppetClipSampler::CaptureCalibration(bool bReferencePose, double Time, const FRotator& Alignment, FPuppetPerformerCalibration& OutCalibration) const
{
	if (!Sequence)
	{
		return false;
	}
	TArray<FTransform> Local(ReferencePose);
	if (!bReferencePose)
	{
		Sample(Time, Local);
	}
	FPuppetPerformerPose Pose;
	FString Error;
	if (!Pose.SetSkeleton(Names, Parents, nullptr, Error) || !Pose.SetLocalTransforms(Local))
	{
		return false;
	}
	OutCalibration = FPuppetPerformerCalibration::Capture(Pose, Alignment);
	return true;
}

bool FPuppetClipSampler::SampleOnto(double Time, const FReferenceSkeleton& Target, TArray<FTransform>& OutLocal) const
{
	TArray<FTransform> Clip;
	if (!Sample(Time, Clip))
	{
		return false;
	}
	OutLocal = Target.GetRefBonePose();
	for (int32 Bone = 0; Bone < Target.GetNum(); ++Bone)
	{
		const int32 ClipBone = Names.IndexOfByKey(Target.GetBoneName(Bone));
		if (ClipBone != INDEX_NONE)
		{
			OutLocal[Bone].SetRotation(Clip[ClipBone].GetRotation());
		}
	}
	return true;
}

double FPuppetClipSampler::AdvanceTime(double Time, double DeltaTime, float PlayRate, bool bLoop, double Length)
{
	return PlaceTime(Time + DeltaTime * PlayRate, bLoop, Length);
}

double FPuppetClipSampler::PlaceTime(double Time, bool bLoop, double Length)
{
	if (Length <= 0.0)
	{
		return 0.0;
	}
	if (!bLoop)
	{
		return FMath::Clamp(Time, 0.0, Length);
	}
	double Wrapped = FMath::Fmod(Time, Length);
	if (Wrapped < 0.0)
	{
		Wrapped += Length;
	}
	// Adding Length to a tiny negative remainder can round up to Length itself.
	return Wrapped < Length ? Wrapped : 0.0;
}
