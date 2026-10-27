// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "ControlRig.h"

#include "PuppetTestControlRig.generated.h"

/**
 * A native Control Rig for the Control Rig adapter's tests (ADR 0010), runnable without a rig
 * asset, as the engine's FK Control Rig is. Its bones are the Face fixture's chain from root to
 * head, and the jaw. Controls:
 * - head_ctrl (transform): placed on the head by construction. The forward solve turns and moves
 *   the head, as it comes in, by the control's change from its initial transform.
 * - jaw_open (float): the forward solve sets the jaw to its initial local transform turned by
 *   jaw_open times JawGain (a public variable, 1 by default) times 30 degrees about Z, and the
 *   curve jaw_curve to jaw_open.
 * - pre_open (float): when not 0, Pre Forwards Solve sets jaw_open to it.
 * - post_turn (float): Post Forwards Solve turns the head, and the jaw with it, by post_turn times
 *   30 degrees about Z.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UPuppetTestControlRig : public UControlRig
{
	GENERATED_BODY()

public:
	UPuppetTestControlRig();

	virtual void Initialize(bool bInitRigUnits = true) override;
	virtual void InitializeVMs(bool bRequestInit = true) override { URigVMHost::Initialize(bRequestInit); }
	virtual bool InitializeVMs(const FName& InEventName) override
	{
		// URigVMHost::InitializeVM fails when the rig offers more variables than its VM has, and a
		// native rig's VM has none, so JawGain is hidden while it runs.
		TGuardValue<bool> Guard(bInitializingVM, true);
		return URigVMHost::InitializeVM(InEventName);
	}
	virtual void InitializeVMsFromCDO() override { URigVMHost::InitializeFromCDO(); }
	virtual void RequestInitVMs() override { URigVMHost::RequestInit(); }
	virtual bool Execute_Internal(const FName& InEventName) override;
	virtual bool SupportsEvent(const FName& InEventName) const override;

	/** A public rig variable: the jaw turns jaw_open times JawGain times 30 degrees. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	float JawGain = 1.0f;

protected:
	/** A native rig's own properties are not rig variables, so JawGain is offered as one here. */
	virtual TArray<FRigVMExternalVariable> GetExternalVariablesImpl(bool bFallbackToBlueprint) const override;

private:
	bool bInitializingVM = false;
};
