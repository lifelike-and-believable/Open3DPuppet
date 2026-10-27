// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/CoreMisc.h"
#include "Modules/ModuleManager.h"
#include "SPuppetSessionPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "Open3DPuppet"

namespace Open3DPuppetEditor
{
	const FName SessionTab(TEXT("PuppetSession"));
}

/** Registers the Puppet Session panel (WP-8) under Window > Tools. */
class FOpen3DPuppetEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Commandlets and runs without Slate have no tabs to offer.
		if (IsRunningCommandlet() || !FSlateApplication::IsInitialized())
		{
			return;
		}
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Open3DPuppetEditor::SessionTab, FOnSpawnTab::CreateStatic(&FOpen3DPuppetEditorModule::SpawnSessionTab))
			.SetDisplayName(LOCTEXT("SessionTab", "Puppet Session"))
			.SetTooltipText(LOCTEXT("SessionTabTip", "Performers and calibration, running puppets and their hand-offs, and live module tuning."))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
		bRegisteredSessionTab = true;
	}

	virtual void ShutdownModule() override
	{
		if (bRegisteredSessionTab && FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Open3DPuppetEditor::SessionTab);
		}
	}

private:
	static TSharedRef<SDockTab> SpawnSessionTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SPuppetSessionPanel)];
	}

	bool bRegisteredSessionTab = false;
};

IMPLEMENT_MODULE(FOpen3DPuppetEditorModule, Open3DPuppetEditor)

#undef LOCTEXT_NAMESPACE
