// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Root.h"

#include "PuppetPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Root)

namespace PuppetModuleRoot
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Root>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Root>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			if (Context.PuppetPelvis == INDEX_NONE)
			{
				OutError = TEXT("the puppet has no pelvis bone (UPuppetAsset::PelvisBone)");
				return false;
			}
			if (Context.PerformerPelvis == INDEX_NONE)
			{
				OutError = TEXT("the performer has no 'pelvis' joint; map it in the performer profile");
				return false;
			}
			if (Settings.bDriveTranslation && !Context.bOwnsRoot)
			{
				OutError = TEXT("only a module on the root slot may drive translation (ADR 0004)");
				return false;
			}
			bOwnsHandoff = Context.bOwnsRoot;
			CapturedYaw = 0.0;
			CapturedTranslation = FVector::ZeroVector;
			Remaining = 0.0f;
			bHasPrevious = false;
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			const int32 Pelvis = Context.PuppetPelvis;
			const int32 Joint = Context.PerformerPelvis;
			const FTransform& Reference = Context.Puppet.GetReferenceComponent(Pelvis);
			const FTransform Original = Context.Puppet.GetComponent(Pelvis);
			if (bOwnsHandoff)
			{
				UpdateHandoff(Context, Reference, Original);
			}

			FTransform Target = Original;
			if (Settings.bDriveRotation)
			{
				Target.SetRotation((Context.GetJointDelta(Joint) * Reference.GetRotation()).GetNormalized());
			}
			if (Settings.bDriveTranslation)
			{
				const FVector Moved = Context.GetJointTranslation(Joint);
				const double Horizontal = Context.RootScale * Settings.HorizontalScale;
				const double Vertical = Context.RootScale * Settings.VerticalScale;
				Target.SetTranslation(Reference.GetTranslation() + FVector(Moved.X * Horizontal, Moved.Y * Horizontal, Moved.Z * Vertical + Settings.HeightOffset)
					+ Context.HandoffOffset.GetTranslation());
			}

			FTransform Result;
			Result.Blend(Original, Target, Context.Weight);
			Context.Puppet.SetComponent(Pelvis, Result);
		}

	private:
		/**
		 * Captures the hand-off offset on a re-centre, so the pelvis lands where the animation has it
		 * (Original), then shrinks it towards identity over the convergence time (ADR 0008).
		 */
		void UpdateHandoff(FPuppetSolveContext& Context, const FTransform& Reference, const FTransform& Original)
		{
			const int32 Joint = Context.PerformerPelvis;
			// The performer's own motion, without the current offset's turn.
			const FQuat Unturn = Context.HandoffOffset.GetRotation().Inverse();
			const double PerformerYaw = (Unturn * Context.GetJointDelta(Joint) * Reference.GetRotation()).GetTwistAngle(FVector::ZAxisVector);
			const FVector Moved = Unturn.RotateVector(Context.GetJointTranslation(Joint));
			const double Horizontal = Context.RootScale * Settings.HorizontalScale;
			const FVector Move(Moved.X * Horizontal, Moved.Y * Horizontal, 0.0);

			if (Context.bRecentre)
			{
				CapturedYaw = Settings.bDriveRotation
					? FMath::UnwindRadians(Original.GetRotation().GetTwistAngle(FVector::ZAxisVector) - PerformerYaw)
					: 0.0;
				CapturedTranslation = FVector::ZeroVector;
				if (Settings.bDriveTranslation)
				{
					// The performer's height still comes through: the offset is horizontal only.
					CapturedTranslation = Original.GetTranslation() - Reference.GetTranslation() - FQuat(FVector::ZAxisVector, CapturedYaw).RotateVector(Move);
					CapturedTranslation.Z = 0.0;
				}
				Remaining = Settings.bHoldOffset || Settings.ConvergenceTime > 0.0f ? 1.0f : 0.0f;
			}
			else if (!Settings.bHoldOffset && Remaining > 0.0f)
			{
				float Rate = 1.0f;
				if (Settings.bConvergeOnlyWhileMoving)
				{
					Rate = 0.0f;
					if (bHasPrevious && Context.DeltaTime > 0.0f)
					{
						const double Speed = FVector::Dist2D(Move, PreviousMove) / Context.DeltaTime;
						const double TurnRate = FMath::RadiansToDegrees(FMath::Abs(FMath::UnwindRadians(PerformerYaw - PreviousYaw))) / Context.DeltaTime;
						Rate = FMath::Clamp(static_cast<float>(FMath::Max(Speed / Settings.MovingSpeed, TurnRate / Settings.MovingTurnRate)), 0.0f, 1.0f);
					}
				}
				Remaining = Settings.ConvergenceTime > 0.0f ? FMath::Max(0.0f, Remaining - Rate * Context.DeltaTime / Settings.ConvergenceTime) : 0.0f;
			}
			PreviousMove = Move;
			PreviousYaw = PerformerYaw;
			bHasPrevious = true;

			const double Amount = 1.0f - PuppetEasing::Apply(Settings.ConvergenceEasing, 1.0f - Remaining);
			Context.HandoffOffset = FTransform(FQuat(FVector::ZAxisVector, CapturedYaw * Amount), CapturedTranslation * Amount);
		}

		bool bOwnsHandoff = false;
		/** The offset as captured, before convergence: a turn in radians and a horizontal move. */
		double CapturedYaw = 0.0;
		FVector CapturedTranslation = FVector::ZeroVector;
		/** How much of the captured offset is left to converge, 1 to 0, before easing. */
		float Remaining = 0.0f;
		bool bHasPrevious = false;
		FVector PreviousMove = FVector::ZeroVector;
		double PreviousYaw = 0.0;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Root::CreateSolver() const
{
	return MakeUnique<PuppetModuleRoot::FSolver>(*this);
}
