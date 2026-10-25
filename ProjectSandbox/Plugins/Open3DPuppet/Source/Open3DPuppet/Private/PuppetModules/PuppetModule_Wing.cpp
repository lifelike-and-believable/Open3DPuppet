// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Wing.h"

#include "PuppetBoneOffset.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Wing)

namespace PuppetModuleWing
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Wing>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Wing>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			const FName PerformerNames[2] = { Settings.PerformerUpper, Settings.PerformerLower };
			for (int32 Index = 0; Index < 2; ++Index)
			{
				Joints[Index] = Context.Performer.FindJoint(PerformerNames[Index]);
				if (Joints[Index] == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *PerformerNames[Index].ToString());
					return false;
				}
			}
			const FName PuppetNames[3] = { Settings.PuppetUpper, Settings.PuppetLower, Settings.PuppetHand };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Bones[Index] = Context.Puppet.FindBone(PuppetNames[Index]);
				if (Bones[Index] == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the puppet has no bone '%s'"), *PuppetNames[Index].ToString());
					return false;
				}
				References[Index] = PuppetBoneOffset::ReferenceLocal(Context.Puppet, Bones[Index]);
			}
			if (Context.Puppet.GetParent(Bones[1]) != Bones[0] || Context.Puppet.GetParent(Bones[2]) != Bones[1])
			{
				OutError = TEXT("the puppet's wing bones are not one chain (upper, then lower, then hand)");
				return false;
			}
			UpperParent = Context.Puppet.GetParent(Bones[0]);

			FeatherBones.Reset();
			FeatherReferences.Reset();
			for (const FName Feather : Settings.Feathers)
			{
				const int32 Bone = Context.Puppet.FindBone(Feather);
				if (Bone == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the puppet has no feather bone '%s'"), *Feather.ToString());
					return false;
				}
				FeatherBones.Add(Bone);
				FeatherReferences.Add(PuppetBoneOffset::ReferenceLocal(Context.Puppet, Bone));
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			FPuppetPose& Puppet = Context.Puppet;

			// The flap: the upper arm's turn relative to the chest, under the puppet's own chest.
			const FQuat Flap = PuppetRetarget::ScaleRotation(Context.GetJointDeltaInParent(Joints[0]), Settings.FlapAmplification);
			const FQuat ChestMotion = UpperParent == INDEX_NONE
				? FQuat::Identity
				: Puppet.GetComponent(UpperParent).GetRotation() * Puppet.GetReferenceComponent(UpperParent).GetRotation().Inverse();
			const FQuat Target = (ChestMotion * Flap * Puppet.GetReferenceComponent(Bones[0]).GetRotation()).GetNormalized();
			Puppet.SetComponentRotation(Bones[0], FQuat::Slerp(Puppet.GetComponent(Bones[0]).GetRotation(), Target, Context.Weight).GetNormalized());

			// The fold: how far the elbow bend is through the fold range.
			FQuat Elbow = Context.GetJointDeltaInParent(Joints[1]);
			if (Elbow.W < 0.0)
			{
				Elbow = -Elbow;
			}
			const float Fold = PuppetRetarget::RemapCurve(static_cast<float>(FMath::RadiansToDegrees(Elbow.GetAngle())), Settings.FoldStartAngle, Settings.FoldEndAngle, 0.0f);
			PuppetBoneOffset::Apply(Puppet, Bones[1], References[1], PuppetRetarget::ScaleRotation(Settings.ForearmFold.Quaternion(), Fold), FVector::ZeroVector, Context.Weight);
			PuppetBoneOffset::Apply(Puppet, Bones[2], References[2], PuppetRetarget::ScaleRotation(Settings.HandFold.Quaternion(), Fold), FVector::ZeroVector, Context.Weight);

			// The feathers close like a fan: each by its place in the list.
			const int32 NumFeathers = FeatherBones.Num();
			for (int32 Index = 0; Index < NumFeathers; ++Index)
			{
				const float Share = NumFeathers > 1 ? static_cast<float>(Index) / static_cast<float>(NumFeathers - 1) : 1.0f;
				PuppetBoneOffset::Apply(Puppet, FeatherBones[Index], FeatherReferences[Index],
					PuppetRetarget::ScaleRotation(Settings.FeatherFold.Quaternion(), Fold * Share), FVector::ZeroVector, Context.Weight);
			}
		}

	private:
		int32 Joints[2] = { INDEX_NONE, INDEX_NONE };
		/** Upper, lower and hand, and their reference transforms relative to their parents. */
		int32 Bones[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
		FTransform References[3];
		int32 UpperParent = INDEX_NONE;
		TArray<int32> FeatherBones;
		TArray<FTransform> FeatherReferences;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Wing::CreateSolver() const
{
	return MakeUnique<PuppetModuleWing::FSolver>(*this);
}
