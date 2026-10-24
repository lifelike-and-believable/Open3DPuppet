// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// FPuppetPerformerPose: the performer skeleton arrives from Live Link, a network or a file, so it
// is validated before use (ADR 0003, SEC-002).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PuppetPerformerPose.h"
#include "PuppetPerformerProfile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformerInvalidParentTest, "Open3DPuppet.Performer.Pose.RejectsInvalidParents", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformerInvalidParentTest::RunTest(const FString& Parameters)
{
	FPuppetPerformerPose Pose;
	FString Error;
	const TArray<FName> Names = { TEXT("a"), TEXT("b") };
	TestFalse(TEXT("Parent out of range"), Pose.SetSkeleton(Names, TArray<int32>({ INDEX_NONE, 7 }), nullptr, Error));
	TestFalse(TEXT("Negative parent other than INDEX_NONE"), Pose.SetSkeleton(Names, TArray<int32>({ INDEX_NONE, -3 }), nullptr, Error));
	TestFalse(TEXT("Own parent"), Pose.SetSkeleton(Names, TArray<int32>({ INDEX_NONE, 1 }), nullptr, Error));
	TestFalse(TEXT("A cycle"), Pose.SetSkeleton(Names, TArray<int32>({ 1, 0 }), nullptr, Error));
	TestFalse(TEXT("Counts differ"), Pose.SetSkeleton(Names, TArray<int32>({ INDEX_NONE }), nullptr, Error));
	TestFalse(TEXT("No bones"), Pose.SetSkeleton(TArray<FName>(), TArray<int32>(), nullptr, Error));
	TestFalse(TEXT("The pose is empty after a failure"), Pose.HasSkeleton());
	TestFalse(TEXT("The reason is given"), Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformerOrderTest, "Open3DPuppet.Performer.Pose.ChildBeforeParent", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformerOrderTest::RunTest(const FString& Parameters)
{
	// Live Link does not require parents to come first.
	FPuppetPerformerPose Pose;
	FString Error;
	TestTrue(TEXT("Valid"), Pose.SetSkeleton(TArray<FName>({ TEXT("child"), TEXT("root") }), TArray<int32>({ 1, INDEX_NONE }), nullptr, Error));
	TestTrue(TEXT("Transforms accepted"), Pose.SetLocalTransforms(TArray<FTransform>({ FTransform(FVector(0, 0, 5)), FTransform(FVector(0, 0, 10)) })));
	TestEqual(TEXT("Child in component space"), Pose.GetComponent(0).GetTranslation(), FVector(0, 0, 15));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformerTransformCountTest, "Open3DPuppet.Performer.Pose.RejectsTransformCountMismatch", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformerTransformCountTest::RunTest(const FString& Parameters)
{
	FPuppetPerformerPose Pose;
	FString Error;
	Pose.SetSkeleton(TArray<FName>({ TEXT("root"), TEXT("child") }), TArray<int32>({ INDEX_NONE, 0 }), nullptr, Error);
	TestFalse(TEXT("One transform for two bones"), Pose.SetLocalTransforms(TArray<FTransform>({ FTransform::Identity })));
	TestFalse(TEXT("No pose yet"), Pose.HasTransforms());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformerProfileTest, "Open3DPuppet.Performer.Pose.ProfileMapsCanonicalNames", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformerProfileTest::RunTest(const FString& Parameters)
{
	UPuppetPerformerProfile* Profile = NewObject<UPuppetPerformerProfile>();
	Profile->CanonicalToSource.Add(TEXT("pelvis"), TEXT("Hips"));
	Profile->CanonicalToSource.Add(TEXT("head"), TEXT("NotInTheSkeleton"));

	FPuppetPerformerPose Pose;
	FString Error;
	Pose.SetSkeleton(TArray<FName>({ TEXT("Hips"), TEXT("spine_01") }), TArray<int32>({ INDEX_NONE, 0 }), Profile, Error);
	TestEqual(TEXT("Mapped through the profile"), Pose.FindJoint(TEXT("pelvis")), 0);
	TestEqual(TEXT("Unmapped names are looked up as they are"), Pose.FindJoint(TEXT("spine_01")), 1);
	TestEqual(TEXT("A mapping to a missing bone finds nothing"), Pose.FindJoint(TEXT("head")), INDEX_NONE);
	return true;
}

#endif
