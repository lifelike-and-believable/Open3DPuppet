// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetSettings)

UPuppetSettings::UPuppetSettings()
{
	CategoryName = TEXT("Plugins");
	NamingSchemes = PuppetNaming::DefaultSchemes();
}
