// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_CurveToBone.h"

#include "PuppetBoneOffset.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_CurveToBone)

namespace PuppetModuleCurveToBone
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_CurveToBone>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_CurveToBone>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			Curve = Context.Performer.FindCurve(Settings.PerformerCurve);
			if (Curve == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("the performer has no curve '%s'"), *Settings.PerformerCurve.ToString());
				return false;
			}
			Bone = Context.Puppet.FindBone(Settings.PuppetBone);
			if (Bone == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("the puppet has no bone '%s'"), *Settings.PuppetBone.ToString());
				return false;
			}
			Reference = PuppetBoneOffset::ReferenceLocal(Context.Puppet, Bone);
			bHasAmount = false;
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			const float Target = PuppetRetarget::RemapCurve(Context.GetCurveDelta(Curve), Settings.InputMin, Settings.InputMax, Settings.DeadZone);
			// The first frame after a bind starts at the curve, not at the reference pose.
			Amount = bHasAmount ? PuppetRetarget::SmoothTowards(Amount, Target, Settings.SmoothingTime, Context.DeltaTime) : Target;
			bHasAmount = true;
			const FQuat Turn = PuppetRetarget::ScaleRotation(Settings.Rotation.Quaternion(), Amount);
			PuppetBoneOffset::Apply(Context.Puppet, Bone, Reference, Turn, Settings.Translation * Amount, Context.Weight);
		}

	private:
		int32 Curve = INDEX_NONE;
		int32 Bone = INDEX_NONE;
		FTransform Reference;
		/** How much of Rotation and Translation the bone takes, after smoothing. */
		float Amount = 0.0f;
		bool bHasAmount = false;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_CurveToBone::CreateSolver() const
{
	return MakeUnique<PuppetModuleCurveToBone::FSolver>(*this);
}
