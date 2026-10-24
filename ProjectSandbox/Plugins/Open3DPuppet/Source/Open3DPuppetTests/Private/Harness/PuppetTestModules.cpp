// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetTestModules.h"

#include "PuppetPerformerPose.h"
#include "PuppetPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetTestModules)

namespace PuppetTestModuleCurveCopy
{
	class FSolver : public TPuppetModuleSolver<UPuppetTestModule_CurveCopy>
	{
	public:
		using TPuppetModuleSolver<UPuppetTestModule_CurveCopy>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			Curve = Context.Performer.FindCurve(Settings.PerformerCurve);
			if (Curve == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("the performer has no curve '%s'"), *Settings.PerformerCurve.ToString());
				return false;
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			Context.Puppet.SetCurve(Settings.PuppetCurve, Context.GetCurveDelta(Curve) * Context.Weight);
		}

	private:
		int32 Curve = INDEX_NONE;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetTestModule_CurveCopy::CreateSolver() const
{
	return MakeUnique<PuppetTestModuleCurveCopy::FSolver>(*this);
}
