// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetSession.h"

#include "AnimNode_PuppetSolve.h"

namespace PuppetSession
{
	TArray<FPerformer> GatherPerformers(const ILiveLinkClient* Client, const UPuppetCalibrationSubsystem* Calibrations)
	{
		return {};
	}

	const FAnimNode_PuppetSolve* FPuppet::FindNode() const
	{
		return nullptr;
	}

	UPuppetAsset* FPuppet::GetPuppet() const
	{
		return nullptr;
	}

	TArray<FPuppet> GatherPuppets(UWorld* World)
	{
		return {};
	}

	UWorld* SessionWorld()
	{
		return nullptr;
	}

	FString SlotStateText(int32 State)
	{
		return FString();
	}
}
