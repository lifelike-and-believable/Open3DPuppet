// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Eyes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Eyes)

namespace PuppetModuleEyes
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Eyes>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Eyes>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Eyes::CreateSolver() const
{
	return MakeUnique<PuppetModuleEyes::FSolver>(*this);
}
