// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_PropChain.h"

#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_PropChain)

namespace PuppetModulePropChain
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_PropChain>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_PropChain>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			Joint = Context.Performer.FindJoint(Settings.PerformerJoint);
			if (Joint == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *Settings.PerformerJoint.ToString());
				return false;
			}
			if (!Context.Puppet.GetChain(Context.Puppet.FindBone(Settings.PuppetStart), Context.Puppet.FindBone(Settings.PuppetEnd), PuppetChain))
			{
				OutError = FString::Printf(TEXT("the puppet has no chain from '%s' to '%s'"), *Settings.PuppetStart.ToString(), *Settings.PuppetEnd.ToString());
				return false;
			}
			TArray<FVector> Positions;
			for (const int32 Bone : PuppetChain)
			{
				Positions.Add(Context.Puppet.GetReferenceComponent(Bone).GetTranslation());
			}
			PuppetRetarget::ChainPositions(Positions, ChainPositions);
			ChainParent = Context.Puppet.GetParent(PuppetChain[0]);
			Original.SetNum(PuppetChain.Num());
			// The filter starts over from the prop's current motion.
			bHasSmoothed = false;
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			// Amplify and smooth the prop's own motion, not the hand-off turn the delta carries.
			const FQuat Turn = Context.HandoffOffset.GetRotation();
			FQuat Motion = PuppetRetarget::ScaleRotation(Turn.Inverse() * Context.GetJointDelta(Joint), Settings.Amplification);
			if (Settings.Smoothing > 0.0f)
			{
				if (bHasSmoothed)
				{
					// Exponential, so the same elapsed time gives the same result at any frame rate.
					const float Alpha = 1.0f - FMath::Exp(-Context.DeltaTime / Settings.Smoothing);
					Smoothed = FQuat::Slerp(Smoothed, Motion, Alpha).GetNormalized();
				}
				else
				{
					Smoothed = Motion;
				}
				Motion = Smoothed;
			}
			bHasSmoothed = Settings.Smoothing > 0.0f;

			FPuppetPose& Puppet = Context.Puppet;
			const FQuat ParentMotion = ChainParent == INDEX_NONE
				? FQuat::Identity
				: Puppet.GetComponent(ChainParent).GetRotation() * Puppet.GetReferenceComponent(ChainParent).GetRotation().Inverse();
			// The parent already carries the hand-off turn; on the stage the prop's direction needs it.
			const FQuat StageDelta = Turn * Motion;

			// Before any write: writing a bone moves its descendants.
			for (int32 Index = 0; Index < PuppetChain.Num(); ++Index)
			{
				Original[Index] = Puppet.GetComponent(PuppetChain[Index]).GetRotation();
			}
			for (int32 Index = 0; Index < PuppetChain.Num(); ++Index)
			{
				const int32 Bone = PuppetChain[Index];
				const float T = Settings.Shape == EPuppetPropShape::Rigid ? 1.0f : ChainPositions[Index];
				const FQuat Rotation = Settings.Space == EPuppetPropSpace::Stage
					? FQuat::Slerp(ParentMotion, StageDelta, T)
					: ParentMotion * FQuat::Slerp(FQuat::Identity, Motion, T);
				const FQuat Target = (Rotation * Puppet.GetReferenceComponent(Bone).GetRotation()).GetNormalized();
				Puppet.SetComponentRotation(Bone, FQuat::Slerp(Original[Index], Target, Context.Weight));
			}
		}

	private:
		int32 Joint = INDEX_NONE;
		int32 ChainParent = INDEX_NONE;
		TArray<int32> PuppetChain;
		TArray<float> ChainPositions;
		TArray<FQuat> Original;
		FQuat Smoothed = FQuat::Identity;
		bool bHasSmoothed = false;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_PropChain::CreateSolver() const
{
	return MakeUnique<PuppetModulePropChain::FSolver>(*this);
}
