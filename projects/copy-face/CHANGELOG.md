# Copy-Face revisions

## 0.2.2-proto (source candidate, not installed) — 2026-11-17

- Replaced misleading `LV_SYMBOL_BATTERY_EMPTY` header badge with truthful neutral
  `BAT --` / `BAT X.XXV` formatting (`core/BatteryStatus.h`, `tests/battery_status.cpp`).
- Added validated dense lip/eyelid landmark adapter (`core/LipLandmarkAdapter.h`,
  `tests/lip_landmarks.cpp`) supporting 468-point MediaPipe FaceMesh (`Apache-2.0`)
  and 68-point iBUG/PFLD (`MIT`) inner-lip and 6-point eye rings, while enforcing
  `applyFivePointOnly()` on 5-point `human_face_detect` outputs.
- Added `docs/LIP-MODEL-FEASIBILITY.md` documenting `esp-dl 3.3.15` operator support,
  `esp-ppq` quantization requirements, memory budget, and why raw pixel ridge scans
  do not set `mouthValid = true`. Parked CrowPanel (`0.2.0-proto`) untouched.

## 0.2.1-proto — 2026-11-17

- Added camera startup-stage diagnostics and a visible unavailable/error message
  after the first physical attempt returned no sensor identity or frames.
- Kept all camera failure paths neutral; no synthetic image is substituted.

## 0.2.0-proto — 2026-11-17

- Added local SC2336 capture and five-point face detection on separate workers,
  matching image/geometry publication and stale-result rejection.
- Added a small developer camera preview with green face landmarks, plus a
  separate lower mouth area. Live mouth and eye-activity inference stay unavailable.
- Added nine-sample upper/lower lip geometry, asymmetric contact-line rendering,
  smile/frown/neutral/open shape guesses and eyelid contours for explicit simulation.
- Added the family Menu, release-to-activate touch handling, Display controls and
  top-center 0.8-second Performance restore.
- Pinned camera/model dependencies and revision-1 SC2336 tuning for IDF 5.4.2.
- Added geometry, preview and menu checks. Physical results are recorded separately
  in VERIFICATION.md; synthetic previews do not establish camera performance.

## 0.1.0-proto — 2026-11-17

- Created `copy-face` as a separate OpenToy app/catalog entry, display name
  Copy-Face and header COPY-FACE. Animatronic Eyes is unchanged.
- Added bounded geometry-based face association, confidence/acquisition
  hysteresis, time-based smoothing, mirror mapping and neutral face-loss fallback.
- Reused the smooth native CrowPanel eye renderer with large pupils and preserved
  source hashes/notices; added a coordinated mouth and explicit synthetic input.
- Added canonical metadata, honest unavailable battery display and reversible
  session-only header controls; simulation state stays visible.
- Host tracking/scene checks and P4 target build pass; exported synthetic preview.
- Recorded revision-correct camera/demo evidence and the five-landmark mouth
  limitation. Real camera capture, face inference and mouth inference are pending.
- Included family-roadmap notes for later audio and measured battery follow-up.

This is a **synthetic-only, uninstalled prototype**, not a camera-working release.
Hardware remains untouched until explicit device-owner release. No previous
release tags, source, recovery data or device state are changed.
