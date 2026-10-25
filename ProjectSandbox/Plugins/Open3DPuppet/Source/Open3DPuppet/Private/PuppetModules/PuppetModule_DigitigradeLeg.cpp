// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_DigitigradeLeg.h"

#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"
#include "TwoBoneIK.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_DigitigradeLeg)

namespace PuppetModuleDigitigradeLeg
{
	/** Puppet bones, root of the leg first; performer joints use Upper, Lower and the end joint at index 2. */
	enum EBone : int32 { Upper, Lower, Metatarsal, Paw, NumBones };
	constexpr int32 PerformerEnd = 2;

	class FSolver : public TPuppetModuleSolver<UPuppetModule_DigitigradeLeg>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_DigitigradeLeg>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			const FName PerformerNames[3] = { Settings.PerformerUpper, Settings.PerformerLower, Settings.PerformerEnd };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Joints[Index] = Context.Performer.FindJoint(PerformerNames[Index]);
				if (Joints[Index] == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *PerformerNames[Index].ToString());
					return false;
				}
			}
			const FName PuppetNames[NumBones] = { Settings.PuppetUpper, Settings.PuppetLower, Settings.PuppetMetatarsal, Settings.PuppetPaw };
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
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
			for (int32 Index = 0; Index + 1 < NumBones; ++Index)
			{
				if (!Context.Puppet.GetChain(Bones[Index], Bones[Index + 1], Chain))
				{
					OutError = TEXT("the puppet bones are not one leg (upper, lower, metatarsal, then paw)");
					return false;
				}
			}

			FVector Positions[NumBones];
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				Positions[Index] = Context.Puppet.GetReferenceComponent(Bones[Index]).GetTranslation();
			}
			FVector PerformerPositions[3];
			for (int32 Index = 0; Index < 3; ++Index)
			{
				PerformerPositions[Index] = Context.Calibration.RestComponent[Joints[Index]].GetTranslation();
			}
			const double PerformerLength = PuppetRetarget::ChainLength(MakeArrayView(PerformerPositions));
			ReachScale = PerformerLength > UE_KINDA_SMALL_NUMBER ? PuppetRetarget::ChainLength(MakeArrayView(Positions)) / PerformerLength : 1.0;
			PerformerRestOffset = PerformerPositions[PerformerEnd] - PerformerPositions[Upper];
			PerformerParent = Context.Performer.GetParents()[Joints[Upper]];
			PuppetParent = Context.Puppet.GetParent(Bones[Upper]);
			PawOffset = Positions[Paw] - Positions[Upper];
			ContactZ = Positions[Paw].Z;

			MetatarsalLength = FVector::Dist(Positions[Metatarsal], Positions[Paw]);
			MetatarsalDirection = (Positions[Paw] - Positions[Metatarsal]).GetSafeNormal();
			// Turning the metatarsal about this axis takes it further from the shin's line: the hock flexes.
			HockAxis = ((Positions[Metatarsal] - Positions[Lower]).GetSafeNormal() ^ MetatarsalDirection).GetSafeNormal();
			if (HockAxis.IsZero())
			{
				OutError = TEXT("the puppet's hock is straight in its reference pose");
				return false;
			}

			// The pole rides on the thigh, as in the Two-Bone Limb module, so the knee cannot flip.
			UpperLength = FVector::Dist(Positions[Upper], Positions[Lower]);
			const FVector Axis = (Positions[Metatarsal] - Positions[Upper]).GetSafeNormal();
			FVector Bend = Positions[Lower] - Positions[Upper];
			Bend -= (Bend | Axis) * Axis;
			if (Bend.Size() < 0.01 * UpperLength)
			{
				Bend = Settings.PoleDirection - (Settings.PoleDirection | Axis) * Axis;
			}
			if (Bend.IsNearlyZero())
			{
				OutError = TEXT("the puppet's hip, knee and hock are in a line in its reference pose; set Pole Direction");
				return false;
			}
			PoleLocal = Context.Puppet.GetReferenceComponent(Bones[Upper]).GetRotation().UnrotateVector(Bend.GetSafeNormal());
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			FPuppetPose& Puppet = Context.Puppet;
			FTransform Original[NumBones];
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				Original[Index] = Puppet.GetComponent(Bones[Index]);
			}
			for (const int32 Index : { Upper, Lower })
			{
				Puppet.SetComponentRotation(Bones[Index], (Context.GetJointDelta(Joints[Index]) * Puppet.GetReferenceComponent(Bones[Index]).GetRotation()).GetNormalized());
			}

			const FQuat ParentMotion = PuppetParent == INDEX_NONE
				? FQuat::Identity
				: Puppet.GetComponent(PuppetParent).GetRotation() * Puppet.GetReferenceComponent(PuppetParent).GetRotation().Inverse();

			// The performer's ankle move relative to the hip since calibration, as if the leg's parent
			// (pelvis or chest) had not turned, so the move follows the puppet's own parent instead.
			FVector Offset = Context.Performer.GetComponent(Joints[PerformerEnd]).GetTranslation() - Context.Performer.GetComponent(Joints[Upper]).GetTranslation();
			if (PerformerParent != INDEX_NONE)
			{
				Offset = (Context.Calibration.RestComponent[PerformerParent].GetRotation() * Context.Performer.GetComponent(PerformerParent).GetRotation().Inverse()).RotateVector(Offset);
			}
			FVector Move = Context.Alignment.RotateVector(Offset - PerformerRestOffset);
			Move.X *= Settings.StrideScale;
			Move.Y *= Settings.StrideScale;
			const FVector PawTarget = Puppet.GetComponent(Bones[Upper]).GetTranslation() + ParentMotion.RotateVector(PawOffset + Move * ReachScale);

			FQuat KneeBend = Context.GetJointDeltaInParent(Joints[Lower]);
			if (KneeBend.W < 0.0)
			{
				KneeBend = -KneeBend;
			}
			const double Flex = Settings.KneeToHockRatio * FMath::RadiansToDegrees(KneeBend.GetAngle()) - Settings.HeelLift;
			const FQuat MetatarsalTurn = ParentMotion * FQuat(HockAxis, FMath::DegreesToRadians(Flex));
			const FVector HockTarget = PawTarget - MetatarsalTurn.RotateVector(MetatarsalDirection) * MetatarsalLength;

			FTransform UpperTransform = Puppet.GetComponent(Bones[Upper]);
			FTransform LowerTransform = Puppet.GetComponent(Bones[Lower]);
			FTransform MetatarsalTransform = Puppet.GetComponent(Bones[Metatarsal]);
			const FVector Pole = LowerTransform.GetTranslation() + UpperTransform.GetRotation().RotateVector(PoleLocal) * UpperLength;
			AnimationCore::SolveTwoBoneIK(UpperTransform, LowerTransform, MetatarsalTransform, Pole, HockTarget, false, 1.0, 1.0);
			MetatarsalTransform.SetRotation((MetatarsalTurn * Puppet.GetReferenceComponent(Bones[Metatarsal]).GetRotation()).GetNormalized());
			Puppet.SetComponent(Bones[Upper], UpperTransform);
			Puppet.SetComponent(Bones[Lower], LowerTransform);
			Puppet.SetComponent(Bones[Metatarsal], MetatarsalTransform);

			// On the ground the paw stays flat, turning only with the body's heading; lifted, it follows the performer's foot.
			const FQuat PawTurn = PawTarget.Z <= ContactZ + Settings.ContactHeight
				? FQuat(FVector::UpVector, ParentMotion.GetTwistAngle(FVector::UpVector))
				: Context.GetJointDelta(Joints[PerformerEnd]);
			Puppet.SetComponentRotation(Bones[Paw], (PawTurn * Puppet.GetReferenceComponent(Bones[Paw]).GetRotation()).GetNormalized());

			if (Context.Weight < 1.0f)
			{
				// Every solved transform is read before any is blended: setting a bone moves its descendants.
				FTransform Solved[NumBones];
				for (int32 Index = 0; Index < NumBones; ++Index)
				{
					Solved[Index] = Puppet.GetComponent(Bones[Index]);
				}
				for (int32 Index = 0; Index < NumBones; ++Index)
				{
					FTransform Blended;
					Blended.Blend(Original[Index], Solved[Index], Context.Weight);
					Puppet.SetComponent(Bones[Index], Blended);
				}
			}
		}

	private:
		int32 Joints[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
		int32 Bones[NumBones] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
		int32 PerformerParent = INDEX_NONE;
		int32 PuppetParent = INDEX_NONE;
		/** Puppet leg length over performer leg length, at rest. */
		double ReachScale = 1.0;
		/** The performer's ankle relative to the hip at calibration. */
		FVector PerformerRestOffset = FVector::ZeroVector;
		/** The puppet's paw relative to the hip in the reference pose. */
		FVector PawOffset = FVector::ZeroVector;
		double ContactZ = 0.0;
		double MetatarsalLength = 0.0;
		FVector MetatarsalDirection = FVector::ZeroVector;
		FVector HockAxis = FVector::ZeroVector;
		double UpperLength = 0.0;
		/** The pole direction in the thigh's reference frame. */
		FVector PoleLocal = FVector::ZeroVector;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_DigitigradeLeg::CreateSolver() const
{
	return MakeUnique<PuppetModuleDigitigradeLeg::FSolver>(*this);
}
