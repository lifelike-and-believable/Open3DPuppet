// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformanceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetPerformanceComponent)

UPuppetPerformanceComponent::UPuppetPerformanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
}

void UPuppetPerformanceComponent::TransitionToPerformer(FName Slot, float Duration, EPuppetEasing Easing)
{
}

void UPuppetPerformanceComponent::TransitionToAnimation(FName Slot, float Duration, EPuppetEasing Easing)
{
}

void UPuppetPerformanceComponent::SetControlBlend(FName Slot, float Blend)
{
}

float UPuppetPerformanceComponent::GetControlBlend(FName Slot) const
{
	return 1.0f;
}

bool UPuppetPerformanceComponent::IsTransitioning(FName Slot) const
{
	return false;
}

void UPuppetPerformanceComponent::Recentre(FName Slot)
{
}

void UPuppetPerformanceComponent::AdvanceTransitions(float DeltaTime)
{
}

int32 UPuppetPerformanceComponent::GetRecentreCount(FName Slot) const
{
	return 0;
}

void UPuppetPerformanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceTransitions(DeltaTime);
}

UPuppetPerformanceComponent::FSlotState& UPuppetPerformanceComponent::FindOrAddSlot(FName Slot)
{
	return Slots.FindOrAdd(Slot);
}

void UPuppetPerformanceComponent::StartTransition(FName Slot, float To, float Duration, EPuppetEasing Easing)
{
}
