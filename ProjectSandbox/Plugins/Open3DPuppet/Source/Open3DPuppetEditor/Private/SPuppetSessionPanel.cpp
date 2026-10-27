// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "SPuppetSessionPanel.h"

#include "AnimNode_PuppetSolve.h"
#include "Engine/Engine.h"
#include "Features/IModularFeatures.h"
#include "IDetailsView.h"
#include "ILiveLinkClient.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "PuppetAsset.h"
#include "PuppetCalibrationAsset.h"
#include "PuppetCalibrationSubsystem.h"
#include "PuppetPerformanceComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PuppetSession"

namespace PuppetSessionPanel
{
	ILiveLinkClient* LiveLinkClient()
	{
		IModularFeatures& Features = IModularFeatures::Get();
		return Features.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName)
			? &Features.GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName)
			: nullptr;
	}

	UPuppetCalibrationSubsystem* Calibrations()
	{
		return GEngine ? GEngine->GetEngineSubsystem<UPuppetCalibrationSubsystem>() : nullptr;
	}

	TSharedRef<SWidget> Heading(const FText& Text)
	{
		return SNew(SBox).Padding(FMargin(0.0f, 12.0f, 0.0f, 4.0f))
			[
				SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			];
	}

	TSharedRef<SWidget> Cell(const TAttribute<FText>& Text, float Width)
	{
		return SNew(SBox).WidthOverride(Width)[SNew(STextBlock).Text(Text)];
	}
}

void SPuppetSessionPanel::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TuningView = PropertyEditor.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).AutoWrapText(true)
					.Text_Lambda([]()
					{
						const UWorld* World = PuppetSession::SessionWorld();
						return FText::Format(LOCTEXT("World", "Session world: {0}. Hand-off transitions run in Play In Editor, or in the editor while the viewport is real-time."),
							World ? FText::FromString(World->GetMapName()) : LOCTEXT("NoWorld", "none"));
					})
			]

			+ SVerticalBox::Slot().AutoHeight()[PuppetSessionPanel::Heading(LOCTEXT("Performers", "Performers and calibration"))]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("Alignment", "Alignment yaw"))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(80.0f)
					[
						SNew(SSpinBox<float>).MinValue(-180.0f).MaxValue(180.0f)
							.Value_Lambda([this]() { return AlignmentYaw; })
							.OnValueChanged_Lambda([this](float Value) { AlignmentYaw = Value; })
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("SaveTo", "Save captures to"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(UPuppetCalibrationAsset::StaticClass())
						.ObjectPath_Lambda([this]() { return CaptureAsset.IsValid() ? CaptureAsset->GetPathName() : FString(); })
						.OnObjectChanged_Lambda([this](const FAssetData& Asset) { CaptureAsset = Cast<UPuppetCalibrationAsset>(Asset.GetAsset()); })
						.AllowClear(true)
						.DisplayThumbnail(false)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(PerformersBox, SVerticalBox)]

			+ SVerticalBox::Slot().AutoHeight()[PuppetSessionPanel::Heading(LOCTEXT("Puppets", "Running puppets"))]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("Duration", "Transition duration (s)"))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(80.0f)
					[
						SNew(SSpinBox<float>).MinValue(0.0f).MaxValue(30.0f)
							.Value_Lambda([this]() { return TransitionDuration; })
							.OnValueChanged_Lambda([this](float Value) { TransitionDuration = Value; })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(PuppetsBox, SVerticalBox)]

			+ SVerticalBox::Slot().AutoHeight()[PuppetSessionPanel::Heading(LOCTEXT("Tuning", "Module tuning (numbers apply live)"))]
			+ SVerticalBox::Slot().AutoHeight()[TuningView.ToSharedRef()]
		]
	];

	RegisterActiveTimer(0.25f, FWidgetActiveTimerDelegate::CreateSP(this, &SPuppetSessionPanel::Refresh));
	Refresh(0.0, 0.0f);
}

EActiveTimerReturnType SPuppetSessionPanel::Refresh(double InCurrentTime, float InDeltaTime)
{
	Performers = PuppetSession::GatherPerformers(PuppetSessionPanel::LiveLinkClient(), PuppetSessionPanel::Calibrations());
	FString Key;
	for (const PuppetSession::FPerformer& Performer : Performers)
	{
		Key += Performer.Subject.ToString() + TEXT(";");
	}
	if (Key != PerformersKey)
	{
		PerformersKey = Key;
		RebuildPerformers();
	}

	TArray<PuppetSession::FPuppet> Found = PuppetSession::GatherPuppets(PuppetSession::SessionWorld());
	Key.Reset();
	for (const PuppetSession::FPuppet& Puppet : Found)
	{
		const FAnimNode_PuppetSolve* Node = Puppet.FindNode();
		Key += FString::Printf(TEXT("%s/%s/%d;"), *Puppet.Label, *Puppet.NodeProperty.ToString(), Node ? Node->GetSlotStatus().Num() : 0);
	}
	if (Key != PuppetsKey)
	{
		PuppetsKey = Key;
		Puppets = MoveTemp(Found);
		RebuildPuppets();
	}
	return EActiveTimerReturnType::Continue;
}

void SPuppetSessionPanel::RebuildPerformers()
{
	PerformersBox->ClearChildren();
	if (Performers.Num() == 0)
	{
		PerformersBox->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NoPerformers", "No Live Link subjects and no calibrations."))];
	}
	for (const PuppetSession::FPerformer& Performer : Performers)
	{
		PerformersBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)[MakePerformerRow(Performer.Subject)];
	}
}

TSharedRef<SWidget> SPuppetSessionPanel::MakePerformerRow(FName Subject)
{
	FLiveLinkSubjectName SubjectName;
	SubjectName.Name = Subject;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[PuppetSessionPanel::Cell(FText::FromName(Subject), 200.0f)]
		+ SHorizontalBox::Slot().AutoWidth()[PuppetSessionPanel::Cell(TAttribute<FText>::CreateLambda([Subject]()
			{
				const ILiveLinkClient* Client = PuppetSessionPanel::LiveLinkClient();
				FLiveLinkSubjectName Name;
				Name.Name = Subject;
				return Client && Client->IsSubjectValid(Name) ? LOCTEXT("Streaming", "Streaming") : LOCTEXT("NotStreaming", "Not streaming");
			}), 110.0f)]
		+ SHorizontalBox::Slot().AutoWidth()[PuppetSessionPanel::Cell(TAttribute<FText>::CreateLambda([Subject]()
			{
				int32 Revision = 0;
				const UPuppetCalibrationSubsystem* Calibrations = PuppetSessionPanel::Calibrations();
				return Calibrations && Calibrations->FindCalibration(Subject, Revision) ? LOCTEXT("Calibrated", "Calibrated") : LOCTEXT("NotCalibrated", "Not calibrated");
			}), 110.0f)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SButton).Text(LOCTEXT("Calibrate", "Calibrate"))
				.ToolTipText(LOCTEXT("CalibrateTip", "Capture the performer's current pose as their rest pose, with the alignment yaw. Have them stand in the reference pose first."))
				.OnClicked_Lambda([this, SubjectName]()
				{
					if (UPuppetCalibrationSubsystem* Calibrations = PuppetSessionPanel::Calibrations())
					{
						Calibrations->CalibratePerformer(SubjectName, FRotator(0.0f, AlignmentYaw, 0.0f));
					}
					return FReply::Handled();
				})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SButton).Text(LOCTEXT("SaveToAsset", "Save to asset"))
				.ToolTipText(LOCTEXT("SaveToAssetTip", "Capture the performer into the chosen calibration asset, which also makes it their calibration."))
				.IsEnabled_Lambda([this]() { return CaptureAsset.IsValid(); })
				.OnClicked_Lambda([this, SubjectName]()
				{
					if (UPuppetCalibrationAsset* Asset = CaptureAsset.Get())
					{
						Asset->Subject = SubjectName;
						Asset->Alignment = FRotator(0.0f, AlignmentYaw, 0.0f);
						Asset->Capture();
					}
					return FReply::Handled();
				})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
		[
			SNew(SButton).Text(LOCTEXT("Clear", "Clear"))
				.ToolTipText(LOCTEXT("ClearTip", "Forget the performer's calibration; puppets they drive stop following them."))
				.OnClicked_Lambda([SubjectName]()
				{
					if (UPuppetCalibrationSubsystem* Calibrations = PuppetSessionPanel::Calibrations())
					{
						Calibrations->ClearCalibration(SubjectName);
					}
					return FReply::Handled();
				})
		];
}

void SPuppetSessionPanel::RebuildPuppets()
{
	PuppetsBox->ClearChildren();
	if (Puppets.Num() == 0)
	{
		PuppetsBox->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NoPuppets", "No skeletal mesh in the session world runs a Puppet Solve node."))];
	}
	for (int32 PuppetIndex = 0; PuppetIndex < Puppets.Num(); ++PuppetIndex)
	{
		const PuppetSession::FPuppet& Puppet = Puppets[PuppetIndex];
		UPuppetAsset* Asset = Puppet.GetPuppet();
		PuppetsBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Text(FText::Format(LOCTEXT("PuppetLabel", "{0}: {1}"), FText::FromString(Puppet.Label), Asset ? FText::FromString(Asset->GetName()) : LOCTEXT("NoAsset", "no puppet asset")))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("Tune", "Tune"))
					.ToolTipText(LOCTEXT("TuneTip", "Show this puppet's asset below; module numbers apply live."))
					.IsEnabled(Asset != nullptr)
					.OnClicked_Lambda([this, Asset = TWeakObjectPtr<UPuppetAsset>(Asset)]()
					{
						TuningView->SetObject(Asset.Get());
						return FReply::Handled();
					})
			]
		];
		const FAnimNode_PuppetSolve* Node = Puppet.FindNode();
		const int32 NumSlots = Node ? Node->GetSlotStatus().Num() : 0;
		for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
		{
			PuppetsBox->AddSlot().AutoHeight().Padding(16.0f, 2.0f, 0.0f, 2.0f)[MakeSlotRow(PuppetIndex, SlotIndex)];
		}
	}
}

const FPuppetSlotStatus* SPuppetSessionPanel::FindStatus(int32 PuppetIndex, int32 SlotIndex) const
{
	// The node is found again on every read: a recompiled anim class replaces it.
	const FAnimNode_PuppetSolve* Node = Puppets.IsValidIndex(PuppetIndex) ? Puppets[PuppetIndex].FindNode() : nullptr;
	return Node && Node->GetSlotStatus().IsValidIndex(SlotIndex) ? &Node->GetSlotStatus()[SlotIndex] : nullptr;
}

TSharedRef<SWidget> SPuppetSessionPanel::MakeSlotRow(int32 PuppetIndex, int32 SlotIndex)
{
	const FPuppetSlotStatus* Initial = FindStatus(PuppetIndex, SlotIndex);
	const FName Slot = Initial ? Initial->Slot : NAME_None;
	const TWeakObjectPtr<UPuppetPerformanceComponent> Performance = Puppets[PuppetIndex].Performance;

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[PuppetSessionPanel::Cell(FText::FromName(Slot), 90.0f)]
		+ SHorizontalBox::Slot().AutoWidth()[PuppetSessionPanel::Cell(TAttribute<FText>::CreateLambda([this, PuppetIndex, SlotIndex]()
			{
				const FPuppetSlotStatus* Status = FindStatus(PuppetIndex, SlotIndex);
				if (!Status)
				{
					return FText::GetEmpty();
				}
				const FText Source = Status->Source == EPuppetCastSource::Animation ? LOCTEXT("Clip", "Clip") : LOCTEXT("LiveLink", "Live Link");
				return FText::Format(LOCTEXT("SlotSource", "{0}: {1}"), Source, Status->Performer.IsNone() ? LOCTEXT("NothingCast", "nothing cast") : FText::FromName(Status->Performer));
			}), 240.0f)]
		+ SHorizontalBox::Slot().AutoWidth()[PuppetSessionPanel::Cell(TAttribute<FText>::CreateLambda([this, PuppetIndex, SlotIndex]()
			{
				const FPuppetSlotStatus* Status = FindStatus(PuppetIndex, SlotIndex);
				return Status ? FText::FromString(PuppetSession::SlotStateText(static_cast<int32>(Status->State))) : FText::GetEmpty();
			}), 190.0f)];

	if (!Performance.IsValid())
	{
		Row->AddSlot().AutoWidth()[SNew(STextBlock).Text(LOCTEXT("NoPerformance", "No Puppet Performance Component: always under the performer"))];
		return Row;
	}
	Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("Blend", "Control"))];
	Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(120.0f)
		[
			SNew(SSlider)
				.ToolTipText(LOCTEXT("BlendTip", "The slot's control blend: 0 is the animation, 1 the performer."))
				.Value_Lambda([Performance, Slot]() { return Performance.IsValid() ? Performance->GetControlBlend(Slot) : 0.0f; })
				.OnValueChanged_Lambda([Performance, Slot](float Value) { if (Performance.IsValid()) { Performance->SetControlBlend(Slot, Value); } })
		]
	];
	const auto Button = [](const FText& Text, const FText& Tip, TFunction<void()> Action)
	{
		return SNew(SButton).Text(Text).ToolTipText(Tip).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); });
	};
	Row->AddSlot().AutoWidth().Padding(2.0f, 0.0f)[Button(LOCTEXT("ToPerformer", "To performer"), LOCTEXT("ToPerformerTip", "Hand the slot to its performer over the transition duration."),
		[this, Performance, Slot]() { if (Performance.IsValid()) { Performance->TransitionToPerformer(Slot, TransitionDuration); } })];
	Row->AddSlot().AutoWidth().Padding(2.0f, 0.0f)[Button(LOCTEXT("ToAnimation", "To animation"), LOCTEXT("ToAnimationTip", "Hand the slot back to the animation over the transition duration."),
		[this, Performance, Slot]() { if (Performance.IsValid()) { Performance->TransitionToAnimation(Slot, TransitionDuration); } })];
	Row->AddSlot().AutoWidth().Padding(2.0f, 0.0f)[Button(LOCTEXT("Recentre", "Re-centre"), LOCTEXT("RecentreTip", "Keep the puppet where it is and converge on the performer from here."),
		[Performance, Slot]() { if (Performance.IsValid()) { Performance->Recentre(Slot); } })];
	return Row;
}

#undef LOCTEXT_NAMESPACE
