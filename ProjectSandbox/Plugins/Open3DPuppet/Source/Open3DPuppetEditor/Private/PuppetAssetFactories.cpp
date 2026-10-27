// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetAssetFactories.h"

#include "PuppetAsset.h"
#include "PuppetCalibrationAsset.h"
#include "PuppetPerformerProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetAssetFactories)

UPuppetAssetFactoryBase::UPuppetAssetFactoryBase()
{
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UPuppetAssetFactoryBase::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UObject>(InParent, InClass, InName, Flags | RF_Transactional);
}

UPuppetAssetFactory::UPuppetAssetFactory()
{
	SupportedClass = UPuppetAsset::StaticClass();
}

UPuppetPerformerProfileFactory::UPuppetPerformerProfileFactory()
{
	SupportedClass = UPuppetPerformerProfile::StaticClass();
}

UPuppetCalibrationAssetFactory::UPuppetCalibrationAssetFactory()
{
	SupportedClass = UPuppetCalibrationAsset::StaticClass();
}
