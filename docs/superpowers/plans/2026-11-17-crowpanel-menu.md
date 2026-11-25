# CrowPanel common controls implementation plan

> Execute inline with superpowers:executing-plans. Owner already requested finishing the shared feature sets without another approval; hardware stays released to Copy-Face.

**Goal:** Prepare, host-test and build standalone Eyes 0.3.0 with the approved family Menu and full performance behavior. Do not install.

**Architecture:** Portable Menu.h owns page state, geometry and packet-driven gesture recognition; App.cpp renders the same geometry under the LVGL lock. Copy-Face may reuse this scoped header from main, but no shared files are edited concurrently. Preserve smooth eyes and behavior.

**Tech stack:** Existing C++14 / LVGL8.4 / ESP-IDF5.4.2, pinned dependencies.
**Spec:** docs/CONTROLS.md, family standard v1.0. Normal header88px includes metadata, unknown battery, current personality/ECO and a visible Menu button. Remove footer. Normal eyes use scale7, performance scale8 with full600px viewport. Menu pages overlay play. Touch controls use begin+release inside, permanent drag-out cancellation, and no activation from multi-touch. Performance removes all bars; a fresh single contact begun and held800ms in x352..671/y0..63 restores header+Menu. Leaving the zone cancels until release. These fixed-landscape bounds are implementation choices.

## Constraints and review focus

- No serial, reset, flash, camera or charger writes; installed0.2.2 and recovery remain untouched.
- Pages: Return to play, Apps, Personality, Display, Controls, About. Omit Sound because audio is unavailable. Apps shows current Eyes and explains more apps require installation.
- Display exposes ECO and Performance. Performance confirmation and Controls teach restore. No persistent overlays in performance; reboot normal/Curious/ECO.
- Touch origin prevents play-to-menu drags; multi-contact ambiguity cancels menu gestures. Scene pokes still support two contacts.
- Input failure/staleness clears held play/menus and requires release; no timed action on cached packets.800ms and100ms freshness arithmetic must work across wrap.
- Layout lock failure must retain last shown geometry and suppress input during pending transitions. Opening/closing menu releases physical/synthetic pokes.

## Tasks

- [x] Add tests/menu.cpp with navigation, release-inside, drag-out/reentry, multi-touch/reordering, performance800ms boundary, stale/failure/wrap, transition release barriers, page/button bounds and live play contact cases. Observe missing-feature failure; implement Menu.h and pass.
- [x] Update Presentation viewport tests first for88px header/no footer; implement geometry. Adapt App.cpp to state-driven header/overlay/buttons and same hit rectangles, with press feedback and explicit selection text. Diagnostics report menu/performance; normal app controls never use BOOT/RESET. Set source metadata0.3.0.
- [x] Run project host suite; build only in private/eyes-0.3.0; verify image/header/capacity/source digest. Obtain fresh review; resolve blockers. Do not repeat old smooth renderer benchmarks or open hardware.
- [x] Publish source/build evidence and feature checklist distinguishing installed0.2.2 from candidate0.3.0; share scoped reusable header with Copy-Face. Deployment remains pending explicit hardware handback, then separate live acceptance.

## Execution evidence

Menu test first failed against the absent interface, then against the no-op
interface at the release/pressed contract. Real implementation passes. Viewport
test failed on the old header transform and passes after geometry migration.
Full project suite passes; IDF5.4.2 candidate614096bytes with valid P4 image.

Final review: fresh reviewer found no Critical/Important blockers.
Final: minor (deferred): raw contacts diagnostic can briefly report0 for fresh
non-eye UI contact on a no-new-packet cycle; gesture execution is unaffected.
Final: minor (deferred): failed-layout lock/input integration lacks a host regression
test beyond stored-layout state; suppression reviewed in the adapter.
No scope/design rulings beyond the documented fixed-landscape geometry choices.
Deployment remains pending explicit hardware handback.
