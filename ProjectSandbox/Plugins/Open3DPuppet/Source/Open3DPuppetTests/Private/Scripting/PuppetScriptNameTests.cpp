// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Python names of the plugin's enums and structs. The Python plugin names an enum after its C++
// name without the E, and a struct after its name without the F, unless ScriptName meta says
// otherwise; when an enum and a struct land on one name, the second one is not exposed to Python.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_METADATA

#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"

namespace PuppetScriptNameTests
{
	bool IsPluginType(const UField& Type)
	{
		return Type.GetOutermost()->GetName().StartsWith(TEXT("/Script/Open3DPuppet"));
	}

	/** ScriptName meta (its first entry) if set, otherwise the name with an enum's E removed, as the Python plugin does. */
	FString PythonName(const UField& Type)
	{
		FString Name = Type.GetMetaData(TEXT("ScriptName"));
		if (!Name.IsEmpty())
		{
			Name.Split(TEXT(";"), &Name, nullptr);
			return Name;
		}
		Name = Type.GetName();
		if (Type.IsA<UEnum>() && Name.Len() >= 2 && Name[0] == TEXT('E') && FChar::IsUpper(Name[1]))
		{
			Name.RemoveAt(0);
		}
		return Name;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetScriptNamesUniqueTest, "Open3DPuppet.Scripting.Names.EnumsAndStructsDoNotShareAPythonName", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetScriptNamesUniqueTest::RunTest(const FString& Parameters)
{
	using namespace PuppetScriptNameTests;

	TMap<FString, FString> Owners;
	for (TObjectIterator<UEnum> It; It; ++It)
	{
		if (IsPluginType(**It))
		{
			Owners.Add(PythonName(**It), It->GetPathName());
		}
	}
	int32 Checked = Owners.Num();
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		if (!IsPluginType(**It))
		{
			continue;
		}
		++Checked;
		if (const FString* Enum = Owners.Find(PythonName(**It)))
		{
			AddError(FString::Printf(TEXT("%s and %s are both unreal.%s in Python"), **Enum, *It->GetPathName(), *PythonName(**It)));
		}
	}
	TestTrue(TEXT("the plugin's enums and structs were found"), Checked > 0);
	return true;
}

#endif
