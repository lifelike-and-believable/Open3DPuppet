// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Fan.h"

#include "PuppetBoneOffset.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Fan)

namespace PuppetModuleFan
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Fan>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Fan>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			if (Settings.PerformerJoints.IsEmpty())
			{
				OutError = TEXT("the fan reads no performer joints");
				return false;
			}
			Joints.Reset();
			for (const FName Joint : Settings.PerformerJoints)
			{
				const int32 Index = Context.Performer.FindJoint(Joint);
				if (Index == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *Joint.ToString());
					return false;
				}
				Joints.Add(Index);
			}

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
			bHasAmount = false;
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			// The average bend, measured as the Wing measures its elbow.
			double Bend = 0.0;
			for (const int32 Joint : Joints)
			{
				FQuat Delta = Context.GetJointDeltaInParent(Joint);
				if (Delta.W < 0.0)
				{
					Delta = -Delta;
				}
				Bend += FMath::RadiansToDegrees(Delta.GetAngle());
			}
			Bend /= Joints.Num();

			const float Target = PuppetRetarget::RemapCurve(static_cast<float>(Bend), Settings.ClosedBend, Settings.OpenBend, 0.0f);
			// The first frame after a bind starts at the bend, not at the reference pose.
			Amount = bHasAmount ? PuppetRetarget::SmoothTowards(Amount, Target, Settings.SmoothingTime, Context.DeltaTime) : Target;
			bHasAmount = true;

			// Each feather by its place from the middle of the list: -1 at the first edge, 1 at the last.
			const FVector Axis = Settings.SpreadAxis.GetSafeNormal(UE_SMALL_NUMBER, FVector::ZAxisVector);
			const double EdgeAngle = FMath::DegreesToRadians(static_cast<double>(Settings.SpreadAngle)) * Amount;
			const int32 NumFeathers = FeatherBones.Num();
			const double Middle = 0.5 * static_cast<double>(NumFeathers - 1);
			for (int32 Index = 0; Index < NumFeathers; ++Index)
			{
				const double Place = Middle > 0.0 ? (static_cast<double>(Index) - Middle) / Middle : 0.0;
				PuppetBoneOffset::Apply(Context.Puppet, FeatherBones[Index], FeatherReferences[Index],
					FQuat(Axis, EdgeAngle * Place), FVector::ZeroVector, Context.Weight);
			}
		}

	private:
		TArray<int32> Joints;
		TArray<int32> FeatherBones;
		/** Each feather's reference transform relative to its parent: the fan closed. */
		TArray<FTransform> FeatherReferences;
		/** How open the fan is, 0 closed to 1 open, after smoothing. */
		float Amount = 0.0f;
		bool bHasAmount = false;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Fan::CreateSolver() const
{
	return MakeUnique<PuppetModuleFan::FSolver>(*this);
}
