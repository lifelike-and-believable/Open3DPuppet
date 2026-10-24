---
id: OP-002
title: Copyright line on plugin source files
level: MUST
scope: project
paths: ["**/*.h", "**/*.cpp", "**/*.inl", "**/*.cs"]
verified-by: [ci]
check: "ci: Copyright headers (Build/Scripts/check-copyright-headers.py)"
targets-failure: convention-drift
observed-on: []
rationale: Fab wants the publisher and the year, and vendored code keeps its own notice; replaces FAB-002.
---
Start each new plugin source file under `Source/` with `// Copyright <year> Lifelike & Believable. All Rights Reserved.`, using the current year, followed by a blank line. Leave third-party files under any `ThirdParty/` folder and generated files untouched. Run `python3 Build/Scripts/check-copyright-headers.py` and include its output.
