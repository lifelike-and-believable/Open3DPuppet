// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"

#include "PuppetAsset.generated.h"

class UAnimSequence;
class UPuppetModule;
class USkeletalMesh;

/**
 * A puppet: the mesh it was set up for, its performer slots and the modules that drive it, in the
 * order they run (ADR 0002). Used by the Puppet Solve AnimGraph node.
 */
UCLASS(BlueprintType)
class OPEN3DPUPPET_API UPuppetAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The skeletal mesh this puppet was set up for. The node solves whatever mesh it runs on; this is for the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	TSoftObjectPtr<USkeletalMesh> TargetMesh;

	/** The roles performers play on this puppet, for example Body, or Front and Back. Each module reads one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	TArray<FName> PerformerSlots = { TEXT("Body") };

	/** The slot whose performer owns the puppet's position in the world (ADR 0004). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName RootSlot = TEXT("Body");

	/** The puppet's pelvis bone; its height over the performer's pelvis height sets the root scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PelvisBone = TEXT("pelvis");

	/**
	 * The pose modules retarget from: how the puppet stands when the performer stands in the pose
	 * they calibrate in (T-pose by default). Leave empty to use the mesh's reference pose; set it
	 * when the mesh is modelled in another pose, such as an A-pose mesh for T-pose performers. An
	 * AnimSequence on the puppet's skeleton; its bone rotations at Rest Pose Time are used, bone
	 * lengths stay the mesh's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	TObjectPtr<UAnimSequence> RestPose;

	/** The time in Rest Pose to take the pose from, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (EditCondition = "RestPose != nullptr", ClampMin = "0"))
	float RestPoseTime = 0.0f;

	/** A move of the puppet further than this in one frame, in centimetres, counts as a teleport: dynamic modules start again from the pose (ADR 0007). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0", Units = "cm"))
	float TeleportDistance = 200.0f;

		/** The modules, in the order they run: root first, then the bones each later module hangs from. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Modules")
	TArray<TObjectPtr<UPuppetModule>> Modules;

	/** Changes whenever the asset or one of its modules is edited. Game thread only. */
	int32 GetRevision() const { return Revision; }

	/** Records an edit, so Puppet Solve nodes using this asset bind again. Game thread only. */
	void MarkChanged() { ++Revision; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	int32 Revision = 0;
};
