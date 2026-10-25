// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_DynamicChain.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_DynamicChain)

namespace PuppetModuleDynamicChain
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_DynamicChain>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_DynamicChain>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_DynamicChain::CreateSolver() const
{
	return MakeUnique<PuppetModuleDynamicChain::FSolver>(*this);
}
