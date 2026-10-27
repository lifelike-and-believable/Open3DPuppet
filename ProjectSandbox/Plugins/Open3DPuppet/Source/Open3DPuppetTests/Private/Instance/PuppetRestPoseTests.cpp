// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// The puppet's rest pose (WP-2, ADR 0003): modules retarget from the pose the puppet asset names
// rather than the mesh's reference pose, and a clip on the puppet's skeleton provides it.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetClipSampler.h"
#include "PuppetModules/PuppetModule_Chain.h"

namespace PuppetRestPoseTests
{
	/** The left arm, which lies along +Y in the biped's reference pose, turned 45 degrees down. */
	const FQuat ArmDown = PuppetTests::Rotation(FVector::XAxisVector, -45.0);

	/** A puppet with a Chain module on the left arm, upper arm to forearm. */
	struct FRig
	{
		FRig() : Puppet(PuppetTests::Biped())
		{
			UPuppetModule_Chain& Arm = Puppet.Add<UPuppetModule_Chain>();
			Arm.PerformerStart = TEXT("upperarm_l");
			Arm.PerformerEnd = TEXT("lowerarm_l");
			Arm.PuppetStart = TEXT("upperarm_l");
			Arm.PuppetEnd = TEXT("lowerarm_l");
		}

		/** Builds with RestLocal, calibrates a performer standing with the arm down, then raises it to the reference pose and solves. */
		FQuat SolveArmRaised(TConstArrayView<FTransform> RestLocal)
		{
			PuppetTests::FPerformer Performer(PuppetTests::Biped());
			Puppet.Build(RestLocal);
			Performer.SetLocalRotation(TEXT("upperarm_l"), ArmDown);
			Puppet.Drive(0, Performer);
			Puppet.Calibrate(0, Performer);
			Performer.SetLocalRotation(TEXT("upperarm_l"), FQuat::Identity);
			Puppet.Drive(0, Performer);
			Puppet.Solve();
			return Puppet.Component(TEXT("upperarm_l")).GetRotation();
		}

		PuppetTests::FPuppet Puppet;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRestPoseRetargetTest, "Open3DPuppet.Instance.RestPose.ModulesRetargetFromTheRestPose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRestPoseRetargetTest::RunTest(const FString& Parameters)
{
	// A performer who calibrates with the arm down, on a puppet whose rest pose has the arm down
	// too: raising the arm to horizontal raises the puppet's to horizontal.
	{
		PuppetRestPoseTests::FRig Rig;
		TArray<FTransform> Rest = Rig.Puppet.RefSkeleton.GetRefBonePose();
		Rest[Rig.Puppet.RefSkeleton.FindBoneIndex(TEXT("upperarm_l"))].SetRotation(PuppetRestPoseTests::ArmDown);
		PuppetTests::TestRotation(*this, TEXT("With the rest pose"), Rig.SolveArmRaised(Rest), FQuat::Identity);
		PuppetTests::TestRotation(*this, TEXT("The rest pose is the reference modules see"), Rig.Puppet.Reference(TEXT("upperarm_l")).GetRotation(), PuppetRestPoseTests::ArmDown);
	}
	// Without it the same motion lifts the puppet's arm 45 degrees past horizontal.
	{
		PuppetRestPoseTests::FRig Rig;
		PuppetTests::TestRotation(*this, TEXT("Without"), Rig.SolveArmRaised({}), PuppetRestPoseTests::ArmDown.Inverse());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRestPoseSampleTest, "Open3DPuppet.Clips.Sampler.SamplesRotationsOntoAPuppet", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRestPoseSampleTest::RunTest(const FString& Parameters)
{
	// A rest pose clip on the biped skeleton, applied to a bigger mesh that also has feathers.
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	PuppetTests::FPerformer Pose(PuppetTests::Biped());
	Pose.SetLocalRotation(TEXT("upperarm_l"), PuppetRestPoseTests::ArmDown);
	Pose.SetLocalTranslation(TEXT("pelvis"), FVector(30, 0, 100));
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, { Pose.Local, Pose.Local });
	FPuppetClipSampler Sampler;
	FString Error;
	if (!TestTrue(TEXT("Binds"), Sampler.SetSequence(Clip.Get(), Error)))
	{
		return false;
	}

	const FReferenceSkeleton Target = PuppetTests::MakeReferenceSkeleton(PuppetTests::Scaled(PuppetTests::Wing(), 2.0));
	TArray<FTransform> Local;
	if (!TestTrue(TEXT("Samples"), Sampler.SampleOnto(0.0, Target, Local)) || !TestEqual(TEXT("One per target bone"), Local.Num(), Target.GetNum()))
	{
		return false;
	}
	const int32 Arm = Target.FindBoneIndex(TEXT("upperarm_l"));
	const int32 Pelvis = Target.FindBoneIndex(TEXT("pelvis"));
	const int32 Feather = Target.FindBoneIndex(TEXT("feather_01"));
	PuppetTests::TestRotation(*this, TEXT("The clip's rotation"), Local[Arm].GetRotation(), PuppetRestPoseTests::ArmDown, 1.e-3);
	TestEqual(TEXT("The target's own bone lengths"), Local[Pelvis].GetTranslation(), Target.GetRefBonePose()[Pelvis].GetTranslation(), 1.e-3f);
	TestTrue(TEXT("A bone the clip lacks keeps the target's reference"), Local[Feather].Equals(Target.GetRefBonePose()[Feather], 1.e-3));

	FPuppetClipSampler Unbound;
	TestFalse(TEXT("Nothing to sample"), Unbound.SampleOnto(0.0, Target, Local));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
