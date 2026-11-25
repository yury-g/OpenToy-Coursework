# Round Waveshare common-menu migration

Spec: docs/CONTROLS.md and docs/APP-STATUS-BAR.md. Owner explicitly requests autonomous source/test/build completion and installation only if enrolled hardware is present. Initial enumeration found no enrolled round board; the owner reconnected it and a fresh enumeration found exactly one enrolled match. No other boards may be opened.

Design: target-only Menu.h state/input controller plus MenuView.h compositor. Retain shared Companion/renderers and existing circular eye projection. Owner added curved top-edge text: two concentric radial glyph baselines retain app/version/date/battery icon; Menu occupies its own 88x36 safe row above eye targets. Three 44px menu rows/page keep seven root labels ordered; explicit Next and Back navigation. Submenus offer current app information (no launcher), three personas, mute/soft/quiet volume, performance confirmation and dim/normal backlight, Controls and About. Unsupported listening and rotation are not working controls.

Input contract: activate on release in the original control; leaving its bounds cancels even after reentry. Page transitions/input loss release app actions and require fresh contact. Performance restore: contact begins in x140..220/y10..40, holds800ms, fires once, restores metadata and opens root Menu. BOOT/RESET/EN unused. Session defaults on reboot.

- [x] Tests first: root/submenu reachability and Back, drag cancellation, invalid/stale input, play-to-menu crossing, held-contact transitions,800ms/wrap restore, mute/volume/persona/brightness, fixed bounds and labels.
- [x] Implement menu controller/view; remove active hidden mute/persona holds; keep current1.0.2 historical controls documented.
- [x] Integrate session state with firmware, amplifier stop on menu transitions, bounded diagnostics; version1.1.0 dated2026-11-17. No microphone/network changes.
- [x] Run full host suite, target build, unchanged Feather1/2 builds, source-generated menu preview inspection and independent review.
- [x] Re-enumerate enrolled identity. If present, ROM/security/layout/digest verification, bounded app install and live menu checks; physical acceptance remains pending. If absent, mark installation blocked/disconnected and retain one reconnect request.
- [x] Publish precise feature/test checklist and update only round-device status/matrix rows, preserving BOM/other tracks. Record installed1.1.0 with precise limits; retain1.0.2 historical evidence.
