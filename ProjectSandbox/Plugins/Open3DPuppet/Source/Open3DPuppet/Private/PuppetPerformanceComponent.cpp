// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformanceComponent.h"

#include "Math/UnrealMathUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetPerformanceComponent)

UPuppetPerformanceComponent::UPuppetPerformanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Hand-offs are rehearsed in the editor too, from Sequencer or a button.
	bTickInEditor = true;
}

void UPuppetPerformanceComponent::TransitionToPerformer(FName Slot, float Duration, EPuppetEasing Easing)
{
	++FindOrAddSlot(Slot).RecentreCount;
	StartTransition(Slot, 1.0f, Duration, Easing);
}

void UPuppetPerformanceComponent::TransitionToAnimation(FName Slot, float Duration, EPuppetEasing Easing)
{
	StartTransition(Slot, 0.0f, Duration, Easing);
}

void UPuppetPerformanceComponent::SetControlBlend(FName Slot, float Blend)
{
	FSlotState& State = FindOrAddSlot(Slot);
	State.Blend = FMath::Clamp(Blend, 0.0f, 1.0f);
	State.bActive = false;
}

float UPuppetPerformanceComponent::GetControlBlend(FName Slot) const
{
	if (const FSlotState* State = Slots.Find(Slot))
	{
		return State->Blend;
	}
	return SlotsStartingOnAnimation.Contains(Slot) ? 0.0f : 1.0f;
}

bool UPuppetPerformanceComponent::IsTransitioning(FName Slot) const
{
	const FSlotState* State = Slots.Find(Slot);
	return State && State->bActive;
}

void UPuppetPerformanceComponent::Recentre(FName Slot)
{
	++FindOrAddSlot(Slot).RecentreCount;
}

void UPuppetPerformanceComponent::AdvanceTransitions(float DeltaTime)
{
	// Broadcast after the loop: a listener may start another transition, which changes Slots.
	TArray<TPair<FName, bool>, TInlineAllocator<4>> Finished;
	for (TPair<FName, FSlotState>& Pair : Slots)
	{
		FSlotState& State = Pair.Value;
		if (!State.bActive)
		{
			continue;
		}
		State.Elapsed += DeltaTime;
		const float Alpha = State.Elapsed / State.Duration;
		State.Blend = FMath::Lerp(State.From, State.To, PuppetEasing::Apply(State.Easing, Alpha));
		if (Alpha >= 1.0f)
		{
			State.Blend = State.To;
			State.bActive = false;
			Finished.Emplace(Pair.Key, State.To > 0.5f);
		}
	}
	for (const TPair<FName, bool>& Entry : Finished)
	{
		OnTransitionFinished.Broadcast(Entry.Key, Entry.Value);
	}
}

int32 UPuppetPerformanceComponent::GetRecentreCount(FName Slot) const
{
	const FSlotState* State = Slots.Find(Slot);
	return State ? State->RecentreCount : 0;
}

void UPuppetPerformanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceTransitions(DeltaTime);
}

UPuppetPerformanceComponent::FSlotState& UPuppetPerformanceComponent::FindOrAddSlot(FName Slot)
{
	if (FSlotState* State = Slots.Find(Slot))
	{
		return *State;
	}
	const float Blend = GetControlBlend(Slot);
	FSlotState& State = Slots.Add(Slot);
	State.Blend = Blend;
	return State;
}

void UPuppetPerformanceComponent::StartTransition(FName Slot, float To, float Duration, EPuppetEasing Easing)
{
	FSlotState& State = FindOrAddSlot(Slot);
	if (Duration <= 0.0f)
	{
		State.Blend = To;
		State.bActive = false;
		OnTransitionFinished.Broadcast(Slot, To > 0.5f);
		return;
	}
	State.From = State.Blend;
	State.To = To;
	State.Duration = Duration;
	State.Elapsed = 0.0f;
	State.Easing = Easing;
	State.bActive = true;
}
