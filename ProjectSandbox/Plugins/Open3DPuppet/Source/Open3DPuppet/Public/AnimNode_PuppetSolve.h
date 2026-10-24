// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Animation/AnimNodeBase.h"
#include "LiveLinkTypes.h"
#include "PuppetPerformerCalibration.h"
#include "Templates/SharedPointer.h"

#include "AnimNode_PuppetSolve.generated.h"

class FPuppetInstance;
class ILiveLinkClient;
class UPuppetAsset;
class UPuppetModule;
class UPuppetPerformerProfile;
struct FReferenceSkeleton;

/** Binds one of a puppet's performer slots to a Live Link subject (ADR 0004). */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetCastEntry
{
	GENERATED_BODY()

	/** The performer slot, as named in the puppet asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
	FName Slot = TEXT("Body");

	/** The Live Link subject (Animation role) that plays the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
	FLiveLinkSubjectName Subject;

	/** Maps the subject's bone names to canonical joint names; leave empty for mannequin-named sources. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
	TObjectPtr<UPuppetPerformerProfile> Profile;
};

/**
 * Puppet Solve: drives the mesh from live or recorded performers through a puppet's modules
 * (design overview §3). Bones no module writes pass through from Source.
 */
USTRUCT(BlueprintInternalUseOnly)
struct OPEN3DPUPPET_API FAnimNode_PuppetSolve : public FAnimNode_Base
{
	GENERATED_BODY()

	/** The pose modules start from and blend over. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Links")
	FPoseLink Source;

	/** The puppet: its performer slots and modules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puppet", meta = (PinShownByDefault))
	TObjectPtr<UPuppetAsset> Puppet;

	/** Which Live Link subject plays each performer slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puppet", meta = (PinShownByDefault))
	TArray<FPuppetCastEntry> Cast;

	//~ FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	//~ End of FAnimNode_Base interface

private:
	/** The calibration a cast entry's subject had when PreUpdate last looked, waiting to be applied. */
	struct FCastCalibration
	{
		FName Subject;
		int32 Revision = 0;
		FPuppetPerformerCalibration Calibration;
		bool bHasCalibration = false;
		bool bDirty = true;
	};

	/** A slot's hand-off state from the actor's UPuppetPerformanceComponent (ADR 0008), copied in PreUpdate. */
	struct FSlotControl
	{
		float ControlBlend = 1.0f;
		int32 RecentreCount = 0;
		/** The RecentreCount the instance last re-centred for. */
		int32 AppliedRecentreCount = 0;
	};

	void InitializeInstance(const FReferenceSkeleton& RefSkeleton);
	void ApplySlotControls();
	void ReadPerformers();
	void LogSlotStates();

	/** The asset and modules the instance was built from, kept alive while the solvers use them. */
	UPROPERTY(Transient)
	TObjectPtr<UPuppetAsset> BoundPuppet;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPuppetModule>> BoundModules;

	int32 BoundRevision = INDEX_NONE;
	bool bNeedsInitialize = true;
	const FReferenceSkeleton* BoundRefSkeleton = nullptr;
	int32 BoundNumBones = 0;
	ILiveLinkClient* LiveLinkClient = nullptr;
	float AccumulatedDeltaTime = 0.0f;

	TSharedPtr<FPuppetInstance> Instance;
	TArray<FCastCalibration> CastCalibrations;
	/** Indexed like the bound puppet's performer slots. */
	TArray<FSlotControl> SlotControls;
	TArray<FTransform> InputLocal;
	/** The EPuppetSlotState last logged per slot. */
	TArray<int32> LoggedSlotStates;
};
