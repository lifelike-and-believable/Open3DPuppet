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

Not in CI yet: the UE build and the automation tests. They need a Windows runner with UE 5.8
(WP-9); until then, plugin changes are built and tested at the desk.
