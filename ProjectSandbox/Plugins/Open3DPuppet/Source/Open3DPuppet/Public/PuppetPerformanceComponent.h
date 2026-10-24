// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "PuppetEasing.h"

#include "PuppetPerformanceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPuppetTransitionFinished, FName, Slot, bool, bToPerformer);

/**
 * Hands a puppet's performer slots between an animation and the performer (ADR 0008). Put it on
 * the puppet's actor; the Puppet Solve node reads it every frame. Triggers (a Sequencer event, an
 * operator's input, a performer gesture) call it. A slot it has never been told about is under
 * full performer control, unless it is in SlotsStartingOnAnimation.
 */
UCLASS(ClassGroup = (Animation), meta = (BlueprintSpawnableComponent, DisplayName = "Puppet Performance"))
class OPEN3DPUPPET_API UPuppetPerformanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPuppetPerformanceComponent();

	/** Slots under the animation (control blend 0) until a transition or SetControlBlend, for example those that enter on an animation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet|Performance")
	TArray<FName> SlotsStartingOnAnimation;

	/** Hands the slot to the performer over Duration seconds. The puppet re-centres where the animation left it, then converges on the performer. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Performance")
	void TransitionToPerformer(FName Slot, float Duration = 1.0f, EPuppetEasing Easing = EPuppetEasing::EaseInOut);

	/** Hands the slot back to the animation (the Puppet Solve node's input pose) over Duration seconds. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Performance")
	void TransitionToAnimation(FName Slot, float Duration = 1.0f, EPuppetEasing Easing = EPuppetEasing::EaseInOut);

	/** Sets the slot's control blend at once (0 animation, 1 performer) and stops any transition. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Performance")
	void SetControlBlend(FName Slot, float Blend);

	/** The slot's control blend: 0 animation, 1 performer. */
	UFUNCTION(BlueprintPure, Category = "Puppet|Performance")
	float GetControlBlend(FName Slot) const;

	/** True while the slot is transitioning. */
	UFUNCTION(BlueprintPure, Category = "Puppet|Performance")
	bool IsTransitioning(FName Slot) const;

	/** Re-centres the slot's performer on where the puppet is now, then converges on the performer as after a hand-off. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Performance")
	void Recentre(FName Slot);

	/** Broadcast when a transition reaches its end. */
	UPROPERTY(BlueprintAssignable, Category = "Puppet|Performance")
	FPuppetTransitionFinished OnTransitionFinished;

	/** Advances every transition by DeltaTime seconds. TickComponent calls it. */
	void AdvanceTransitions(float DeltaTime);

	/** Counts the slot's re-centre requests; the Puppet Solve node re-centres when it changes. */
	int32 GetRecentreCount(FName Slot) const;

	//~ UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End of UActorComponent interface

private:
	struct FSlotState
	{
		float Blend = 1.0f;
		float From = 1.0f;
		float To = 1.0f;
		float Duration = 0.0f;
		float Elapsed = 0.0f;
		EPuppetEasing Easing = EPuppetEasing::Linear;
		bool bActive = false;
		int32 RecentreCount = 0;
	};

	/** The slot's state, created at the blend it has before anyone touched it. */
	FSlotState& FindOrAddSlot(FName Slot);
	void StartTransition(FName Slot, float To, float Duration, EPuppetEasing Easing);

	TMap<FName, FSlotState> Slots;
};
