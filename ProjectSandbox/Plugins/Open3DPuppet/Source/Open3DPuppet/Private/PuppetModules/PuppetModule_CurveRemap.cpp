// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_CurveRemap.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_CurveRemap)

namespace PuppetModuleCurveRemap
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_CurveRemap>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_CurveRemap>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_CurveRemap::CreateSolver() const
{
	return MakeUnique<PuppetModuleCurveRemap::FSolver>(*this);
}
