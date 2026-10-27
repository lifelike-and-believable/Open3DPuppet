// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModule_PhysicsRegion.h"

#include "PuppetPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_PhysicsRegion)

namespace PuppetModulePhysicsRegion
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_PhysicsRegion>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_PhysicsRegion>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			if (Settings.Region.IsNone())
			{
				OutError = TEXT("it names no region");
				return false;
			}
			if (!PuppetDynamics::BindPhysicsWeight(Settings.PhysicsWeight, Context.Performer, PhysicsWeightCurve, OutError)
				|| !PuppetDynamics::BindPhysicsWeight(Settings.Damping, Context.Performer, DampingCurve, OutError))
			{
				return false;
			}
			WeightName = PuppetPhysics::WeightCurve(Settings.Region);
			DampingName = PuppetPhysics::DampingCurve(Settings.Region);
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			// 0 is the physics as authored, so the module's weight scales towards it.
			Context.Puppet.SetCurve(WeightName, PuppetDynamics::GetPhysicsWeight(Settings.PhysicsWeight, PhysicsWeightCurve, Context) * Context.Weight);
			Context.Puppet.SetCurve(DampingName, PuppetDynamics::GetPhysicsWeight(Settings.Damping, DampingCurve, Context) * Context.Weight);
		}

	private:
		int32 PhysicsWeightCurve = INDEX_NONE;
		int32 DampingCurve = INDEX_NONE;
		FName WeightName;
		FName DampingName;
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
		return FName(FString::Printf(TEXT("Physics_%s"), *Region.ToString()));
	}

	FName DampingCurve(FName Region)
	{
		return FName(FString::Printf(TEXT("Physics_%s_Damping"), *Region.ToString()));
	}
}
