// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetDynamics.h"
#include "PuppetModule.h"

#include "PuppetModule_DynamicChain.generated.h"

/** A sphere on a puppet bone that a dynamic chain keeps out of. */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetCollisionBone
{
	GENERATED_BODY()

	/** The bone the sphere is centred on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName Bone;

	/** The sphere's radius, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (ClampMin = "0.0", Units = "cm"))
	float Radius = 10.0f;
};

/**
 * A chain of puppet bones that swings and settles with the puppet's motion (ADR 0007, Tier 1): a
 * tail, ears, a mane, feathers. It follows the pose it receives, optionally mixed with a performer
 * chain (a hand gesture or a second performer taking over), and simulates on top of it: springs
 * towards that pose, damping, gravity, and collision with spheres on bones and the ground.
 */
UCLASS(meta = (DisplayName = "Dynamic Chain (Tail)"))
class OPEN3DPUPPET_API UPuppetModule_DynamicChain : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** First bone of the chain; it stays where the pose puts it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetStart = TEXT("tail_01");

	/** Last bone of the chain; a descendant of PuppetStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetEnd = TEXT("tail_05");

	/** Optional first performer joint of a chain that also drives the bones (Input Mix). None follows the received pose only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerStart;

	/** Last performer joint of that chain; a descendant of PerformerStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerEnd;

	/** How much of the performer chain's rotation the followed pose takes: 0 the received pose, 1 the performer chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InputMix = 0.0f;

	/** How hard each simulation step pulls the chain back to the followed pose: 0 swings freely, 1 holds the pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Stiffness = 0.1f;

	/** How fast the swing loses its speed, per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0", UIMax = "20.0"))
	float Damping = 2.0f;

	/** Acceleration on the chain, in the puppet's component space, in cm/s². */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics")
	FVector Gravity = FVector(0.0, 0.0, -980.0);

	/** Scales the swing away from the followed pose: below 1 calms it, above 1 exaggerates it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float Amplitude = 1.0f;

	/** How much the chain simulates: 0 follows the pose, 1 is fully simulated; a performer curve can play it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics")
	FPuppetPhysicsWeight PhysicsWeight;

	/** Spheres on puppet bones the chain keeps out of, for example the hind legs for a tail. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (TitleProperty = "Bone"))
	TArray<FPuppetCollisionBone> CollisionBones;

	/** Keep the chain above the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
	bool bCollideWithGround = false;

	/** The ground's height in the puppet's component space, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (EditCondition = "bCollideWithGround", Units = "cm"))
	float GroundHeight = 0.0f;

	/** The chain's thickness for collision, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (ClampMin = "0.0", Units = "cm"))
	float Radius = 2.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
