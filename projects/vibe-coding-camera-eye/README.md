# Vibe Coding Camera Eye

Use Tab5 as a movable camera helper to inspect other OpenToy devices during firmware development, reducing requests for the owner to describe their screens.

Status: November 17, 2026 — **implemented and live-verified against Tab5 `0.1.1`** for local capture, separate target crops, bounded animation bursts, and strict `PASS`/`FAIL`/`INCONCLUSIVE` provenance grading (`core/EvidenceGrade.h`, `tools/camera_eye_eval.py`, `tests/camera_eye_test.cpp`).

## Live evidence & honesty rules
- Live dual-target `1280x720` captures retrieved locally with CRC32, advancing sequence numbers, AE-warmed timestamps, and `<= 140 ms` capture-to-send freshness:
  - **UNO Q + 2.8-inch TFT** (left crop): showed only a lit backlight in the initial captures. Later 0.2.0 captures show the color-test corner marker and then the installed Eyes 0.1.0 header, both eyes and battery badge on the foreground TFT. The separate mirror reflection could not establish the Tab5’s own header.
  - **Round Waveshare 1.85C** (right crop): observed baseline `EYES1 V1.1.0` (`Menu 1 of 3`), then upgraded `EYES1 V1.1.1` Play eyes, `Menu 1 of 4`, and a 12-frame Play burst showing pupil motion.
- Strict honesty guards enforced by `core/EvidenceGrade.h` and `tools/camera_eye_eval.py`:
  - Stills cannot prove motion or smoothness (`INCONCLUSIVE` if `< 3` frames).
  - Stale, non-advancing, missing-provenance, or unverified frames cannot pass; frozen clips or version mismatches (`intended != observed`) fail.
  - Camera images never establish physical touch feel, audio loudness, electrical compatibility, or charging current (`INCONCLUSIVE`).
- Privacy & retention: all captures remain local with bounded stop and retention (`enforce_bounded_retention`); raw camera images and private device IDs are never pushed to GitHub.

See the [Tab5 track](../tab5/README.md), [roadmap](../../docs/ROADMAP.md) and [checkpoint](../../docs/CHECKPOINT-2026-11-17.md).
