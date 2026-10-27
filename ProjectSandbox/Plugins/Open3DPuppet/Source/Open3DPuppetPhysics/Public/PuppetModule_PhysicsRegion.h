// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetDynamics.h"
#include "PuppetModule.h"

#include "PuppetModule_PhysicsRegion.generated.h"

/**
 * How physical one region of the puppet is under engine physics (ADR 0007, Tier 2). Writes no
 * bones: it publishes two puppet curves, PuppetPhysics::WeightCurve and DampingCurve, that
 * UPuppetPhysicsLibrary::MakeRegionParameters turns into the Rigid Body With Control node's
 * parameters, so the AnimGraph reads Puppet Solve, then Rigid Body With Control. Both curves are
 * 0 when the region is as its physics controls were authored, so a module at weight 0, a
 * performer at neutral or a blend that drops the curves leaves the physics as authored.
 */
UCLASS(meta = (DisplayName = "Physics Region"))
class OPEN3DPUPPETPHYSICS_API UPuppetModule_PhysicsRegion : public UPuppetModule
{
	GENERATED_BODY()

public:
	UPuppetModule_PhysicsRegion();

	/** The region: a limb name in the Rigid Body With Control node's Character Setup Data (Tail, LeftFrontLeg, Body). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	FName Region = TEXT("Tail");

	/** How physical the region is: 0 keeps its controls' authored strength, 1 takes it to 0 (limp). A performer curve can play it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	FPuppetPhysicsWeight PhysicsWeight;

	/** Extra damping for the region: 0 keeps its controls' authored damping, 1 doubles it. A performer curve can play it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	FPuppetPhysicsWeight Damping;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};

namespace PuppetPhysics
{
	/** The puppet curve that carries a region's physics weight: Physics_<Region>. */
	OPEN3DPUPPETPHYSICS_API FName WeightCurve(FName Region);

	/** The puppet curve that carries a region's extra damping: Physics_<Region>_Damping. */
	OPEN3DPUPPETPHYSICS_API FName DampingCurve(FName Region);
}
