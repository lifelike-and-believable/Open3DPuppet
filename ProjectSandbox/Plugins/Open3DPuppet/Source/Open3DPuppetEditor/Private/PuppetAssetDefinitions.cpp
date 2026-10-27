// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetAssetDefinitions.h"

#include "PuppetAsset.h"
#include "PuppetCalibrationAsset.h"
#include "PuppetPerformerProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetAssetDefinitions)

#define LOCTEXT_NAMESPACE "Open3DPuppet"

TConstArrayView<FAssetCategoryPath> UAssetDefinition_PuppetBase::GetAssetCategories() const
{
	// A top-level category; the asset tools register it the first time the Add menu asks.
	static const FAssetCategoryPath Categories[] = { FAssetCategoryPath(LOCTEXT("PuppetCategory", "Puppet")) };
	return Categories;
}

FText UAssetDefinition_PuppetAsset::GetAssetDisplayName() const
{
	return LOCTEXT("PuppetAsset", "Puppet Asset");
}

TSoftClassPtr<UObject> UAssetDefinition_PuppetAsset::GetAssetClass() const
{
	return UPuppetAsset::StaticClass();
}

FLinearColor UAssetDefinition_PuppetAsset::GetAssetColor() const
{
	return FLinearColor(FColor(230, 140, 40));
}

FText UAssetDefinition_PuppetPerformerProfile::GetAssetDisplayName() const
{
	return LOCTEXT("PuppetPerformerProfile", "Puppet Performer Profile");
}

TSoftClassPtr<UObject> UAssetDefinition_PuppetPerformerProfile::GetAssetClass() const
{
	return UPuppetPerformerProfile::StaticClass();
}

FLinearColor UAssetDefinition_PuppetPerformerProfile::GetAssetColor() const
{
	return FLinearColor(FColor(60, 160, 220));
}

FText UAssetDefinition_PuppetCalibrationAsset::GetAssetDisplayName() const
{
	return LOCTEXT("PuppetCalibrationAsset", "Puppet Calibration Asset");
}

TSoftClassPtr<UObject> UAssetDefinition_PuppetCalibrationAsset::GetAssetClass() const
{
	return UPuppetCalibrationAsset::StaticClass();
}

FLinearColor UAssetDefinition_PuppetCalibrationAsset::GetAssetColor() const
{
	return FLinearColor(FColor(90, 190, 110));
}

#undef LOCTEXT_NAMESPACE
