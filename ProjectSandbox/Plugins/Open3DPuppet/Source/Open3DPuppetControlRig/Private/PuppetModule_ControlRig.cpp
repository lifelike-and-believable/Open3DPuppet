// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModule_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_ControlRig)

namespace PuppetModuleControlRig
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_ControlRig>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_ControlRig>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_ControlRig::CreateSolver() const
{
	return MakeUnique<PuppetModuleControlRig::FSolver>(*this);
}
