// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "AnimNode_PuppetSolve.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Open3DPuppetLog.h"
#include "PuppetAsset.h"
#include "PuppetCalibrationAsset.h"
#include "PuppetCalibrationSubsystem.h"
#include "PuppetInstance.h"
#include "PuppetLiveLink.h"
#include "PuppetPerformanceComponent.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_PuppetSolve)

namespace PuppetSolveNode
{
	const TCHAR* SlotStateText(EPuppetSlotState State)
	{
		switch (State)
		{
		case EPuppetSlotState::NoData: return TEXT("no performer data");
		case EPuppetSlotState::NotCalibrated: return TEXT("performer not calibrated");
		case EPuppetSlotState::CalibrationMismatch: return TEXT("calibration is from a different skeleton; calibrate again");
		default: return TEXT("driving");
		}
	}
}

void FAnimNode_PuppetSolve::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	Source.Initialize(Context);
	if (!Instance.IsValid())
	{
		Instance = MakeShared<FPuppetInstance>();
	}
	for (FCastClip& Clip : CastClips)
	{
		Clip.bRestart = true;
	}
}

void FAnimNode_PuppetSolve::PreUpdate(const UAnimInstance* InAnimInstance)
{
	LiveLinkClient = PuppetLiveLink::GetClient();

	if (Puppet != BoundPuppet || (Puppet && Puppet->GetRevision() != BoundRevision))
	{
		BoundPuppet = Puppet;
		BoundModules = Puppet ? Puppet->Modules : TArray<TObjectPtr<UPuppetModule>>();
		BoundRevision = Puppet ? Puppet->GetRevision() : INDEX_NONE;
		bNeedsInitialize = true;

		BoundRestPose = Puppet ? Puppet->RestPose : nullptr;
		BoundRestPoseTime = Puppet ? Puppet->RestPoseTime : 0.0f;
		FString Error;
		if (!RestPoseSampler.SetSequence(BoundRestPose, Error))
		{
			UE_LOG(LogOpen3DPuppet, Warning, TEXT("Puppet %s: rest pose %s cannot be used (%s); modules retarget from the mesh's reference pose."),
				*Puppet->GetName(), *GetNameSafe(BoundRestPose), *Error);
		}
	}

	// Cast can change through a pin; it is read here on the game thread for the calibrations and
	// to bind performer clips.
	CastCalibrations.SetNum(Cast.Num());
	CastClips.SetNum(Cast.Num());
	BoundClips.SetNum(Cast.Num());
	const UPuppetCalibrationSubsystem* Calibrations = GEngine ? GEngine->GetEngineSubsystem<UPuppetCalibrationSubsystem>() : nullptr;
	for (int32 Index = 0; Index < Cast.Num(); ++Index)
	{
		if (Cast[Index].Source == EPuppetCastSource::Animation)
		{
			UpdateClip(Index);
			continue;
		}
		if (CastClips[Index].Requested)
		{
			FString Unused;
			CastClips[Index].Sampler.SetSequence(nullptr, Unused);
			CastClips[Index].Requested = nullptr;
			BoundClips[Index] = nullptr;
		}

		FCastCalibration& Entry = CastCalibrations[Index];
		const FName Subject = Cast[Index].Subject.Name;
		int32 Revision = 0;
		const FPuppetPerformerCalibration* Calibration = Calibrations ? Calibrations->FindCalibration(Subject, Revision) : nullptr;
		// A live calibration wins over the saved one.
		const UPuppetCalibrationAsset* Saved = Cast[Index].SavedCalibration;
		const int32 SavedRevision = Saved ? Saved->GetRevision() : 0;
		if (!Calibration && Saved && Saved->HasCalibration())
		{
			Calibration = &Saved->GetCalibration();
		}
		if (Entry.Source != EPuppetCastSource::LiveLink || Subject != Entry.Subject || Revision != Entry.Revision
			|| Saved != Entry.Saved || SavedRevision != Entry.SavedRevision)
		{
			Entry.Source = EPuppetCastSource::LiveLink;
			Entry.Subject = Subject;
			Entry.Revision = Revision;
			Entry.Saved = Saved;
			Entry.SavedRevision = SavedRevision;
			Entry.bHasCalibration = Calibration != nullptr;
			Entry.Calibration = Calibration ? *Calibration : FPuppetPerformerCalibration();
			Entry.bDirty = true;
		}
	}

	// Without a performance component every slot is under its performer.
	const AActor* Owner = InAnimInstance ? InAnimInstance->GetOwningActor() : nullptr;
	const UPuppetPerformanceComponent* Performance = Owner ? Owner->FindComponentByClass<UPuppetPerformanceComponent>() : nullptr;
	const int32 NumSlots = BoundPuppet ? BoundPuppet->PerformerSlots.Num() : 0;
	SlotControls.SetNum(NumSlots);
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		const FName SlotName = BoundPuppet->PerformerSlots[Slot];
		SlotControls[Slot].ControlBlend = Performance ? Performance->GetControlBlend(SlotName) : 1.0f;
		SlotControls[Slot].RecentreCount = Performance ? Performance->GetRecentreCount(SlotName) : SlotControls[Slot].AppliedRecentreCount;
	}
}

void FAnimNode_PuppetSolve::UpdateClip(int32 Index)
{
	const FPuppetCastEntry& Entry = Cast[Index];
	FCastClip& Clip = CastClips[Index];
	FCastCalibration& Calibration = CastCalibrations[Index];

	const bool bSequenceChanged = Entry.Animation != Clip.Requested;
	if (bSequenceChanged)
	{
		Clip.Requested = Entry.Animation;
		BoundClips[Index] = Entry.Animation;
		Clip.bRestart = true;
		FString Error;
		if (!Clip.Sampler.SetSequence(Entry.Animation, Error))
		{
			UE_LOG(LogOpen3DPuppet, Warning, TEXT("Puppet %s, slot %s: animation %s cannot play as a performer: %s."),
				*GetNameSafe(BoundPuppet), *Entry.Slot.ToString(), *GetNameSafe(Entry.Animation), *Error);
		}
		// Sized here so sampling on the worker thread does not allocate.
		Clip.Local.SetNumUninitialized(Clip.Sampler.GetBoneNames().Num());
	}

	if (bSequenceChanged || Calibration.Source != EPuppetCastSource::Animation || Entry.ClipCalibration != Clip.CalibrationMode
		|| Entry.CalibrationTime != Clip.CalibrationTime || !Entry.CalibrationAlignment.Equals(Clip.CalibrationAlignment, 0.0))
	{
		Clip.CalibrationMode = Entry.ClipCalibration;
		Clip.CalibrationTime = Entry.CalibrationTime;
		Clip.CalibrationAlignment = Entry.CalibrationAlignment;
		Calibration.Source = EPuppetCastSource::Animation;
		Calibration.Subject = NAME_None;
		Calibration.Revision = 0;
		Calibration.bHasCalibration = Clip.Sampler.CaptureCalibration(Entry.ClipCalibration == EPuppetClipCalibration::ReferencePose,
			Entry.CalibrationTime, Entry.CalibrationAlignment, Calibration.Calibration);
		Calibration.bDirty = true;
	}
}

void FAnimNode_PuppetSolve::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_Base::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);

	// LOD changes also land here; only a different reference skeleton needs a new instance.
	const FReferenceSkeleton& RefSkeleton = Context.AnimInstanceProxy->GetRequiredBones().GetReferenceSkeleton();
	if (&RefSkeleton != BoundRefSkeleton || RefSkeleton.GetNum() != BoundNumBones)
	{
		bNeedsInitialize = true;
	}
}

void FAnimNode_PuppetSolve::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	Source.Update(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);
	AccumulatedDeltaTime += Context.GetDeltaTime();

	for (int32 Index = 0; Index < Cast.Num() && Index < CastClips.Num(); ++Index)
	{
		const FPuppetCastEntry& Entry = Cast[Index];
		FCastClip& Clip = CastClips[Index];
		if (Entry.Source != EPuppetCastSource::Animation || !Clip.Sampler.IsBound())
		{
			continue;
		}
		const double Length = Clip.Sampler.GetPlayLength();
		if (Entry.bUseTime)
		{
			Clip.Time = FPuppetClipSampler::PlaceTime(Entry.Time, Entry.bLoop, Length);
		}
		else if (Clip.bRestart)
		{
			Clip.Time = FPuppetClipSampler::PlaceTime(Entry.StartTime, Entry.bLoop, Length);
		}
		else
		{
			Clip.Time = FPuppetClipSampler::AdvanceTime(Clip.Time, Context.GetDeltaTime(), Entry.PlayRate, Entry.bLoop, Length);
		}
		Clip.bRestart = false;
	}
}

void FAnimNode_PuppetSolve::Evaluate_AnyThread(FPoseContext& Output)
{
	Source.Evaluate(Output);

	const float DeltaTime = AccumulatedDeltaTime;
	AccumulatedDeltaTime = 0.0f;
	if (!Instance.IsValid() || !BoundPuppet)
	{
		return;
	}

	const FBoneContainer& RequiredBones = Output.AnimInstanceProxy->GetRequiredBones();
	const FReferenceSkeleton& RefSkeleton = RequiredBones.GetReferenceSkeleton();
	if (bNeedsInitialize)
	{
		InitializeInstance(RefSkeleton);
	}

	// Bones outside this LOD keep their reference pose; no module output reaches them anyway.
	for (const FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
	{
		InputLocal[RequiredBones.MakeMeshPoseIndex(BoneIndex).GetInt()] = Output.Pose[BoneIndex];
	}

	ReadPerformers();
	ApplySlotControls();
	Instance->Solve(InputLocal, DeltaTime, Output.AnimInstanceProxy->GetComponentTransform());
	LogSlotStates();

	const FPuppetPose& Solved = Instance->GetPose();
	for (const FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
	{
		Output.Pose[BoneIndex] = Solved.GetLocal(RequiredBones.MakeMeshPoseIndex(BoneIndex).GetInt());
	}
	// Curves no module wrote pass through from Source (ADR 0006).
	const TConstArrayView<FName> CurveNames = Solved.GetCurveNames();
	const TConstArrayView<float> CurveValues = Solved.GetCurveValues();
	for (int32 Curve = 0; Curve < CurveNames.Num(); ++Curve)
	{
		Output.Curve.Set(CurveNames[Curve], CurveValues[Curve]);
	}
}

void FAnimNode_PuppetSolve::InitializeInstance(const FReferenceSkeleton& RefSkeleton)
{
	bNeedsInitialize = false;
	BoundRefSkeleton = &RefSkeleton;
	BoundNumBones = RefSkeleton.GetNum();
	TArray<FTransform> RestLocal;
	if (RestPoseSampler.IsBound())
	{
		RestPoseSampler.SampleOnto(BoundRestPoseTime, RefSkeleton, RestLocal);
	}
	Instance->Initialize(*BoundPuppet, BoundModules, RefSkeleton, RestLocal);
	InputLocal = RefSkeleton.GetRefBonePose();
	// INDEX_NONE matches no state, so each slot's first state is logged.
	LoggedSlotStates.Init(INDEX_NONE, Instance->NumSlots());
	for (FCastCalibration& Entry : CastCalibrations)
	{
		Entry.bDirty = true;
	}
}

void FAnimNode_PuppetSolve::ApplySlotControls()
{
	for (int32 Slot = 0; Slot < Instance->NumSlots() && Slot < SlotControls.Num(); ++Slot)
	{
		FSlotControl& Control = SlotControls[Slot];
		Instance->SetControlBlend(Slot, Control.ControlBlend);
		if (Control.RecentreCount != Control.AppliedRecentreCount)
		{
			Control.AppliedRecentreCount = Control.RecentreCount;
			Instance->RequestRecentre(Slot);
		}
	}
}

void FAnimNode_PuppetSolve::ReadPerformers()
{
	for (int32 Index = 0; Index < Cast.Num() && Index < CastCalibrations.Num(); ++Index)
	{
		const FPuppetCastEntry& Entry = Cast[Index];
		const int32 Slot = Instance->FindSlot(Entry.Slot);
		if (Slot == INDEX_NONE)
		{
			continue;
		}

		FCastCalibration& Calibration = CastCalibrations[Index];
		if (Calibration.bDirty)
		{
			Calibration.bDirty = false;
			if (Calibration.bHasCalibration)
			{
				Instance->SetCalibration(Slot, Calibration.Calibration);
			}
			else
			{
				Instance->ClearCalibration(Slot);
			}
		}

		if (Entry.Source == EPuppetCastSource::Animation)
		{
			FCastClip& Clip = CastClips[Index];
			if (!Clip.Sampler.Sample(Clip.Time, Clip.Local))
			{
				continue;
			}
			FString Error;
			if (Instance->SetPerformerSkeleton(Slot, Clip.Sampler.GetBoneNames(), Clip.Sampler.GetParents(), Entry.Profile, Error))
			{
				Instance->SetPerformerTransforms(Slot, Clip.Local);
			}
			else
			{
				UE_CLOG(!Error.IsEmpty(), LogOpen3DPuppet, Warning, TEXT("Puppet %s, slot %s: animation %s has an invalid skeleton: %s"),
					*BoundPuppet->GetName(), *Entry.Slot.ToString(), *GetNameSafe(Clip.Sampler.GetSequence()), *Error);
			}
			// A clip has no curves; none stay from a Live Link subject that played the slot before.
			Instance->SetPerformerCurveNames(Slot, TConstArrayView<FName>(), Entry.Profile);
			Instance->SetPerformerCurveValues(Slot, TConstArrayView<float>());
			continue;
		}

		// No frame this time: the slot keeps the performer's last pose and curves.
		FLiveLinkSubjectFrameData Frame;
		PuppetLiveLink::FSubjectData Data;
		if (!LiveLinkClient || !PuppetLiveLink::EvaluateSubject(*LiveLinkClient, Entry.Subject, Frame, Data))
		{
			continue;
		}
		if (Data.Skeleton && Data.Animation)
		{
			FString Error;
			if (Instance->SetPerformerSkeleton(Slot, Data.Skeleton->BoneNames, Data.Skeleton->BoneParents, Entry.Profile, Error))
			{
				Instance->SetPerformerTransforms(Slot, Data.Animation->Transforms);
			}
			else
			{
				// Empty when this skeleton was already reported.
				UE_CLOG(!Error.IsEmpty(), LogOpen3DPuppet, Warning, TEXT("Puppet %s, slot %s: Live Link subject %s has an invalid skeleton: %s"),
					*BoundPuppet->GetName(), *Entry.Slot.ToString(), *Entry.Subject.ToString(), *Error);
			}
		}
		Instance->SetPerformerCurveNames(Slot, Data.Static->PropertyNames, Entry.Profile);
		Instance->SetPerformerCurveValues(Slot, Data.Frame->PropertyValues);
	}
}

void FAnimNode_PuppetSolve::LogSlotStates()
{
	for (int32 Slot = 0; Slot < Instance->NumSlots(); ++Slot)
	{
		const EPuppetSlotState State = Instance->GetSlotState(Slot);
		if (static_cast<int32>(State) != LoggedSlotStates[Slot])
		{
			LoggedSlotStates[Slot] = static_cast<int32>(State);
			UE_LOG(LogOpen3DPuppet, Log, TEXT("Puppet %s, slot %s: %s."), *BoundPuppet->GetName(), *Instance->GetSlotName(Slot).ToString(), PuppetSolveNode::SlotStateText(State));
		}
	}
}

void FAnimNode_PuppetSolve::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(Puppet: %s)"), Puppet ? *Puppet->GetName() : TEXT("none"));
	DebugData.AddDebugItem(DebugLine);
	Source.GatherDebugData(DebugData);
}
