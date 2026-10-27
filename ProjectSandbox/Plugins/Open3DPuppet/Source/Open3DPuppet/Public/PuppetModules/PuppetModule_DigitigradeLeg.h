// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_DigitigradeLeg.generated.h"

/**
 * Drives a three-segment leg (thigh, shin, metatarsal, then the paw), as a wolf's hind and front
 * legs have, from a performer's two-segment leg or arm. The paw follows the performer's ankle (or
 * wrist), relative to the hip and scaled by leg length. The metatarsal keeps its reference angle,
 * flexed with the performer's knee and lifted by Heel Lift, and two-bone IK places the thigh and
 * shin to meet it at the hock. On the ground the paw stays level.
 */
UCLASS(meta = (DisplayName = "Digitigrade Leg"))
class OPEN3DPUPPET_API UPuppetModule_DigitigradeLeg : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Performer upper joint, by canonical name (thigh_l, or upperarm_l for a front leg). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerUpper = TEXT("thigh_l");

	/** Performer lower joint (calf_l, or lowerarm_l). Its bend is the knee bend Knee To Hock Ratio reads. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerLower = TEXT("calf_l");

	/** Performer end joint (foot_l, or hand_l): the paw follows it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerEnd = TEXT("foot_l");

	/** Puppet thigh (or upper arm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetUpper = TEXT("thigh_l");

	/** Puppet shin, from knee to hock; a descendant of PuppetUpper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetLower = TEXT("calf_l");

	/** Puppet metatarsal, from hock to paw; a descendant of PuppetLower. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetMetatarsal = TEXT("foot_l");

	/** Puppet paw; a descendant of PuppetMetatarsal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetPaw = TEXT("ball_l");

	/** Scales how far the paw moves forward, back and sideways for the performer's steps; up and down are not scaled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float StrideScale = 1.0f;

	/** How much of the performer's knee bend the hock takes as well: 0 keeps the metatarsal at its reference angle, 1 flexes it by the whole knee bend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "1.5"))
	float KneeToHockRatio = 0.5f;

	/** Raises the hock by turning the metatarsal towards upright, in degrees, with the paw where it is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (UIMin = "-45.0", UIMax = "45.0", Units = "deg"))
	float HeelLift = 0.0f;

	/** While the paw is within this height of its reference height, it stays level on the ground; above it, it turns with the performer's foot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", Units = "cm"))
	float ContactHeight = 2.0f;

	/**
	 * Which way the knee points in the puppet's reference pose, in component space. Only needed
	 * when the hip, knee and hock are in a straight line in the reference pose.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FVector PoleDirection = FVector::ZeroVector;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
