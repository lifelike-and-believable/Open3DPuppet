// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetTestModules.generated.h"

/** Test-only module: writes a puppet curve from a performer curve's change since neutral. */
UCLASS(HideDropdown)
class UPuppetTestModule_CurveCopy : public UPuppetModule
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName PerformerCurve;

	UPROPERTY()
	FName PuppetCurve;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
