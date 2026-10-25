// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_CurveToBone.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_CurveToBone)

namespace PuppetModuleCurveToBone
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_CurveToBone>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_CurveToBone>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_CurveToBone::CreateSolver() const
{
	return MakeUnique<PuppetModuleCurveToBone::FSolver>(*this);
}
