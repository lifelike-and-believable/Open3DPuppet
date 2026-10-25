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

/** Test-only module: records the world motion its last solve context carried (ADR 0007). */
UCLASS(HideDropdown)
class UPuppetTestModule_WorldMotion : public UPuppetModule
{
	GENERATED_BODY()

public:
	// Written by the solver for the tests to read; not settings.
	mutable FTransform LastComponentToWorld = FTransform::Identity;
	mutable FTransform LastPreviousComponentToWorld = FTransform::Identity;
	mutable bool bLastTeleport = false;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
