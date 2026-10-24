# 0009: Derived finger-curl curves

- **Status:** Accepted (maintainer sign-off 2026-10-24)
- **Date:** 2026-10-24
- **Related:** [ADR 0006](0006-fingers-face-and-curves.md) item 7 (finger curl as a derived performer curve), [ADR 0002](0002-puppet-module-architecture.md) (the contract), roadmap WP-12, WP-13

## Context

ADR 0006 item 7 decided that finger curl becomes a derived performer curve (`curl_index_l` and
so on), computed from finger bones, so any curve-driven module can use hand gestures as controls:
a talon or a wolf's lip driven by a fist. It left open how curl is measured and where the values
are computed.

What is true today (`develop` at `269ae83`):

- Performer curves live in `FPuppetPerformerPose`: names from Live Link static data, values per
  frame, looked up by canonical name (`FindCurve`).
- A calibration captures each curve's neutral value (`FPuppetPerformerCalibration::Capture`), and
  modules read the change from neutral (`FPuppetSolveContext::GetCurveDelta`).
- A finger's bend relative to its parent since calibration is
  `PuppetRetarget::RotationDeltaInParent`; the Digits module (WP-12) uses it through
  `GetJointDeltaInParent`.
- A finger bone's own local rotation is not a bend: the mannequin's finger bones are not
  identity at rest, and the angle of a rotation is not additive, so "local angle now minus local
  angle at rest" is not the bend since rest. Curl needs the calibration's rest pose.

## Decision drivers

1. A curve module reads a curl exactly as it reads `jawOpen`: by name, through `FindCurve`, with
   `GetCurveDelta`.
2. Curl means the same on every performer and source: 0 in the calibrated neutral hand, whatever
   the bones' rest orientations.
3. No per-frame allocation, no change for modules that do not use it.

## Options considered

### Option A: derived curves in the performer pose, values computed from the calibration (chosen)
- Pros: modules find them like any other curve; one definition for the whole library.
- Cons: the performer pose (a contract type) gains derived curves and a call that needs the
  calibration.

### Option B: a solve-context helper (`GetFingerCurl(finger)`)
- Pros: no change to the performer pose.
- Cons: curve modules would need a second input type for the same kind of control, and a curl
  could not stand in for a source curve by name.

## Decision

1. **Names.** `curl_<finger>_<side>` for `thumb`, `index`, `middle`, `ring` and `pinky`, sides `l`
   and `r`, matching the canonical joint names. A performer has a curl curve when it has that
   finger's three canonical joints (`<finger>_01_<side>` to `<finger>_03_<side>`, found through
   the profile like any joint). A source curve with the same canonical name wins, and no curl is
   derived for it.
2. **Value.** The sum of the unsigned bend angles of the finger's three joints, each relative to
   its parent since calibration (the angle of `RotationDeltaInParent`), divided by 270 degrees:
   0 in the calibrated pose, 1 when each joint has bent 90 degrees. Bending back counts as curl.
   It is not clamped, so a hyperextended or overcurled finger can pass 1.
3. **Where.** `FPuppetPerformerPose` appends the derived curves after the source curves; their
   names come with the skeleton. `UpdateDerivedCurves(Calibration)` computes their values; the
   instance calls it each frame for every ready slot, before the modules run.
4. **Calibration.** A calibration captures the source curves only. A derived curve's neutral is
   therefore 0, and `GetCurveDelta` returns its value: it is already measured from calibration.
   Live Link curve counts (`HasCurveNames`, `SetCurveValues`) are counted against the source
   curves only.
5. **Contract change.** `FPuppetPerformerPose` gains `UpdateDerivedCurves`,
   `GetSourceCurveNames` and `GetSourceCurveValues`, and `GetCurveNames`/`GetCurveValues` include
   the derived curves. Existing modules need no change.

## Consequences

- Any curve module (WP-13 Curve to bone, Curve remap) can take `curl_index_l` as its input, so a
  talon, a lip or an ear can follow a hand gesture.
- Curl has no direction: a puppeteer who needs "only forward" bends uses the Digits module or a
  bone-driven module instead.
- A body slot without finger bones has no curl curves; its curve list is unchanged.

## Open questions for the maintainer

None. The definition (items 1 and 2) and the place (item 3) are the maintainer's choices of
2026-10-24.
