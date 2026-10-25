// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_TwoBoneLimb.h"

#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"
#include "TwoBoneIK.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_TwoBoneLimb)

namespace PuppetModuleTwoBoneLimb
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_TwoBoneLimb>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_TwoBoneLimb>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			const FName PerformerNames[3] = { Settings.PerformerUpper, Settings.PerformerLower, Settings.PerformerEnd };
			const FName PuppetNames[3] = { Settings.PuppetUpper, Settings.PuppetLower, Settings.PuppetEnd };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Joints[Index] = Context.Performer.FindJoint(PerformerNames[Index]);
				if (Joints[Index] == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *PerformerNames[Index].ToString());
					return false;
				}
				Bones[Index] = Context.Puppet.FindBone(PuppetNames[Index]);
				if (Bones[Index] == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the puppet has no bone '%s'"), *PuppetNames[Index].ToString());
					return false;
				}
			}
			TArray<int32> Chain;
			if (!PuppetRetarget::CollectChain(Context.Performer.GetParents(), Joints[0], Joints[1], Chain)
				|| !PuppetRetarget::CollectChain(Context.Performer.GetParents(), Joints[1], Joints[2], Chain))
			{
				OutError = TEXT("the performer joints are not one limb (upper, then lower, then end)");
				return false;
			}
			if (!Context.Puppet.GetChain(Bones[0], Bones[1], Chain) || !Context.Puppet.GetChain(Bones[1], Bones[2], Chain))
			{
				OutError = TEXT("the puppet bones are not one limb (upper, then lower, then end)");
				return false;
			}
			if (Settings.EffectorMode == EPuppetEffectorMode::PelvisRelative && (Context.PuppetPelvis == INDEX_NONE || Context.PerformerPelvis == INDEX_NONE))
			{
				OutError = TEXT("Pelvis Relative needs a pelvis on both the puppet and the performer");
				return false;
			}

			FVector Positions[3];
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Positions[Index] = Context.Calibration.RestComponent[Joints[Index]].GetTranslation();
			}
			const double PerformerLength = PuppetRetarget::ChainLength(MakeArrayView(Positions));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Positions[Index] = Context.Puppet.GetReferenceComponent(Bones[Index]).GetTranslation();
			}
			const double PuppetLength = PuppetRetarget::ChainLength(MakeArrayView(Positions));
			ReachScale = PerformerLength > UE_KINDA_SMALL_NUMBER ? PuppetLength / PerformerLength : 1.0;
			UpperLength = FVector::Dist(Positions[0], Positions[1]);

			if (Settings.EffectorMode != EPuppetEffectorMode::ForwardKinematics)
			{
				// The pole rides on the upper bone, so the bend plane turns with the performer's upper
				// limb, and a nearly straight limb cannot flip its elbow or knee.
				const FVector Axis = (Positions[2] - Positions[0]).GetSafeNormal();
				FVector Bend = Positions[1] - Positions[0];
				Bend -= (Bend | Axis) * Axis;
				if (Bend.Size() < 0.01 * UpperLength)
				{
					Bend = Settings.PoleDirection - (Settings.PoleDirection | Axis) * Axis;
				}
				if (Bend.IsNearlyZero())
				{
					OutError = TEXT("the puppet's limb is straight in its reference pose; set Pole Direction");
					return false;
				}
				PoleLocal = Context.Puppet.GetReferenceComponent(Bones[0]).GetRotation().UnrotateVector(Bend.GetSafeNormal());
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			FPuppetPose& Puppet = Context.Puppet;
			FTransform Original[3];
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Original[Index] = Puppet.GetComponent(Bones[Index]);
			}

			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FQuat Target = Context.GetJointDelta(Joints[Index]) * Puppet.GetReferenceComponent(Bones[Index]).GetRotation();
				Puppet.SetComponentRotation(Bones[Index], Target.GetNormalized());
			}

			if (Settings.EffectorMode != EPuppetEffectorMode::ForwardKinematics)
			{
				FTransform Upper = Puppet.GetComponent(Bones[0]);
				FTransform Lower = Puppet.GetComponent(Bones[1]);
				FTransform End = Puppet.GetComponent(Bones[2]);
				const FVector Pole = Lower.GetTranslation() + Upper.GetRotation().RotateVector(PoleLocal) * UpperLength;
				AnimationCore::SolveTwoBoneIK(Upper, Lower, End, Pole, EffectorTarget(Context, Upper.GetTranslation()),
					Settings.bAllowStretching, 1.0, Settings.MaxStretchScale);
				Puppet.SetComponent(Bones[0], Upper);
				Puppet.SetComponent(Bones[1], Lower);
				Puppet.SetComponent(Bones[2], End);
			}

			if (Context.Weight < 1.0f)
			{
				// Every solved transform is read before any is blended: setting a bone moves its descendants.
				FTransform Solved[3];
				for (int32 Index = 0; Index < 3; ++Index)
				{
					Solved[Index] = Puppet.GetComponent(Bones[Index]);
				}
				for (int32 Index = 0; Index < 3; ++Index)
				{
					FTransform Blended;
					Blended.Blend(Original[Index], Solved[Index], Context.Weight);
					Puppet.SetComponent(Bones[Index], Blended);
				}
			}
		}

	private:
		FVector EffectorTarget(const FPuppetSolveContext& Context, const FVector& UpperPosition) const
		{
			const FVector Effector = Context.GetJointPosition(Joints[2]);
			switch (Settings.EffectorMode)
			{
			case EPuppetEffectorMode::ScaledReach:
				return UpperPosition + (Effector - Context.GetJointPosition(Joints[0])) * ReachScale;
			case EPuppetEffectorMode::PelvisRelative:
				return Context.Puppet.GetComponent(Context.PuppetPelvis).GetTranslation()
					+ (Effector - Context.GetJointPosition(Context.PerformerPelvis)) * Context.RootScale;
			default:
				return Effector;
			}
		}

		int32 Joints[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
		int32 Bones[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
		double ReachScale = 1.0;
		double UpperLength = 0.0;
		/** The pole direction in the upper bone's reference frame. */
		FVector PoleLocal = FVector::ZeroVector;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_TwoBoneLimb::CreateSolver() const
{
	return MakeUnique<PuppetModuleTwoBoneLimb::FSolver>(*this);
}
