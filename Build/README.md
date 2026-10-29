# Build and CI

## Local build and tests

The commands are in [`AGENTS.md`](../AGENTS.md#commands): build the `ProjectSandboxEditor`
target with UE 5.8's `Build.bat`, then run the `Open3DPuppet` automation tests headless and read
the result from the exported report. A run must execute at least the number of tests in
[`automation-test-floors.json`](automation-test-floors.json).

## CI

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) holds every check, so its last job sees
them all. The source checks run on every PR and push to `develop` and `main`, on
`ubuntu-latest`:

| Job | Script | What it checks |
|---|---|---|
| Copyright headers | `Scripts/check-copyright-headers.py` | Every first-party `.h`, `.cpp` and `.cs` under the plugin's `Source/` starts with the copyright line (rule OP-002). Exceptions need an entry with a reason in `Build/copyright-allowlist.txt`. |
| Runtime modules free of editor code | `Scripts/check-runtime-editor-deps.py` | Runtime modules name no editor-only module in their `Build.cs` and include no editor-only header (ADR 0002). |
| Markdown links resolve | `Scripts/check-markdown-links.py` | Relative links and `#anchors` in every tracked Markdown file resolve. |
| Fab source zip | `Scripts/fab-package.py` | Builds the Fab source zip and checks it (see [Fab source package](#fab-source-package)); uploads it as `Open3DPuppet-Fab-Source-<sha>`. Runs on drafts too. |

The scripts come from Open3DBroadcast and need Python 3.8 or later, standard library only.

The UE job runs on non-draft PRs, pushes to `develop` and `main`, and by hand (Actions > CI > Run
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
4. `Scripts/Build-FabZip.ps1`: BuildPlugin on the contents of the zip from the "Fab source zip"
   job, the way Fab's build farm receives it, failing on warnings; then a check that every module
   in the zip's `.uplugin` produced an editor DLL and that nothing of an excluded module did. Its
   BuildPlugin log is uploaded as `Open3DPuppet-FabBuild-<sha>`. It runs when the zip job passed;
   when that job failed, steps 1 to 3 still run.

Drafts skip the UE job so unfinished work does not hold the shared runner. The scripts come
from Open3DBroadcast.

**Temporary (2026-10-28): `-DisableFabPlugin`.** Epic's Fab plugin 0.0.22 adds a `FabToolset`
module that the installed UE 5.8.4's precompiled build rules do not contain, so every build with
Fab enabled fails before compiling (`Expecting to find a type ... named 'FabToolset'`,
RulesError). Steps 1 and 4 pass `-DisableFabPlugin`, which disables Fab for UBT
(`UBT_EXTRA_ARGS=-DisablePlugin=Fab`) and builds only the editor target: UBT refuses to disable
a plugin for the game targets, so the Development and Shipping game builds are not checked until
it is removed. Pass it to local package builds too. To build ProjectSandbox locally, disable Fab
in your copy of `ProjectSandbox.uproject` (not committed). Remove the switch from `ci.yml` once a
Fab or engine update builds again.

The last job, `status`, calls rules-for-robots' reusable
[`rfr-ci-status.yml`](https://github.com/lifelike-and-believable/rules-for-robots/blob/main/.github/workflows/rfr-ci-status.yml).
It posts a new comment on the PR each time the workflow finishes, headed "Open3DPuppet CI
completed successfully!" when every job passed (or failed, or was cancelled), with the full head
SHA and each job's result. A session waiting to merge on green is woken by that comment (the
Claude desktop app's PR monitor relays new comments, not a passing check). Drafts get no comment,
because their UE job skipped. The comment can describe an older commit once a later push lands,
so confirm the checks on the head before merging.

## Fab source package

The Fab listing gets a source-only zip, made as Open3DBroadcast makes its own (roadmap WP-9).

- `Build/Fab/exclude-modules.txt` lists modules left out of it: `Open3DPuppetTests` (ADR 0005).
- `Build/Fab/exclude-files.txt` lists files left out, as globs. It has no rules today.
- `Build/Scripts/fab-package.py` (Python 3.8+, standard library) takes the files git tracks under
  the plugin folder, applies both lists, removes the excluded modules' entries from the staged
  `.uplugin` without reformatting it, and writes a zip with one top-level `Open3DPuppet/` folder
  and fixed timestamps, so the same commit gives the same bytes, plus a manifest (SHA-256, size
  and path per file).
  - **Checks on the zip.** It reopens the zip and fails on any of these:
    - an excluded module (folder or `.uplugin` entry);
    - a listed module without its `Build.cs`;
    - a module without a `PlatformAllowList`, or one naming a platform that
      `SupportedTargetPlatforms` does not list (ADR 0001);
    - `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`;
    - `.pdb`, `.py` or `.pyc` files;
    - a Markdown file other than a root `README.md`, `USER_GUIDE.md` or
      `THIRD_PARTY_LICENSES.md`, or one under a `ThirdParty/` folder;
    - a missing `Resources/Icon128.png`, `Config/FilterPlugin.ini` or `LICENSE`;
    - a top-level file or folder outside `Binaries`, `Config`, `Content`, `Resources`, `Shaders`
      and `Source` that `Config/FilterPlugin.ini` does not list, since BuildPlugin would leave it
      out.
  - **Checks on the tracked tree.** Before any exclusion, so that neither a rule nor an excluded
    module hides a problem, it fails on any of these anywhere under the plugin:
    - a tracked `.pdb`, `.py`, `.pyc`, `.exe` or `.msi`;
    - Markdown under `Source/` other than a `README.md` or `USER_GUIDE.md` outside `ThirdParty/`;
    - a path longer than 170 characters, measured from the plugin folder (rule FAB-003);
    - a folder or file name with anything but English letters, digits and underscores, with
      dot-separated parts allowed in a file name, as in `Open3DPuppet.Build.cs` (rule FAB-003).
  - **Self-test.** `--self-test` runs these checks on made-up file lists, clean and bad; CI runs
    it first.
- `Build/Scripts/Build-FabZip.ps1` runs BuildPlugin on the zip's contents, adding nothing.

```bash
python3 Build/Scripts/fab-package.py --self-test
python3 Build/Scripts/fab-package.py --out-dir Artifacts/Fab
```

```powershell
.\Build\Scripts\Build-FabZip.ps1 -UEPath "C:\Program Files\Epic Games\UE_5.8" -Zip Artifacts\Fab -WorkDir Artifacts\FB
```

Keep `-WorkDir` short: BuildPlugin's intermediate paths under it must stay within Windows' 260
characters.

### Plugin icon

`Resources/Icon128.svg` is the source of the icon: a marionette in the Open3DBroadcast icon's
colours. `Icon128.png` is rendered from it; edit the SVG and render again, never the PNG. The
cairosvg renderer Open3DBroadcast uses needs a native cairo library that Windows does not have,
so the PNG was rendered with a browser and downsampled with Pillow:

1. Open the SVG in an HTML page that shows it at 512 by 512 pixels on a transparent background,
   and screenshot it with headless Edge (`msedge --headless=new --default-background-color=00000000
   --window-size=512,512 --screenshot=<file> <page>`).
2. Downsample to 128 by 128 with Pillow's LANCZOS filter and save as PNG (RGBA).

Check it at 64 and 32 pixels too: Unreal's plugin browser and a Fab listing card show it small.
