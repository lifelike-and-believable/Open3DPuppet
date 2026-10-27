// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetSession.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
struct FPuppetSlotStatus;
class SVerticalBox;
class UPuppetCalibrationAsset;

/**
 * The Puppet Session panel (WP-8, Window > Tools > Puppet Session): the session's performers and
 * their calibration, each running puppet's slots and hand-off controls, and the selected puppet's
 * modules to tune live. It refreshes four times a second and rebuilds its rows only when the
 * performers or puppets change, so a slider being dragged is not replaced under the cursor.
 */
class SPuppetSessionPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPuppetSessionPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EActiveTimerReturnType Refresh(double InCurrentTime, float InDeltaTime);
	void RebuildPerformers();
	void RebuildPuppets();
	TSharedRef<SWidget> MakePerformerRow(FName Subject);
	TSharedRef<SWidget> MakeSlotRow(int32 PuppetIndex, int32 SlotIndex);

	/** The slot's status in the puppet's node as of its last PreUpdate, or null. */
	const FPuppetSlotStatus* FindStatus(int32 PuppetIndex, int32 SlotIndex) const;

	TSharedPtr<SVerticalBox> PerformersBox;
	TSharedPtr<SVerticalBox> PuppetsBox;
	TSharedPtr<IDetailsView> TuningView;

	TArray<PuppetSession::FPerformer> Performers;
	TArray<PuppetSession::FPuppet> Puppets;
	/** What the rows were last built from; a change rebuilds them. */
	FString PerformersKey;
	FString PuppetsKey;

	/** The yaw Calibrate and Capture use, in degrees. */
	float AlignmentYaw = 0.0f;
	/** The duration hand-off transitions use, in seconds. */
	float TransitionDuration = 1.0f;
	/** Where Save To Asset captures a performer. */
	TWeakObjectPtr<UPuppetCalibrationAsset> CaptureAsset;
};
