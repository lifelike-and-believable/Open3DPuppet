<!--
Title: the work package and what it does, for example
"WP-4: digitigrade leg module (ADR 0002)". One work package per PR, based on develop.
-->

## What and why

<!-- What changes, the work package and ADRs it implements, and why this approach. Name any maintainer decision it relies on, with its date. -->

## Tests

<!-- The tests that failed before the change and pass after it, with the red and green runs (or why the change has no test).
     The commands are in AGENTS.md: the UE build and automation tests for plugin changes. -->

## Not verified

<!-- Anything that needs a person at the desk: a UE build on Windows, a live performer, a capture stage. "None" if nothing. -->

## Checklist

- [ ] **Module contract:** new or changed puppet modules follow `docs/guides/authoring-puppet-modules.md` (rule OP-006); contract changes have an ADR.
- [ ] **Threads:** nothing new blocks the game thread; solver code reads game-thread state only through what `PreUpdate` copied, and does not allocate per frame.
- [ ] **Untrusted input:** Live Link data (bone counts, parent indices, transforms) is range-checked before use, with a test for malformed input.
- [ ] **UE APIs:** new engine API use cites its UE 5.8 source location (rule UE-001).
- [ ] **Tests:** they fail without the change and pass with it; no placeholder assertions; `Build/automation-test-floors.json` raised by the tests added.
- [ ] **Build:** no new warnings.
- [ ] **Scope:** no drive-by refactors; behaviour changes and refactors are separate PRs.
- [ ] **Docs:** user-visible changes update the design overview, the authoring guide, the affected READMEs and `CHANGELOG.md`.
