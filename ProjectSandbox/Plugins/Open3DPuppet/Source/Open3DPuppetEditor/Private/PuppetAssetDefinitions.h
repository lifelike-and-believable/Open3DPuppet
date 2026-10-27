// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "AssetDefinitionDefault.h"

#include "PuppetAssetDefinitions.generated.h"

/**
 * How the Content Browser shows one of the plugin's asset types: its name, colour and the Puppet
 * category of the Add menu, where its factory (PuppetAssetFactories.h) creates it.
 */
UCLASS(Abstract)
class UAssetDefinition_PuppetBase : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
};

UCLASS()
class UAssetDefinition_PuppetAsset : public UAssetDefinition_PuppetBase
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual FLinearColor GetAssetColor() const override;
};

UCLASS()
class UAssetDefinition_PuppetPerformerProfile : public UAssetDefinition_PuppetBase
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual FLinearColor GetAssetColor() const override;
};

UCLASS()
class UAssetDefinition_PuppetCalibrationAsset : public UAssetDefinition_PuppetBase
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual FLinearColor GetAssetColor() const override;
};
