// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_DigitigradeLeg.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_DigitigradeLeg)

namespace PuppetModuleDigitigradeLeg
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_DigitigradeLeg>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_DigitigradeLeg>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_DigitigradeLeg::CreateSolver() const
{
	return MakeUnique<PuppetModuleDigitigradeLeg::FSolver>(*this);
}
