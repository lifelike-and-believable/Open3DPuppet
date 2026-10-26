// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Performer clips as a cast source (ADR 0008, item 1): playback time, sampling an AnimSequence on
// its own skeleton as a Live Link subject's data, calibration from a frame or the reference pose,
// and a puppet re-solving a recorded take.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetClipSampler.h"
#include "PuppetModules/PuppetModule_Root.h"
#include "PuppetPerformerCalibration.h"

namespace PuppetClipTests
{
	constexpr double KeyTime = 1.0 / 30.0;

	/** A biped take of two keys: the rest pose, then the pelvis at Pelvis (component space; the rest is at 0, 0, 100). */
	TArray<TArray<FTransform>> Take(const FVector& Pelvis)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Biped());
		TArray<TArray<FTransform>> Keys;
		Keys.Add(Performer.Local);
		Performer.SetLocalTranslation(TEXT("pelvis"), Pelvis);
		Keys.Add(Performer.Local);
		return Keys;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetClipTimeTest, "Open3DPuppet.Clips.PlaybackTime.LoopsClampsAndReverses", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetClipTimeTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual(TEXT("Plays at the rate"), FPuppetClipSampler::AdvanceTime(1.0, 0.5, 2.0f, true, 4.0), 2.0);
	TestNearlyEqual(TEXT("A loop wraps at the end"), FPuppetClipSampler::AdvanceTime(3.5, 1.0, 1.0f, true, 4.0), 0.5);
	TestNearlyEqual(TEXT("A loop wraps backwards"), FPuppetClipSampler::AdvanceTime(0.5, 1.0, -1.0f, true, 4.0), 3.5);
	TestNearlyEqual(TEXT("A loop wraps a long step"), FPuppetClipSampler::AdvanceTime(1.0, 10.0, 1.0f, true, 4.0), 3.0);
	TestNearlyEqual(TEXT("Without a loop it holds at the end"), FPuppetClipSampler::AdvanceTime(3.5, 1.0, 1.0f, false, 4.0), 4.0);
	TestNearlyEqual(TEXT("Without a loop it holds at the start"), FPuppetClipSampler::AdvanceTime(0.5, 1.0, -1.0f, false, 4.0), 0.0);
	TestNearlyEqual(TEXT("A clip of length 0 stays at 0"), FPuppetClipSampler::AdvanceTime(0.0, 1.0, 1.0f, true, 0.0), 0.0);
	TestNearlyEqual(TEXT("Placing wraps a loop"), FPuppetClipSampler::PlaceTime(5.0, true, 4.0), 1.0);
	TestNearlyEqual(TEXT("Placing clamps otherwise"), FPuppetClipSampler::PlaceTime(5.0, false, 4.0), 4.0);
	TestNearlyEqual(TEXT("Placing clamps below"), FPuppetClipSampler::PlaceTime(-1.0, false, 4.0), 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetClipSkeletonTest, "Open3DPuppet.Clips.Sampler.GivesTheSkeletonAndKeys", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetClipSkeletonTest::RunTest(const FString& Parameters)
{
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, PuppetClipTests::Take(FVector(10, 0, 100)));

	FPuppetClipSampler Sampler;
	FString Error;
	if (!TestTrue(TEXT("Binds"), Sampler.SetSequence(Clip.Get(), Error)))
	{
		AddError(Error);
		return false;
	}
	const PuppetTests::FPerformer Expected(PuppetTests::Biped());
	TestEqual(TEXT("Bone names"), TArray<FName>(Sampler.GetBoneNames()), Expected.Names);
	TestEqual(TEXT("Parents"), TArray<int32>(Sampler.GetParents()), Expected.Parents);
	TestNearlyEqual(TEXT("Length"), Sampler.GetPlayLength(), PuppetClipTests::KeyTime, 1.e-6);

	const int32 Pelvis = Expected.Names.IndexOfByKey(FName(TEXT("pelvis")));
	const int32 Thigh = Expected.Names.IndexOfByKey(FName(TEXT("thigh_l")));
	TArray<FTransform> Local;
	TestTrue(TEXT("Samples"), Sampler.Sample(0.0, Local));
	if (!TestEqual(TEXT("One transform per bone"), Local.Num(), Expected.Names.Num()))
	{
		return false;
	}
	TestEqual(TEXT("First key"), Local[Pelvis].GetTranslation(), FVector(0, 0, 100), 1.e-2f);
	Sampler.Sample(PuppetClipTests::KeyTime, Local);
	TestEqual(TEXT("Second key"), Local[Pelvis].GetTranslation(), FVector(10, 0, 100), 1.e-2f);
	Sampler.Sample(PuppetClipTests::KeyTime * 0.5, Local);
	TestEqual(TEXT("Between keys"), Local[Pelvis].GetTranslation(), FVector(5, 0, 100), 1.e-2f);
	TestEqual(TEXT("An unmoved bone"), Local[Thigh].GetTranslation(), FVector(0, 10, 0), 1.e-2f);
	Sampler.Sample(10.0, Local);
	TestEqual(TEXT("Past the end holds the last key"), Local[Pelvis].GetTranslation(), FVector(10, 0, 100), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetClipRetargetModeTest, "Open3DPuppet.Clips.Sampler.KeepsTheTakesTranslationsWhateverTheRetargetMode", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetClipRetargetModeTest::RunTest(const FString& Parameters)
{
	// Mocap skeletons often set bones to Skeleton translation retargeting, which takes a bone's
	// translation from the reference pose; a performer clip must still give the take's own.
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	const int32 Pelvis = Skeleton->GetReferenceSkeleton().FindBoneIndex(TEXT("pelvis"));
	Skeleton->SetBoneTranslationRetargetingMode(Pelvis, EBoneTranslationRetargetingMode::Skeleton);
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, PuppetClipTests::Take(FVector(10, 0, 100)));

	FPuppetClipSampler Sampler;
	FString Error;
	TArray<FTransform> Local;
	if (!TestTrue(TEXT("Binds"), Sampler.SetSequence(Clip.Get(), Error)) || !TestTrue(TEXT("Samples"), Sampler.Sample(PuppetClipTests::KeyTime, Local)))
	{
		return false;
	}
	TestEqual(TEXT("The take's pelvis"), Local[Pelvis].GetTranslation(), FVector(10, 0, 100), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetClipRejectTest, "Open3DPuppet.Clips.Sampler.RejectsAdditiveAndUnbinds", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetClipRejectTest::RunTest(const FString& Parameters)
{
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, PuppetClipTests::Take(FVector(10, 0, 100)));

	FPuppetClipSampler Sampler;
	FString Error;
	TestTrue(TEXT("No sequence unbinds without an error"), Sampler.SetSequence(nullptr, Error) && Error.IsEmpty());
	TestFalse(TEXT("Unbound"), Sampler.IsBound());
	TArray<FTransform> Local;
	TestFalse(TEXT("Nothing to sample"), Sampler.Sample(0.0, Local));

	TestTrue(TEXT("Binds"), Sampler.SetSequence(Clip.Get(), Error));
	Clip->AdditiveAnimType = AAT_LocalSpaceBase;
	Clip->RefPoseType = ABPT_RefPose;
	TestTrue(TEXT("Additive"), Clip->IsValidAdditive());
	TestFalse(TEXT("An additive clip is rejected"), Sampler.SetSequence(Clip.Get(), Error));
	TestFalse(TEXT("With a reason"), Error.IsEmpty());
	TestFalse(TEXT("And the sampler is unbound"), Sampler.IsBound());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetClipCalibrationTest, "Open3DPuppet.Clips.Calibration.FromAFrameOrTheReferencePose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetClipCalibrationTest::RunTest(const FString& Parameters)
{
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	// The take starts away from the reference pose: the pelvis 20 cm forward, then 30.
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	TArray<TArray<FTransform>> Keys;
	Performer.SetLocalTranslation(TEXT("pelvis"), FVector(20, 0, 100));
	Keys.Add(Performer.Local);
	Performer.SetLocalTranslation(TEXT("pelvis"), FVector(30, 0, 100));
	Keys.Add(Performer.Local);
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, Keys);

	FPuppetClipSampler Sampler;
	FString Error;
	TestTrue(TEXT("Binds"), Sampler.SetSequence(Clip.Get(), Error));
	const int32 Pelvis = Performer.Names.IndexOfByKey(FName(TEXT("pelvis")));

	FPuppetPerformerCalibration Calibration;
	TestTrue(TEXT("From a frame"), Sampler.CaptureCalibration(false, PuppetClipTests::KeyTime, FRotator(0.0, 90.0, 0.0), Calibration));
	if (!TestEqual(TEXT("Bone names"), Calibration.BoneNames, Performer.Names))
	{
		return false;
	}
	TestEqual(TEXT("The frame's pelvis"), Calibration.RestComponent[Pelvis].GetTranslation(), FVector(30, 0, 100), 1.e-2f);
	TestEqual(TEXT("Alignment"), Calibration.Alignment, FRotator(0.0, 90.0, 0.0));

	if (!TestTrue(TEXT("From the reference pose"), Sampler.CaptureCalibration(true, 0.0, FRotator::ZeroRotator, Calibration)))
	{
		return false;
	}
	TestEqual(TEXT("The reference pelvis"), Calibration.RestComponent[Pelvis].GetTranslation(), FVector(0, 0, 100), 1.e-2f);

	FPuppetClipSampler Unbound;
	TestFalse(TEXT("Nothing to calibrate"), Unbound.CaptureCalibration(false, 0.0, FRotator::ZeroRotator, Calibration));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetClipTakeTest, "Open3DPuppet.Clips.Puppet.ReSolvesARecordedTake", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetClipTakeTest::RunTest(const FString& Parameters)
{
	// A recorded take drives the puppet as a live performer would: a puppet twice the performer's
	// size covers twice the distance the take's pelvis moves.
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, PuppetClipTests::Take(FVector(10, 0, 100)));
	FPuppetClipSampler Sampler;
	FString Error;
	if (!TestTrue(TEXT("Binds"), Sampler.SetSequence(Clip.Get(), Error)))
	{
		return false;
	}

	PuppetTests::FPuppet Puppet(PuppetTests::Scaled(PuppetTests::Biped(), 2.0));
	Puppet.Add<UPuppetModule_Root>();
	Puppet.Build();
	FPuppetPerformerCalibration Calibration;
	TestTrue(TEXT("Calibrates"), Sampler.CaptureCalibration(false, 0.0, FRotator::ZeroRotator, Calibration));
	Puppet.Instance.SetCalibration(0, Calibration);
	TestTrue(TEXT("The clip's skeleton"), Puppet.Instance.SetPerformerSkeleton(0, Sampler.GetBoneNames(), Sampler.GetParents(), nullptr, Error));

	TArray<FTransform> Local;
	Sampler.Sample(PuppetClipTests::KeyTime * 0.5, Local);
	TestTrue(TEXT("The clip's transforms"), Puppet.Instance.SetPerformerTransforms(0, Local));
	Puppet.Solve();
	TestEqual(TEXT("Halfway"), Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(10, 0, 200), 1.e-1f);

	Sampler.Sample(PuppetClipTests::KeyTime, Local);
	Puppet.Instance.SetPerformerTransforms(0, Local);
	Puppet.Solve();
	TestEqual(TEXT("At the end"), Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(20, 0, 200), 1.e-1f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
