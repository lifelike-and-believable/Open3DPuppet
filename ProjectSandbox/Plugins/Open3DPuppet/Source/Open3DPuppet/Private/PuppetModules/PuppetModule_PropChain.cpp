// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_PropChain.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_PropChain)

namespace PuppetModulePropChain
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_PropChain>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_PropChain>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_PropChain::CreateSolver() const
{
	return MakeUnique<PuppetModulePropChain::FSolver>(*this);
}
