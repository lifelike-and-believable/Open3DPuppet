# Architecture Decision Records

This folder holds the Architecture Decision Records (ADRs) for Open3DPuppet. Each ADR records one
decision: the problem, the options that were considered, what was chosen and why, and what follows
from it. ADRs need maintainer sign-off before coding work that depends on them is merged.

## Index

| ADR | Title | Status |
|---|---|---|
| [0001](0001-platform-and-engine-scope.md) | Platform and engine scope | Proposed |
| [0002](0002-puppet-module-architecture.md) | Puppet module architecture | Proposed |
| [0003](0003-performer-input-and-calibration.md) | Performer input and calibration | Proposed |
| [0004](0004-multiple-performers.md) | Several performers on one puppet | Proposed |
| [0005](0005-test-layout.md) | Test layout | Proposed |
| [0006](0006-fingers-face-and-curves.md) | Fingers, face and curves | Proposed |

## Conventions

- **File name:** `NNNN-short-slug.md`, numbered in the order they are written. Numbers are never reused.
- **Status values:** `Proposed` (written, awaiting sign-off), `Accepted` (maintainer signed off), `Superseded by NNNN`, `Rejected`. When an ADR is accepted, update its Status line and the index above.
- **Evidence:** factual claims about this repository or the engine cite `path:line`. Engine paths are relative to the engine root (`Engine/...`) and name the engine version they were read from.
- **Unverified claims:** anything that depends on Unreal Engine behaviour that could not be confirmed from source is marked **needs-verification** and repeated under *Open questions*.
- An ADR is not edited after acceptance except to change its Status. A changed decision gets a new ADR that supersedes the old one.

## Template

```markdown
# NNNN: <Title>

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** YYYY-MM-DD
- **Related:** <other ADRs, work packages>

## Context
What is true today, with evidence. What forces a decision now.

## Decision drivers
The criteria the options are judged against, most important first.

## Options considered
### Option A: <name>
- Pros:
- Cons:

## Decision
One clear recommendation, stated so a coding agent can act on it.

## Consequences
What gets easier, what gets harder, which work packages this enables or constrains.

## Open questions for the maintainer
Numbered, including every needs-verification item.
```
