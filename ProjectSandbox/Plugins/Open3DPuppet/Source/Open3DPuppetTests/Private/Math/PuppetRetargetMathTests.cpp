// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// PuppetRetarget: the operations every module builds on (design overview §4).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetRetargetMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRotationDeltaRestTest, "Open3DPuppet.Math.RotationDelta.IdentityAtRest", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRotationDeltaRestTest::RunTest(const FString& Parameters)
{
	const FQuat Rest = PuppetTests::Rotation(FVector(1, 2, 3), 40.0);
	PuppetTests::TestRotation(*this, TEXT("No motion since calibration is no rotation"), PuppetRetarget::RotationDelta(Rest, Rest, FQuat::Identity), FQuat::Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRotationDeltaRestIndependentTest, "Open3DPuppet.Math.RotationDelta.IndependentOfRestOrientation", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRotationDeltaRestIndependentTest::RunTest(const FString& Parameters)
{
	// The same 30 degree turn about X, from two different rest orientations (two capture systems'
	// bone axes), gives the same delta.
	const FQuat Turn = PuppetTests::Rotation(FVector::XAxisVector, 30.0);
	const FQuat RestA = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);
	const FQuat RestB = PuppetTests::Rotation(FVector(1, 1, 0), 120.0);
	PuppetTests::TestRotation(*this, TEXT("Rest A"), PuppetRetarget::RotationDelta(Turn * RestA, RestA, FQuat::Identity), Turn);
	PuppetTests::TestRotation(*this, TEXT("Rest B"), PuppetRetarget::RotationDelta(Turn * RestB, RestB, FQuat::Identity), Turn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRotationDeltaAlignmentTest, "Open3DPuppet.Math.RotationDelta.AlignmentRotatesTheAxis", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRotationDeltaAlignmentTest::RunTest(const FString& Parameters)
{
	// A performer facing 90 degrees away from the puppet: a turn about the performer's X is a turn
	// about the puppet's Y.
	const FQuat Alignment = FRotator(0.0, 90.0, 0.0).Quaternion();
	const FQuat Current = PuppetTests::Rotation(FVector::XAxisVector, 30.0);
	PuppetTests::TestRotation(*this, TEXT("Turn about Y"), PuppetRetarget::RotationDelta(Current, FQuat::Identity, Alignment),
		PuppetTests::Rotation(FVector::YAxisVector, 30.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetScaleRotationTest, "Open3DPuppet.Math.ScaleRotation.ScalesTheAngle", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetScaleRotationTest::RunTest(const FString& Parameters)
{
	const FQuat Turn = PuppetTests::Rotation(FVector::ZAxisVector, 30.0);
	PuppetTests::TestRotation(*this, TEXT("Twice"), PuppetRetarget::ScaleRotation(Turn, 2.0), PuppetTests::Rotation(FVector::ZAxisVector, 60.0));
	PuppetTests::TestRotation(*this, TEXT("Half"), PuppetRetarget::ScaleRotation(Turn, 0.5), PuppetTests::Rotation(FVector::ZAxisVector, 15.0));
	PuppetTests::TestRotation(*this, TEXT("None"), PuppetRetarget::ScaleRotation(Turn, 0.0), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("Negated quaternion, same rotation"), PuppetRetarget::ScaleRotation(-Turn, 2.0), PuppetTests::Rotation(FVector::ZAxisVector, 60.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainPositionsTest, "Open3DPuppet.Math.ChainPositions.ByDistanceAlongTheChain", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainPositionsTest::RunTest(const FString& Parameters)
{
	TArray<float> Positions;
	PuppetRetarget::ChainPositions(TArray<FVector>({ FVector(0, 0, 0), FVector(0, 0, 10), FVector(0, 0, 30) }), Positions);
	TestEqual(TEXT("Three joints"), Positions.Num(), 3);
	TestNearlyEqual(TEXT("First"), Positions[0], 0.0f);
	TestNearlyEqual(TEXT("Second, a third of the way"), Positions[1], 1.0f / 3.0f);
	TestNearlyEqual(TEXT("Last"), Positions[2], 1.0f);

	PuppetRetarget::ChainPositions(TArray<FVector>({ FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector }), Positions);
	TestNearlyEqual(TEXT("Zero length: evenly spaced"), Positions[1], 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSampleChainTest, "Open3DPuppet.Math.SampleChain.InterpolatesAndClamps", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSampleChainTest::RunTest(const FString& Parameters)
{
	const TArray<FQuat> Rotations = { FQuat::Identity, PuppetTests::Rotation(FVector::ZAxisVector, 90.0) };
	const TArray<float> Positions = { 0.0f, 1.0f };
	PuppetTests::TestRotation(*this, TEXT("Halfway"), PuppetRetarget::SampleChain(Rotations, Positions, 0.5f), PuppetTests::Rotation(FVector::ZAxisVector, 45.0));
	PuppetTests::TestRotation(*this, TEXT("Before the start"), PuppetRetarget::SampleChain(Rotations, Positions, -1.0f), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("After the end"), PuppetRetarget::SampleChain(Rotations, Positions, 2.0f), Rotations[1]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCollectChainTest, "Open3DPuppet.Math.CollectChain.StartToEnd", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCollectChainTest::RunTest(const FString& Parameters)
{
	// 0 <- 1 <- 2 <- 3, and 4 is a second child of 1.
	const TArray<int32> Parents = { INDEX_NONE, 0, 1, 2, 1 };
	TArray<int32> Chain;
	TestTrue(TEXT("1 to 3 is a chain"), PuppetRetarget::CollectChain(Parents, 1, 3, Chain));
	TestTrue(TEXT("Start first"), Chain == TArray<int32>({ 1, 2, 3 }));
	TestTrue(TEXT("A single bone is a chain"), PuppetRetarget::CollectChain(Parents, 2, 2, Chain));
	TestFalse(TEXT("4 does not descend from 2"), PuppetRetarget::CollectChain(Parents, 2, 4, Chain));
	TestTrue(TEXT("Empty on failure"), Chain.IsEmpty());
	TestFalse(TEXT("An index out of range"), PuppetRetarget::CollectChain(Parents, 0, 9, Chain));
	return true;
}

#endif
