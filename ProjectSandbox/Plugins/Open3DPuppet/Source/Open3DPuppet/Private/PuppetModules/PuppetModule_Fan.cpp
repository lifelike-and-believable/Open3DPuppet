// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Fan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Fan)

namespace PuppetModuleFan
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Fan>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Fan>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Fan::CreateSolver() const
{
	return MakeUnique<PuppetModuleFan::FSolver>(*this);
}
