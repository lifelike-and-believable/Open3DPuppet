// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModule_PhysicsRegion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_PhysicsRegion)

namespace PuppetModulePhysicsRegion
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_PhysicsRegion>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_PhysicsRegion>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

UPuppetModule_PhysicsRegion::UPuppetModule_PhysicsRegion()
{
	// As authored until a puppeteer asks for more.
	PhysicsWeight.Weight = 0.0f;
	Damping.Weight = 0.0f;
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_PhysicsRegion::CreateSolver() const
{
	return MakeUnique<PuppetModulePhysicsRegion::FSolver>(*this);
}

namespace PuppetPhysics
{
	FName WeightCurve(FName Region)
	{
		return NAME_None;
	}

	FName DampingCurve(FName Region)
	{
		return NAME_None;
	}
}
