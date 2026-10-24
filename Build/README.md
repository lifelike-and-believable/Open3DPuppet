# Build and CI

## Local build and tests

The commands are in [`AGENTS.md`](../AGENTS.md#commands): build the `ProjectSandboxEditor`
target with UE 5.8's `Build.bat`, then run the `Open3DPuppet` automation tests headless and read
the result from the exported report. A run must execute at least the number of tests in
[`automation-test-floors.json`](automation-test-floors.json).

## CI

[`.github/workflows/plugin-checks.yml`](../.github/workflows/plugin-checks.yml) runs on every PR
and push to `develop` and `main`:

| Job | Script | What it checks |
|---|---|---|
| Copyright headers | `Scripts/check-copyright-headers.py` | Every first-party `.h`, `.cpp` and `.cs` under the plugin's `Source/` starts with the copyright line (rule OP-002). Exceptions need an entry with a reason in `Build/copyright-allowlist.txt`. |
| Runtime modules free of editor code | `Scripts/check-runtime-editor-deps.py` | Runtime modules name no editor-only module in their `Build.cs` and include no editor-only header (ADR 0002). |
| Markdown links resolve | `Scripts/check-markdown-links.py` | Relative links and `#anchors` in every tracked Markdown file resolve. |

The scripts come from Open3DBroadcast and need Python 3.8 or later, standard library only.

[`.github/workflows/ue-build-test.yml`](../.github/workflows/ue-build-test.yml) runs the UE job on
non-draft PRs, pushes to `develop` and `main`, and by hand (Actions > UE build and tests > Run
workflow), on the self-hosted `[self-hosted, ue5, windows]` runner that Open3DBroadcast also uses,
with UE 5.8 at `C:\Program Files\Epic Games\UE_5.8`:

1. `Scripts/Build-Plugin.ps1 -FailOnWarnings`: RunUAT BuildPlugin for Win64; a compiler warning
   in a plugin source file fails the job (rule FAB-004).
2. `Scripts/Run-AutomationTests.ps1 -PluginPackageDir <package> -MinTestsKey plugin`: the
   `Open3DPuppet` tests in a throwaway host project that loads exactly the package from step 1.
   Pass or fail comes from the exported report, and fewer tests than the floor in
   `automation-test-floors.json` is a failure.
3. The report, the editor log and the BuildPlugin log are uploaded as
   `Open3DPuppet-Tests-<sha>`.

Drafts skip the UE job so unfinished work does not hold the shared runner. Both scripts come
from Open3DBroadcast.
