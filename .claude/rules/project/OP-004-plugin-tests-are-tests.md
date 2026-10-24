---
id: OP-004
title: Keep the plugin's test module honest
level: SHOULD
scope: project
paths: ["**/Open3DPuppetTests/**"]
verified-by: [review]
targets-failure: test-gaming
observed-on: []
rationale: TEST-001's paths match folders named Tests, not the Open3DPuppetTests module, so TEST-001 would not load for most of this project's tests.
---
Files under `Open3DPuppetTests/` are tests, and TEST-001 applies to them: make failing tests pass by changing the implementation, leave existing assertions and expected values as they are, and stop and say which test and why if one looks wrong. When a change of behaviour is intended, replace the test in the same change and list it, with the reason, in your report.
