// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "AnimGraphNode_PuppetSolve.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimGraphNode_PuppetSolve)

#define LOCTEXT_NAMESPACE "Open3DPuppet"

FText UAnimGraphNode_PuppetSolve::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PuppetSolveTitle", "Puppet Solve");
}

FText UAnimGraphNode_PuppetSolve::GetTooltipText() const
{
	return LOCTEXT("PuppetSolveTooltip", "Drives the mesh from Live Link performers through a puppet's modules. Bones no module writes pass through from Source.");
}

FText UAnimGraphNode_PuppetSolve::GetMenuCategory() const
{
	return LOCTEXT("PuppetSolveCategory", "Puppet");
}

#undef LOCTEXT_NAMESPACE
