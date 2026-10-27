// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_Digits.generated.h"

/** One performer finger driving one puppet digit (a finger, toe, talon or feather). */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetDigit
{
	GENERATED_BODY()

	/** First joint of the performer finger, by canonical name (for example index_01_l). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerStart;

	/** Last joint of the performer finger (for example index_03_l); a descendant of PerformerStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerEnd;

	/** First bone of the puppet digit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetStart;

	/** Last bone of the puppet digit; a descendant of PuppetStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetEnd;

	/** Scales the finger's curl (its bend about any axis but the spread axis): 0 keeps the digit straight, above 1 exaggerates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float CurlGain = 1.0f;

	/** Scales the finger's spread (its turn about the module's Spread Axis): 0 keeps the digit's rest spread. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float SpreadGain = 1.0f;
};

/**
 * Drives puppet digits (fingers, toes, talons, feathers) from the performer's fingers, relative to
 * the hand (ADR 0006): a digit curls and spreads the same way whatever the hand or the paw is doing.
 * A finger and a digit need not have the same number of bones; the finger's total bend is spread
 * along the digit by relative position (design overview §4). One finger may drive several digits.
 */
UCLASS(meta = (DisplayName = "Digits"))
class OPEN3DPUPPET_API UPuppetModule_Digits : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Fills Digits with the five fingers of a mannequin's left hand, mapped onto the same names. */
	UPuppetModule_Digits();

	/** The finger-to-digit mappings. Each digit's bones are written by this module; other bones follow their parents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Digits", meta = (TitleProperty = "PuppetStart"))
	TArray<FPuppetDigit> Digits;

	/**
	 * The axis fingers spread about, in the puppet's component space at rest (the palm's normal; up
	 * for a hand that is flat in a T-pose). Rotation about it is spread, the rest is curl.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	FVector SpreadAxis = FVector::UpVector;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
