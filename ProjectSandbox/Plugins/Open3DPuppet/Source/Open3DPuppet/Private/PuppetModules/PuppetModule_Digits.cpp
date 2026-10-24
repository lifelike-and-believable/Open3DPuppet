// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Digits.h"

#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Digits)

namespace PuppetModuleDigits
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Digits>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Digits>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
		}
	};
}

UPuppetModule_Digits::UPuppetModule_Digits()
{
	for (const TCHAR* Finger : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
	{
		FPuppetDigit& Digit = Digits.AddDefaulted_GetRef();
		Digit.PerformerStart = Digit.PuppetStart = *FString::Printf(TEXT("%s_01_l"), Finger);
		Digit.PerformerEnd = Digit.PuppetEnd = *FString::Printf(TEXT("%s_03_l"), Finger);
	}
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Digits::CreateSolver() const
{
	return MakeUnique<PuppetModuleDigits::FSolver>(*this);
}
