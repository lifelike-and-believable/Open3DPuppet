// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Animation/AnimNodeBase.h"
#include "LiveLinkTypes.h"
#include "PuppetClipSampler.h"
#include "PuppetInstance.h"
#include "PuppetPerformerCalibration.h"
#include "Templates/SharedPointer.h"

#include "AnimNode_PuppetSolve.generated.h"

class FPuppetInstance;
class ILiveLinkClient;
class UAnimSequence;
class UPuppetCalibrationAsset;
class UPuppetAsset;
class UPuppetModule;
class UPuppetPerformerProfile;
struct FReferenceSkeleton;

/** Where a performer slot's motion comes from (ADR 0008). */
UENUM(BlueprintType)
enum class EPuppetCastSource : uint8
{
	/** A Live Link subject: a live performer, or a take played back through Live Link. */
	LiveLink UMETA(DisplayName = "Live Link"),
	/** An AnimSequence of human motion on a performer skeleton (a recorded take), retargeted like live motion. */
	Animation,
};

/** What a performer clip is calibrated from (ADR 0008). */
UENUM(BlueprintType)
enum class EPuppetClipCalibration : uint8
{
	/** The clip's pose at Calibration Time: for a take that starts with the performer in the reference pose (T-pose). */
	Frame,
	/** The reference pose of the clip's skeleton. */
	ReferencePose UMETA(DisplayName = "Reference Pose"),
};

/** Binds one of a puppet's performer slots to a Live Link subject or a performer clip (ADR 0004, ADR 0008). */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetCastEntry
{
	GENERATED_BODY()

	/** The performer slot, as named in the puppet asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
	FName Slot = TEXT("Body");

	/** Where the slot's performer comes from: a Live Link subject or an AnimSequence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
	EPuppetCastSource Source = EPuppetCastSource::LiveLink;

	/** The Live Link subject (Animation role, or Basic for curves only) that plays the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast", meta = (EditCondition = "Source == EPuppetCastSource::LiveLink", EditConditionHides))
	FLiveLinkSubjectName Subject;

	/**
	 * A saved calibration, used while the subject has none in the Puppet Calibration subsystem, so
	 * a show can start without calibrating. A calibration captured since (Calibrate Performer, or
	 * Capture on any calibration asset) takes precedence.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast", meta = (EditCondition = "Source == EPuppetCastSource::LiveLink", EditConditionHides))
	TObjectPtr<UPuppetCalibrationAsset> SavedCalibration;

	/**
	 * The performer clip: human motion on its own (performer) skeleton, retargeted through the
	 * modules like a live performer. Its bones only; its curves are not read. Bones keep the take's
	 * own translations whatever the skeleton's retargeting modes, but the sequence's Force Root Lock
	 * still pins its root. A clip authored on the puppet's own skeleton plays into the node's Source
	 * pose instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides))
	TObjectPtr<UAnimSequence> Animation;

	/** Playback speed; negative plays backwards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides))
	float PlayRate = 1.0f;

	/** Wraps at the ends; otherwise the clip holds its last frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides))
	bool bLoop = true;

	/** Where playback starts, in seconds, when the clip is first played or replaced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides, ClampMin = "0"))
	float StartTime = 0.0f;

	/** Shows the clip at Time instead of playing it, so Sequencer or a Blueprint can scrub the take through the Cast pin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides))
	bool bUseTime = false;

	/** The clip time to show, in seconds, when Use Time is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation && bUseTime", EditConditionHides))
	float Time = 0.0f;

	/** The clip's rest pose for retargeting: a frame of the clip, or its skeleton's reference pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides))
	EPuppetClipCalibration ClipCalibration = EPuppetClipCalibration::Frame;

	/** The frame calibrated from, in seconds (the first frame by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation && ClipCalibration == EPuppetClipCalibration::Frame", EditConditionHides, ClampMin = "0"))
	float CalibrationTime = 0.0f;

	/** Rotation from the clip's facing to the puppet's, as a live calibration's alignment (ADR 0003). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Animation", meta = (EditCondition = "Source == EPuppetCastSource::Animation", EditConditionHides))
	FRotator CalibrationAlignment = FRotator::ZeroRotator;

	/** Maps the subject's bone names to canonical joint names; leave empty for mannequin-named sources. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
	TObjectPtr<UPuppetPerformerProfile> Profile;
};

/** One performer slot as the Puppet Solve node last ran it, for the session panel (WP-8). */
struct FPuppetSlotStatus
{
	FName Slot;
	EPuppetCastSource Source = EPuppetCastSource::LiveLink;
	/** The Live Link subject, or the clip's name; None when nothing is cast into the slot. */
	FName Performer;
	EPuppetSlotState State = EPuppetSlotState::NoData;
	float ControlBlend = 1.0f;
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

	/** Which Live Link subject or performer clip plays each performer slot. */
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

	/**
	 * Each of the puppet's slots as of the last PreUpdate, which runs on the game thread after the
	 * previous evaluation finished, so the session panel can read it safely there.
	 */
	TConstArrayView<FPuppetSlotStatus> GetSlotStatus() const { return SlotStatus; }

private:
	/** The calibration a cast entry's subject or clip had when PreUpdate last looked, waiting to be applied. */
	struct FCastCalibration
	{
		EPuppetCastSource Source = EPuppetCastSource::LiveLink;
		FName Subject;
		int32 Revision = 0;
		/** The saved calibration asset and its revision, for a subject the subsystem has no calibration for. */
		const UPuppetCalibrationAsset* Saved = nullptr;
		int32 SavedRevision = 0;
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

	/** A cast entry's performer clip (ADR 0008): bound in PreUpdate, played in Update, sampled in Evaluate. */
	struct FCastClip
	{
		FPuppetClipSampler Sampler;
		/** What the sampler and calibration were made from; a change binds or calibrates again. */
		const UAnimSequence* Requested = nullptr;
		EPuppetClipCalibration CalibrationMode = EPuppetClipCalibration::Frame;
		float CalibrationTime = 0.0f;
		FRotator CalibrationAlignment = FRotator::ZeroRotator;
		double Time = 0.0;
		/** Playback starts again from the entry's StartTime. */
		bool bRestart = true;
		TArray<FTransform> Local;
	};

	void InitializeInstance(const FReferenceSkeleton& RefSkeleton);
	void UpdateClip(int32 Index);
	void ApplySlotControls();
	void ReadPerformers();
	void LogSlotStates();

	/** The asset and modules the instance was built from, kept alive while the solvers use them. */
	UPROPERTY(Transient)
	TObjectPtr<UPuppetAsset> BoundPuppet;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPuppetModule>> BoundModules;

	/** The puppet's rest pose clip (UPuppetAsset::RestPose), kept alive while it is sampled. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> BoundRestPose;

	/** The performer clips the samplers play, indexed like Cast, kept alive while they are sampled. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> BoundClips;

	int32 BoundRevision = INDEX_NONE;
	bool bNeedsInitialize = true;
	const FReferenceSkeleton* BoundRefSkeleton = nullptr;
	int32 BoundNumBones = 0;
	ILiveLinkClient* LiveLinkClient = nullptr;
	float AccumulatedDeltaTime = 0.0f;

	TSharedPtr<FPuppetInstance> Instance;
	TArray<FCastCalibration> CastCalibrations;
	TArray<FCastClip> CastClips;
	FPuppetClipSampler RestPoseSampler;
	float BoundRestPoseTime = 0.0f;
	/** Indexed like the bound puppet's performer slots. */
	TArray<FSlotControl> SlotControls;
	TArray<FPuppetSlotStatus> SlotStatus;
	TArray<FTransform> InputLocal;
	/** The EPuppetSlotState last logged per slot. */
	TArray<int32> LoggedSlotStates;
};
