// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetCalibrationSubsystem.h"

#include "Open3DPuppetLog.h"
#include "PuppetLiveLink.h"
#include "PuppetPerformerPose.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetCalibrationSubsystem)

bool UPuppetCalibrationSubsystem::CalibratePerformer(FLiveLinkSubjectName Subject, FRotator Alignment)
{
	ILiveLinkClient* Client = PuppetLiveLink::GetClient();
	if (!Client)
	{
		UE_LOG(LogOpen3DPuppet, Warning, TEXT("Cannot calibrate %s: Live Link is not available."), *Subject.ToString());
		return false;
	}
	FLiveLinkSubjectFrameData Frame;
	PuppetLiveLink::FSubjectData Data;
	if (!PuppetLiveLink::EvaluateSubject(*Client, Subject, Frame, Data))
	{
		UE_LOG(LogOpen3DPuppet, Warning, TEXT("Cannot calibrate %s: the subject has no frame."), *Subject.ToString());
		return false;
	}

	FPuppetPerformerPose Pose;
	if (Data.HasBones())
	{
		FString Error;
		if (!Pose.SetSkeleton(Data.BoneNames, Data.BoneParents, nullptr, Error) || !Pose.SetLocalTransforms(Data.BoneTransforms))
		{
			UE_LOG(LogOpen3DPuppet, Warning, TEXT("Cannot calibrate %s: %s"), *Subject.ToString(),
				Error.IsEmpty() ? TEXT("the frame's transform count does not match its skeleton") : *Error);
			return false;
		}
	}
	Pose.SetCurveNames(Data.Static->PropertyNames, nullptr);
	Pose.SetCurveValues(Data.Frame->PropertyValues);
	if (!Pose.HasData())
	{
		UE_LOG(LogOpen3DPuppet, Warning, TEXT("Cannot calibrate %s: the subject has neither bones nor curves."), *Subject.ToString());
		return false;
	}
	SetCalibration(Subject, FPuppetPerformerCalibration::Capture(Pose, Alignment));
	UE_LOG(LogOpen3DPuppet, Log, TEXT("Calibrated performer %s (%d bones, %d curves)."), *Subject.ToString(), Pose.Num(), Pose.GetCurveNames().Num());
	return true;
}

void UPuppetCalibrationSubsystem::SetCalibration(FLiveLinkSubjectName Subject, const FPuppetPerformerCalibration& Calibration)
{
	FEntry& Entry = Calibrations.FindOrAdd(Subject.Name);
	Entry.Calibration = Calibration;
	Entry.Revision = ++LastRevision;
}

bool UPuppetCalibrationSubsystem::GetCalibration(FLiveLinkSubjectName Subject, FPuppetPerformerCalibration& OutCalibration) const
{
	const FEntry* Entry = Calibrations.Find(Subject.Name);
	if (!Entry)
	{
		return false;
	}
	OutCalibration = Entry->Calibration;
	return true;
}

void UPuppetCalibrationSubsystem::ClearCalibration(FLiveLinkSubjectName Subject)
{
	Calibrations.Remove(Subject.Name);
}

const FPuppetPerformerCalibration* UPuppetCalibrationSubsystem::FindCalibration(FName Subject, int32& OutRevision) const
{
	const FEntry* Entry = Calibrations.Find(Subject);
	OutRevision = Entry ? Entry->Revision : 0;
	return Entry ? &Entry->Calibration : nullptr;
}
