// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_ControlRig: the performer drives a Control Rig's controls and the rig poses the
// listed bones and curves (ADR 0010). The rig is UPuppetTestControlRig, a native rig on the Face
// fixture's bones: head_ctrl turns and moves the head, jaw_open turns the jaw about Z and sets the
// curve jaw_curve, and its Pre and Post Forwards Solve events read pre_open and post_turn. The
// fixture's rest rotations are identity, so component rotations read as the turns applied.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestControlRig.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetModule_ControlRig.h"

namespace PuppetControlRigTests
{
	/** A Control Rig module on the test rig, posing Bones. */
	UPuppetModule_ControlRig& AddRig(PuppetTests::FPuppet& Puppet, TArray<FName> Bones)
	{
		UPuppetModule_ControlRig& Rig = Puppet.Add<UPuppetModule_ControlRig>();
		Rig.Rig = FControlRigAssetStrongReference(TSubclassOf<UControlRig>(UPuppetTestControlRig::StaticClass()));
		Rig.Bones = MoveTemp(Bones);
		return Rig;
	}

	FPuppetControlRigInput HeadInput(EPuppetControlRigInput Input = EPuppetControlRigInput::JointRotation, float Scale = 1.0f)
	{
		FPuppetControlRigInput Entry;
		Entry.Control = TEXT("head_ctrl");
		Entry.Input = Input;
		Entry.PerformerJoint = TEXT("head");
		Entry.Scale = Scale;
		return Entry;
	}

	/** jawOpen into a float control, jaw_open unless named. */
	FPuppetControlRigInput JawInput(const TCHAR* Control = TEXT("jaw_open"))
	{
		FPuppetControlRigInput Entry;
		Entry.Control = Control;
		Entry.Input = EPuppetControlRigInput::Curve;
		Entry.PerformerCurve = TEXT("jawOpen");
		return Entry;
	}

	/** Builds the puppet, calibrates a Face performer at rest (jawOpen 0) and drives the slot. */
	void Calibrate(PuppetTests::FPuppet& Puppet, PuppetTests::FPerformer& Performer, TConstArrayView<FTransform> RestLocal = {})
	{
		Performer.SetCurve(TEXT("jawOpen"), 0.0f);
		Puppet.Build(RestLocal);
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
	}

	FQuat Yaw(double Degrees)
	{
		return PuppetTests::Rotation(FVector::ZAxisVector, Degrees);
	}

	/** The performer's head turned 40 degrees of yaw since calibration. */
	void TurnHead(PuppetTests::FPuppet& Puppet, PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("head"), Yaw(40.0));
		Puppet.Drive(0, Performer);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigJointRotationTest, "Open3DPuppet.Modules.ControlRig.JointRotationDrivesAControl", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigJointRotationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head") }).Inputs = { PuppetControlRigTests::HeadInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("At rest, the head at rest"), Puppet.Component(TEXT("head")).GetRotation(), FQuat::Identity);

	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("The head turns with the performer's"), Puppet.Component(TEXT("head")).GetRotation(), PuppetControlRigTests::Yaw(40.0));
	TestEqual(TEXT("And keeps its place"), Puppet.Component(TEXT("head")).GetTranslation(), Puppet.Reference(TEXT("head")).GetTranslation(), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigJointTransformTest, "Open3DPuppet.Modules.ControlRig.JointTransformMovesAControl", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigJointTransformTest::RunTest(const FString& Parameters)
{
	// Same skeleton for performer and puppet, so the root scale is 1 and only Scale applies.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head") }).Inputs = { PuppetControlRigTests::HeadInput(EPuppetControlRigInput::JointTransform, 0.5f) };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetLocalTranslation(TEXT("head"), FVector(10.0, 0.0, 10.0));
	Performer.SetLocalRotation(TEXT("head"), PuppetControlRigTests::Yaw(40.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("Half the performer's 10 cm"), Puppet.Component(TEXT("head")).GetTranslation(), Puppet.Reference(TEXT("head")).GetTranslation() + FVector(5.0, 0.0, 0.0), 1.e-3f);
	PuppetTests::TestRotation(*this, TEXT("And the turn"), Puppet.Component(TEXT("head")).GetRotation(), PuppetControlRigTests::Yaw(40.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigLiveScaleTest, "Open3DPuppet.Modules.ControlRig.ScaleIsReadEveryFrame", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigLiveScaleTest::RunTest(const FString& Parameters)
{
	// Scale is a number edited in place, so it applies without binding again (PuppetEditing::IsLiveEdit).
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("head") });
	Rig.Inputs = { PuppetControlRigTests::HeadInput(EPuppetControlRigInput::JointTransform, 0.5f) };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetLocalTranslation(TEXT("head"), FVector(10.0, 0.0, 10.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	Rig.Inputs[0].Scale = 1.0f;
	Puppet.Solve();
	TestEqual(TEXT("The new Scale on the next frame"), Puppet.Component(TEXT("head")).GetTranslation(), Puppet.Reference(TEXT("head")).GetTranslation() + FVector(10.0, 0.0, 0.0), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigCurveTest, "Open3DPuppet.Modules.ControlRig.CurveDrivesAFloatControl", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigCurveTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") }).Inputs = { PuppetControlRigTests::JawInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Half of 30 degrees"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(15.0));
	TestEqual(TEXT("The jaw keeps its place"), Puppet.Component(TEXT("jaw")).GetTranslation(), Puppet.Reference(TEXT("jaw")).GetTranslation(), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigPrePostTest, "Open3DPuppet.Modules.ControlRig.PreAndPostForwardsSolveRun", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigPrePostTest::RunTest(const FString& Parameters)
{
	// jawOpen 0.5 into pre_open opens the jaw 15 degrees through Pre Forwards Solve, and into
	// post_turn turns the head 15 degrees after the forward solve, taking the jaw with it.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head"), TEXT("jaw") }).Inputs = { PuppetControlRigTests::JawInput(TEXT("pre_open")), PuppetControlRigTests::JawInput(TEXT("post_turn")) };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Post Forwards Solve turned the head"), Puppet.Component(TEXT("head")).GetRotation(), PuppetControlRigTests::Yaw(15.0));
	PuppetTests::TestRotation(*this, TEXT("Pre Forwards Solve opened the jaw, on the turned head"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(30.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigCurvesTest, "Open3DPuppet.Modules.ControlRig.RigCurvesReachThePuppet", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigCurvesTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, {});
	Rig.Inputs = { PuppetControlRigTests::JawInput() };
	Rig.Curves = { TEXT("jaw_curve") };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	const float* Value = Puppet.Instance.GetPose().FindCurve(TEXT("jaw_curve"));
	if (TestNotNull(TEXT("The rig's curve is a puppet curve"), Value))
	{
		TestEqual(TEXT("With the rig's value"), *Value, 0.5f, 1.e-4f);
	}
	Rig.Weight = 0.5f;
	Puppet.Solve();
	Value = Puppet.Instance.GetPose().FindCurve(TEXT("jaw_curve"));
	if (TestNotNull(TEXT("Still written"), Value))
	{
		TestEqual(TEXT("Times the weight"), *Value, 0.25f, 1.e-4f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigRefreshVariablesTest, "Open3DPuppet.Modules.ControlRig.RefreshListsTheRigsVariables", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigRefreshVariablesTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") });
	Rig.Variables.AddProperty(TEXT("TailGain"), EPropertyBagPropertyType::Float);
	Rig.RefreshVariables();
	TestEqual(TEXT("The rig's one variable"), Rig.Variables.GetNumPropertiesInBag(), 1);
	TestTrue(TEXT("One the rig no longer has is removed"), Rig.Variables.FindPropertyDescByName(TEXT("TailGain")) == nullptr);
	const TValueOrError<float, EPropertyBagResult> Default = Rig.Variables.GetValueFloat(TEXT("JawGain"));
	if (TestTrue(TEXT("JawGain, a float"), Default.HasValue()))
	{
		TestEqual(TEXT("With the rig's default"), Default.GetValue(), 1.0f);
	}

	Rig.Variables.SetValueFloat(TEXT("JawGain"), 2.0f);
	Rig.RefreshVariables();
	const TValueOrError<float, EPropertyBagResult> Kept = Rig.Variables.GetValueFloat(TEXT("JawGain"));
	if (TestTrue(TEXT("Still listed"), Kept.HasValue()))
	{
		TestEqual(TEXT("A value already set is kept"), Kept.GetValue(), 2.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigVariablesTest, "Open3DPuppet.Modules.ControlRig.VariablesReachTheRig", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigVariablesTest::RunTest(const FString& Parameters)
{
	// JawGain 2 doubles the jaw's turn: jawOpen 0.5 gives 30 degrees, not 15.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") });
	Rig.Inputs = { PuppetControlRigTests::JawInput() };
	Rig.RefreshVariables();
	Rig.Variables.SetValueFloat(TEXT("JawGain"), 2.0f);
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Twice the turn"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(30.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigLiveVariablesTest, "Open3DPuppet.Modules.ControlRig.VariablesAreReadEveryFrame", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigLiveVariablesTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") });
	Rig.Inputs = { PuppetControlRigTests::JawInput() };
	Rig.RefreshVariables();
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("The rig's default gain"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(15.0));
	Rig.Variables.SetValueFloat(TEXT("JawGain"), 3.0f);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("The new gain on the next frame"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(45.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigBadVariableTest, "Open3DPuppet.Modules.ControlRig.UnknownOrMistypedVariableSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigBadVariableTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the Control Rig has no variable 'TailGain'"), ELogVerbosity::Warning);
	AddExpectedMessagePlain(TEXT("the Control Rig's variable 'JawGain' is not of the type the module sets"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Unknown = PuppetControlRigTests::AddRig(Puppet, { TEXT("head") });
	Unknown.Inputs = { PuppetControlRigTests::HeadInput() };
	Unknown.Variables.AddProperty(TEXT("TailGain"), EPropertyBagPropertyType::Float);
	UPuppetModule_ControlRig& Mistyped = PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") });
	Mistyped.Inputs = { PuppetControlRigTests::HeadInput() };
	Mistyped.Variables.AddProperty(TEXT("JawGain"), EPropertyBagPropertyType::Bool);
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	TestEqual(TEXT("The jaw left as the input"), Puppet.Component(TEXT("jaw")), Puppet.Reference(TEXT("jaw")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigInputPoseTest, "Open3DPuppet.Modules.ControlRig.RigStartsFromTheIncomingPose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigInputPoseTest::RunTest(const FString& Parameters)
{
	// An animation turns the head 90 degrees; the rig turns the jaw 15 more on top of it.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") }).Inputs = { PuppetControlRigTests::JawInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	Performer.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Performer);
	Puppet.Solve(Puppet.InputWith(TEXT("head"), FTransform(PuppetControlRigTests::Yaw(90.0), FVector(0.0, 0.0, 10.0))));
	PuppetTests::TestRotation(*this, TEXT("The jaw on the turned head"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(105.0));
	const FVector JawOffset = PuppetControlRigTests::Yaw(90.0).RotateVector(FVector(3.0, 0.0, -2.0));
	TestEqual(TEXT("And under it"), Puppet.Component(TEXT("jaw")).GetTranslation(), Puppet.Component(TEXT("head")).GetTranslation() + JawOffset, 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigListedBonesTest, "Open3DPuppet.Modules.ControlRig.OnlyListedBonesAreWritten", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigListedBonesTest::RunTest(const FString& Parameters)
{
	// The rig turns its head and the jaw with it, but only the jaw is the module's.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") }).Inputs = { PuppetControlRigTests::HeadInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head is left as it came in"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	PuppetTests::TestRotation(*this, TEXT("The jaw takes the rig's turn"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetControlRigTests::Yaw(40.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigWeightTest, "Open3DPuppet.Modules.ControlRig.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("head") });
	Rig.Inputs = { PuppetControlRigTests::HeadInput() };
	Rig.Weight = 0.5f;
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Half the turn"), Puppet.Component(TEXT("head")).GetRotation(), PuppetControlRigTests::Yaw(20.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigRestPoseTest, "Open3DPuppet.Modules.ControlRig.RestPoseIsTheRigsInitialPose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigRestPoseTest::RunTest(const FString& Parameters)
{
	// A rest pose with the jaw 2 cm lower: the rig puts the jaw at its initial transform, which
	// comes from the puppet's rest pose rather than the mesh's reference pose.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") }).Inputs = { PuppetControlRigTests::JawInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	const TArray<FTransform> RestLocal = Puppet.InputWith(TEXT("jaw"), FTransform(FVector(3.0, 0.0, -4.0)));
	PuppetControlRigTests::Calibrate(Puppet, Performer, RestLocal);
	Puppet.Solve();
	TestEqual(TEXT("The jaw at its rest place"), Puppet.Component(TEXT("jaw")).GetTranslation(), Puppet.Component(TEXT("head")).GetTranslation() + FVector(3.0, 0.0, -4.0), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigNoRigTest, "Open3DPuppet.Modules.ControlRig.NoRigSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigNoRigTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("no Control Rig is set"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("head") });
	Rig.Inputs = { PuppetControlRigTests::HeadInput() };
	Rig.Rig.Reset();
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigMissingControlTest, "Open3DPuppet.Modules.ControlRig.MissingControlSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigMissingControlTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the Control Rig has no control 'neck_ctrl'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	FPuppetControlRigInput Input = PuppetControlRigTests::HeadInput();
	Input.Control = TEXT("neck_ctrl");
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head") }).Inputs = { Input };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigControlTypeTest, "Open3DPuppet.Modules.ControlRig.ControlTypeMustSuitTheInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigControlTypeTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the Control Rig's control 'jaw_open' cannot take a joint rotation"), ELogVerbosity::Warning);
	AddExpectedMessagePlain(TEXT("the Control Rig's control 'head_ctrl' is not a float control"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	FPuppetControlRigInput Joint = PuppetControlRigTests::HeadInput();
	Joint.Control = TEXT("jaw_open");
	UPuppetModule_ControlRig& First = PuppetControlRigTests::AddRig(Puppet, { TEXT("head") });
	First.Inputs = { Joint };
	FPuppetControlRigInput Curve = PuppetControlRigTests::JawInput();
	Curve.Control = TEXT("head_ctrl");
	PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") }).Inputs = { Curve };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	TestEqual(TEXT("The jaw left as the input"), Puppet.Component(TEXT("jaw")), Puppet.Reference(TEXT("jaw")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigMissingBoneTest, "Open3DPuppet.Modules.ControlRig.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigMissingBoneTest::RunTest(const FString& Parameters)
{
	// eye_l is on the puppet but not in the rig; tail_01 is in neither.
	AddExpectedMessagePlain(TEXT("the Control Rig has no bone 'eye_l'"), ELogVerbosity::Warning);
	AddExpectedMessagePlain(TEXT("the puppet has no bone 'tail_01'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head"), TEXT("eye_l") }).Inputs = { PuppetControlRigTests::HeadInput() };
	PuppetControlRigTests::AddRig(Puppet, { TEXT("tail_01") }).Inputs = { PuppetControlRigTests::HeadInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigMissingCurveTest, "Open3DPuppet.Modules.ControlRig.MissingRigCurveSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigMissingCurveTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the Control Rig has no curve 'brow_curve'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_ControlRig& Rig = PuppetControlRigTests::AddRig(Puppet, { TEXT("head") });
	Rig.Inputs = { PuppetControlRigTests::HeadInput() };
	Rig.Curves = { TEXT("brow_curve") };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigMissingPerformerTest, "Open3DPuppet.Modules.ControlRig.MissingJointOrCurveSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigMissingPerformerTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no joint 'tail_01'"), ELogVerbosity::Warning);
	AddExpectedMessagePlain(TEXT("the performer has no curve 'beakOpen'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	FPuppetControlRigInput Joint = PuppetControlRigTests::HeadInput();
	Joint.PerformerJoint = TEXT("tail_01");
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head") }).Inputs = { Joint };
	FPuppetControlRigInput Curve = PuppetControlRigTests::JawInput();
	Curve.PerformerCurve = TEXT("beakOpen");
	PuppetControlRigTests::AddRig(Puppet, { TEXT("jaw") }).Inputs = { Curve };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);
	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	TestEqual(TEXT("The head left as the input"), Puppet.Component(TEXT("head")), Puppet.Reference(TEXT("head")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlRigInstancesTest, "Open3DPuppet.Modules.ControlRig.EachInstanceRunsItsOwnRig", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlRigInstancesTest::RunTest(const FString& Parameters)
{
	// Two running puppets from one asset, one performer turned: the other puppet's rig is not.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetControlRigTests::AddRig(Puppet, { TEXT("head") }).Inputs = { PuppetControlRigTests::HeadInput() };
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	PuppetControlRigTests::Calibrate(Puppet, Performer);

	FPuppetInstance Other;
	Other.Initialize(*Puppet.Asset, Puppet.Asset->Modules, Puppet.RefSkeleton);
	FString Error;
	Other.SetPerformerSkeleton(0, Performer.Names, Performer.Parents, nullptr, Error);
	Other.SetPerformerTransforms(0, Performer.Local);
	Other.SetPerformerCurveNames(0, Performer.CurveNames, nullptr);
	Other.SetPerformerCurveValues(0, Performer.CurveValues);
	Other.SetCalibration(0, FPuppetPerformerCalibration::Capture(Performer.ToPose(), FRotator::ZeroRotator));

	PuppetControlRigTests::TurnHead(Puppet, Performer);
	Puppet.Solve();
	Other.Solve(Puppet.RefSkeleton.GetRefBonePose(), 1.0f / 30.0f);
	PuppetTests::TestRotation(*this, TEXT("The turned performer's puppet"), Puppet.Component(TEXT("head")).GetRotation(), PuppetControlRigTests::Yaw(40.0));
	const int32 Head = Other.GetPose().FindBone(TEXT("head"));
	if (TestTrue(TEXT("The other puppet has a head"), Head != INDEX_NONE))
	{
		PuppetTests::TestRotation(*this, TEXT("The other puppet's head at rest"), Other.GetPose().GetComponent(Head).GetRotation(), FQuat::Identity);
	}
	return true;
}

#endif
