// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"

class ILiveLinkClient;
class UAnimInstance;
class UPuppetAsset;
class UPuppetCalibrationSubsystem;
class UPuppetPerformanceComponent;
class UWorld;
struct FAnimNode_PuppetSolve;

/** What the Puppet Session panel shows (WP-8), gathered on the game thread. */
namespace PuppetSession
{
	/** A performer the session knows: a Live Link subject, a calibrated subject, or both. */
	struct FPerformer
	{
		FName Subject;
		/** Live Link has the subject now. */
		bool bStreaming = false;
		/** The Puppet Calibration subsystem has a calibration for it. */
		bool bCalibrated = false;
	};

	/** Every Live Link subject (Client may be null) and every calibrated subject, sorted by name. */
	OPEN3DPUPPETEDITOR_API TArray<FPerformer> GatherPerformers(const ILiveLinkClient* Client, const UPuppetCalibrationSubsystem* Calibrations);

	/** A running Puppet Solve node: the anim instance it lives in and its property there. */
	struct FPuppet
	{
		TWeakObjectPtr<UAnimInstance> AnimInstance;
		/** The node's property in the anim instance's class; the node is found again from it each refresh. */
		FName NodeProperty;
		/** The owning actor's label and the node, for the panel. */
		FString Label;
		TWeakObjectPtr<UPuppetPerformanceComponent> Performance;

		/** The node, or null when the anim instance is gone or was recompiled. Do not keep the pointer. */
		const FAnimNode_PuppetSolve* FindNode() const;
		/** The puppet asset the node runs, or null. */
		UPuppetAsset* GetPuppet() const;
	};

	/** Every Puppet Solve node in World's skeletal meshes, linked anim graphs included. */
	OPEN3DPUPPETEDITOR_API TArray<FPuppet> GatherPuppets(UWorld* World);

	/** The world the session runs in: a Play In Editor world while one exists, else the editor's. */
	OPEN3DPUPPETEDITOR_API UWorld* SessionWorld();

	/** How the panel names a slot state. */
	OPEN3DPUPPETEDITOR_API FString SlotStateText(int32 State);
}
