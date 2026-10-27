// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"

#include "PuppetAssetFactories.generated.h"

/**
 * Creates one of the plugin's asset types from the Content Browser's Add menu, where its asset
 * definition places it under Puppet, so a user creates a Puppet Asset directly rather than a Data
 * Asset whose class they then pick. Each subclass names its type.
 */
UCLASS(Abstract)
class UPuppetAssetFactoryBase : public UFactory
{
	GENERATED_BODY()

public:
	UPuppetAssetFactoryBase();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

/** Content Browser > Add > Puppet > Puppet Asset. */
UCLASS()
class UPuppetAssetFactory : public UPuppetAssetFactoryBase
{
	GENERATED_BODY()

public:
	UPuppetAssetFactory();
};

/** Content Browser > Add > Puppet > Puppet Performer Profile. */
UCLASS()
class UPuppetPerformerProfileFactory : public UPuppetAssetFactoryBase
{
	GENERATED_BODY()

public:
	UPuppetPerformerProfileFactory();
};

/** Content Browser > Add > Puppet > Puppet Calibration Asset. */
UCLASS()
class UPuppetCalibrationAssetFactory : public UPuppetAssetFactoryBase
{
	GENERATED_BODY()

public:
	UPuppetCalibrationAssetFactory();
};
