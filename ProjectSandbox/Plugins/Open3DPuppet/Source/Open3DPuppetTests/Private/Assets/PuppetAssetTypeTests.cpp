// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// The plugin's asset types are first-class in the editor: each has its own factory and asset
// definition, so the Content Browser's Add menu offers it under Puppet, rather than a generic
// Data Asset whose class the user then has to pick.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetDefinition.h"
#include "AssetDefinitionRegistry.h"
#include "Engine/DataAsset.h"
#include "Factories/Factory.h"
#include "Misc/FeedbackContext.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

namespace PuppetAssetTypeTests
{
	/** The plugin's asset classes users create: every non-abstract data asset class in Open3DPuppet. */
	TArray<UClass*> AssetClasses()
	{
		TArray<UClass*> Classes;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UDataAsset::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)
				&& It->GetOutermost()->GetName() == TEXT("/Script/Open3DPuppet"))
			{
				Classes.Add(*It);
			}
		}
		return Classes;
	}

	/** The factory that creates Class from the Add menu, or null. */
	UFactory* FactoryFor(UClass* Class)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UFactory::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
			{
				UFactory* Factory = It->GetDefaultObject<UFactory>();
				if (Factory->GetSupportedClass() == Class && Factory->CanCreateNew() && Factory->ShouldShowInNewMenu())
				{
					return Factory;
				}
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetAssetTypesTest, "Open3DPuppet.Assets.Types.EachHasAFactoryAndACategory", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetAssetTypesTest::RunTest(const FString& Parameters)
{
	const TArray<UClass*> Classes = PuppetAssetTypeTests::AssetClasses();
	for (const TCHAR* Name : { TEXT("PuppetAsset"), TEXT("PuppetPerformerProfile"), TEXT("PuppetCalibrationAsset") })
	{
		TestTrue(FString::Printf(TEXT("%s is a plugin asset type"), Name), Classes.ContainsByPredicate([Name](const UClass* Class) { return Class->GetName() == Name; }));
	}
	for (UClass* Class : Classes)
	{
		UFactory* Factory = PuppetAssetTypeTests::FactoryFor(Class);
		if (!TestNotNull(FString::Printf(TEXT("%s has a factory in the Add menu"), *Class->GetName()), Factory))
		{
			continue;
		}
		UObject* Created = Factory->FactoryCreateNew(Class, GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), Class), RF_Transient, nullptr, GWarn);
		TestTrue(FString::Printf(TEXT("The factory creates a %s"), *Class->GetName()), Created && Created->IsA(Class));

		const UAssetDefinition* Definition = UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(Class);
		if (!TestNotNull(FString::Printf(TEXT("%s has an asset definition"), *Class->GetName()), Definition))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s's definition is for it"), *Class->GetName()), Definition->GetAssetClass() == Class);
		const TConstArrayView<FAssetCategoryPath> Categories = Definition->GetAssetCategories();
		TestTrue(FString::Printf(TEXT("%s is under Puppet"), *Class->GetName()),
			Categories.Num() > 0 && Categories[0].GetCategoryText().ToString() == TEXT("Puppet"));
		TestFalse(FString::Printf(TEXT("%s has a display name"), *Class->GetName()), Definition->GetAssetDisplayName().IsEmpty());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
