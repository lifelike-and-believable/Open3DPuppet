// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Digits.h"

#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Digits)

namespace PuppetModuleDigits
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Digits>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Digits>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			SpreadAxis = Settings.SpreadAxis.GetSafeNormal();
			if (SpreadAxis.IsZero())
			{
				OutError = TEXT("the spread axis is zero");
				return false;
			}
			if (Settings.Digits.IsEmpty())
			{
				OutError = TEXT("it has no digits");
				return false;
			}

			Digits.SetNum(Settings.Digits.Num());
			TArray<FVector> Positions;
			for (int32 Index = 0; Index < Digits.Num(); ++Index)
			{
				const FPuppetDigit& Mapping = Settings.Digits[Index];
				FDigit& Digit = Digits[Index];
				const int32 Start = Context.Performer.FindJoint(Mapping.PerformerStart);
				const int32 End = Context.Performer.FindJoint(Mapping.PerformerEnd);
				if (!PuppetRetarget::CollectChain(Context.Performer.GetParents(), Start, End, Digit.PerformerChain))
				{
					OutError = FString::Printf(TEXT("the performer has no chain from '%s' to '%s'"), *Mapping.PerformerStart.ToString(), *Mapping.PerformerEnd.ToString());
					return false;
				}
				if (!Context.Puppet.GetChain(Context.Puppet.FindBone(Mapping.PuppetStart), Context.Puppet.FindBone(Mapping.PuppetEnd), Digit.PuppetChain))
				{
					OutError = FString::Printf(TEXT("the puppet has no chain from '%s' to '%s'"), *Mapping.PuppetStart.ToString(), *Mapping.PuppetEnd.ToString());
					return false;
				}
				Digit.PuppetBase = Context.Puppet.GetParent(Digit.PuppetChain[0]);

				Positions.Reset();
				for (const int32 Joint : Digit.PerformerChain)
				{
					Positions.Add(Context.Calibration.RestComponent[Joint].GetTranslation());
				}
				PuppetRetarget::ChainPositions(Positions, Digit.PerformerPositions);

				Positions.Reset();
				for (const int32 Bone : Digit.PuppetChain)
				{
					Positions.Add(Context.Puppet.GetReferenceComponent(Bone).GetTranslation());
				}
				PuppetRetarget::ChainPositions(Positions, Digit.PuppetPositions);

				Digit.Bends.SetNum(Digit.PerformerChain.Num());
				Digit.Original.SetNum(Digit.PuppetChain.Num());
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			for (int32 Index = 0; Index < Digits.Num(); ++Index)
			{
				// The settings change size only when an edit re-binds the puppet; until then, skip what is gone.
				if (!Settings.Digits.IsValidIndex(Index))
				{
					break;
				}
				SolveDigit(Context, Settings.Digits[Index], Digits[Index]);
			}
		}

	private:
		struct FDigit
		{
			TArray<int32> PerformerChain;
			TArray<int32> PuppetChain;
			TArray<float> PerformerPositions;
			TArray<float> PuppetPositions;
			/** The puppet digit's parent (the hand or paw), or INDEX_NONE. */
			int32 PuppetBase = INDEX_NONE;
			/** Each finger joint's bend relative to the finger's parent: the parent-relative deltas of the joints up to it, composed. */
			TArray<FQuat> Bends;
			TArray<FQuat> Original;
		};

		void SolveDigit(FPuppetSolveContext& Context, const FPuppetDigit& Mapping, FDigit& Digit) const
		{
			const bool bScale = Mapping.CurlGain != 1.0f || Mapping.SpreadGain != 1.0f;
			FQuat Bend = FQuat::Identity;
			for (int32 Index = 0; Index < Digit.PerformerChain.Num(); ++Index)
			{
				FQuat Delta = Context.GetJointDeltaInParent(Digit.PerformerChain[Index]);
				if (bScale)
				{
					FQuat Curl;
					FQuat Spread;
					Delta.ToSwingTwist(SpreadAxis, Curl, Spread);
					Delta = PuppetRetarget::ScaleRotation(Curl, Mapping.CurlGain) * PuppetRetarget::ScaleRotation(Spread, Mapping.SpreadGain);
				}
				Bend = (Bend * Delta).GetNormalized();
				Digit.Bends[Index] = Bend;
			}

			// The base moves with whatever module placed the hand or paw; the digit bends under it.
			FQuat BaseMotion = FQuat::Identity;
			if (Digit.PuppetBase != INDEX_NONE)
			{
				BaseMotion = Context.Puppet.GetComponent(Digit.PuppetBase).GetRotation() * Context.Puppet.GetReferenceComponent(Digit.PuppetBase).GetRotation().Inverse();
			}
			// Before any write: writing a bone moves its descendants.
			for (int32 Index = 0; Index < Digit.PuppetChain.Num(); ++Index)
			{
				Digit.Original[Index] = Context.Puppet.GetComponent(Digit.PuppetChain[Index]).GetRotation();
			}
			for (int32 Index = 0; Index < Digit.PuppetChain.Num(); ++Index)
			{
				const int32 Bone = Digit.PuppetChain[Index];
				// Sampling the composed bends, not each joint's own, keeps the finger's whole curl on a digit with more or fewer bones.
				const FQuat Sampled = PuppetRetarget::SampleChain(Digit.Bends, Digit.PerformerPositions, Digit.PuppetPositions[Index]);
				const FQuat Target = (BaseMotion * Sampled * Context.Puppet.GetReferenceComponent(Bone).GetRotation()).GetNormalized();
				Context.Puppet.SetComponentRotation(Bone, FQuat::Slerp(Digit.Original[Index], Target, Context.Weight));
			}
		}

		FVector SpreadAxis = FVector::UpVector;
		TArray<FDigit> Digits;
	};
}

UPuppetModule_Digits::UPuppetModule_Digits()
{
	for (const TCHAR* Finger : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
	{
		FPuppetDigit& Digit = Digits.AddDefaulted_GetRef();
		Digit.PerformerStart = Digit.PuppetStart = *FString::Printf(TEXT("%s_01_l"), Finger);
		Digit.PerformerEnd = Digit.PuppetEnd = *FString::Printf(TEXT("%s_03_l"), Finger);
	}
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Digits::CreateSolver() const
{
	return MakeUnique<PuppetModuleDigits::FSolver>(*this);
}
