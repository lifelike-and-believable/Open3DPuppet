// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "PuppetNaming.h"

#include "PuppetSettings.generated.h"

/** Project Settings > Plugins > Open3DPuppet. */
UCLASS(config = Open3DPuppet, defaultconfig, meta = (DisplayName = "Open3DPuppet"))
class OPEN3DPUPPET_API UPuppetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPuppetSettings();

	/**
	 * How capture systems name their bones, for filling a performer profile from a source skeleton
	 * (Map From Skeleton, Map From Live Link Subject). Starts with the plugin's defaults; edit a
	 * scheme to match what your system streams, or add one.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Performer Profiles")
	TArray<FPuppetNamingScheme> NamingSchemes;
};
