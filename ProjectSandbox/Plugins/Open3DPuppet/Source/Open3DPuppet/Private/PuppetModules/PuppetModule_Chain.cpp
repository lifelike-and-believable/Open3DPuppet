// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Chain.h"

#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Chain)

namespace PuppetModuleChain
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Chain>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Chain>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			const int32 Start = Context.Performer.FindJoint(Settings.PerformerStart);
			const int32 End = Context.Performer.FindJoint(Settings.PerformerEnd);
			if (!PuppetRetarget::CollectChain(Context.Performer.GetParents(), Start, End, PerformerChain))
			{
				OutError = FString::Printf(TEXT("the performer has no chain from '%s' to '%s'"), *Settings.PerformerStart.ToString(), *Settings.PerformerEnd.ToString());
				return false;
			}
			if (!Context.Puppet.GetChain(Context.Puppet.FindBone(Settings.PuppetStart), Context.Puppet.FindBone(Settings.PuppetEnd), PuppetChain))
			{
				OutError = FString::Printf(TEXT("the puppet has no chain from '%s' to '%s'"), *Settings.PuppetStart.ToString(), *Settings.PuppetEnd.ToString());
				return false;
			}

			TArray<FVector> Positions;
			for (const int32 Joint : PerformerChain)
			{
				Positions.Add(Context.Calibration.RestComponent[Joint].GetTranslation());
			}
			PuppetRetarget::ChainPositions(Positions, PerformerPositions);

			Positions.Reset();
			for (const int32 Bone : PuppetChain)
			{
				Positions.Add(Context.Puppet.GetReferenceComponent(Bone).GetTranslation());
			}
			PuppetRetarget::ChainPositions(Positions, PuppetPositions);

			Deltas.SetNum(PerformerChain.Num());
			Original.SetNum(PuppetChain.Num());
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			for (int32 Index = 0; Index < PerformerChain.Num(); ++Index)
			{
				const FQuat Delta = Context.GetJointDelta(PerformerChain[Index]);
				// Amplify the performer's motion, not the hand-off turn the delta carries.
				const FQuat Turn = Context.HandoffOffset.GetRotation();
				Deltas[Index] = Settings.Amplification == 1.0f ? Delta : Turn * PuppetRetarget::ScaleRotation(Turn.Inverse() * Delta, Settings.Amplification);
			}
			// Before any write: writing a bone moves its descendants.
			for (int32 Index = 0; Index < PuppetChain.Num(); ++Index)
			{
				Original[Index] = Context.Puppet.GetComponent(PuppetChain[Index]).GetRotation();
			}
			for (int32 Index = 0; Index < PuppetChain.Num(); ++Index)
			{
				const int32 Bone = PuppetChain[Index];
				const FQuat Delta = PuppetRetarget::SampleChain(Deltas, PerformerPositions, PuppetPositions[Index]);
				const FQuat Target = (Delta * Context.Puppet.GetReferenceComponent(Bone).GetRotation()).GetNormalized();
				Context.Puppet.SetComponentRotation(Bone, FQuat::Slerp(Original[Index], Target, Context.Weight));
			}
		}

	private:
		TArray<int32> PerformerChain;
		TArray<int32> PuppetChain;
		TArray<float> PerformerPositions;
		TArray<float> PuppetPositions;
		TArray<FQuat> Deltas;
		TArray<FQuat> Original;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Chain::CreateSolver() const
{
	return MakeUnique<PuppetModuleChain::FSolver>(*this);
}
