// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "Containers/UnrealString.h"
#include "PuppetNaming.h"

class UPuppetAsset;

namespace PuppetSetup
{
	/**
	 * The setup assistant (WP-8): fills a puppet asset's empty module list for its Target Mesh. The
	 * mesh's bones are named through the naming scheme that fits them best (PuppetNaming::Match),
	 * and the asset gets:
	 *
	 * - Root, with Pelvis Bone set;
	 * - a Chain for the spine (spine_01 up to the bone the neck hangs from) and one for the neck and
	 *   head;
	 * - a Two-Bone Limb for each arm, and for each leg with Pelvis Relative effectors;
	 * - Digits for each hand with fingers, and Eyes when the mesh has both eyes;
	 * - a Dynamic Chain for each leftover chain of two or more bones (a tail, ears), leaving out
	 *   helper bones (IK targets, twist and corrective bones).
	 *
	 * Performer joints are the canonical ones; puppet bones are the mesh's. False, with the asset
	 * unchanged, when it has no Target Mesh, already has modules, or no scheme names the mesh's
	 * bones. OutReport says what was proposed and what to check. Game thread; allocates.
	 */
	OPEN3DPUPPET_API bool ProposeModules(UPuppetAsset& Asset, TConstArrayView<FPuppetNamingScheme> Schemes, FString& OutReport);
}
