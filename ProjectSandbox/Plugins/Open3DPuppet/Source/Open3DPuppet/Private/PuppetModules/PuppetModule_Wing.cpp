// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Wing.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Wing)

namespace PuppetModuleWing
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Wing>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Wing>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Wing::CreateSolver() const
{
	return MakeUnique<PuppetModuleWing::FSolver>(*this);
}
