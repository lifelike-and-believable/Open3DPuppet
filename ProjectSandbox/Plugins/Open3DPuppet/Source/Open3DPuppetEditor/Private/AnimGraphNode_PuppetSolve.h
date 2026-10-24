// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "AnimGraphNode_Base.h"
#include "AnimNode_PuppetSolve.h"

#include "AnimGraphNode_PuppetSolve.generated.h"

namespace ENodeTitleType { enum Type : int; }

/** The editor side of the Puppet Solve node (ADR 0002). */
UCLASS()
class UAnimGraphNode_PuppetSolve : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_PuppetSolve Node;

	//~ UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	//~ End of UEdGraphNode interface

	//~ UK2Node interface
	virtual FText GetMenuCategory() const override;
	//~ End of UK2Node interface
};
