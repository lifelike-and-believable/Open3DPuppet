// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetAsset.h"

#include "PuppetModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetAsset)

TArray<FName> UPuppetAsset::GetPuppetBoneOptions() const
{
	return {};
}

TArray<FName> UPuppetAsset::GetPerformerSlotOptions() const
{
	return {};
}

#if WITH_EDITOR
void UPuppetAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	MarkChanged();
}
#endif
