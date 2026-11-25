# Orange Face Implementation Plan

> **For agentic workers:** execute inline with superpowers:executing-plans; check tasks in order.

**Goal:** Add Orange's battery and touchscreen navigation to the existing three-device companion.
**Architecture:** Shared packet/state/rendering code, small Nesso hardware adapter, direct peer telemetry.
**Tech Stack:** ESP32 Arduino3.3.10, Adafruit graphics/MAX17048, M5GFX0.2.25.
**Spec:** ../specs/2026-10-19-orange-face.md

## Global constraints

Preserve big135x240 portrait UI, original face headers and private flash backups. No battery tests or altered charger configuration. Freshness8seconds, updates2seconds, known MACs only. All devices already authorized for installation.

## Review focus

Wrong source or reset must not mix identities/history. Missing/stale battery must blank live values. Tap/vertical/noisy touch must not accidentally select or start tests. Nesso native gauge reads must not rewrite calibration/charger settings. Physical swipe needs real evidence.

## Tasks

- [ ] Back up Orange16MB flash, parse partitions and verify digest. Snapshot current Feathers.
- [ ] Extend PeerBattery.h version3 with measured-current/charger/diagnostic-age fields and known Orange MAC; add packet routing/liveness/reset and gesture tests, see failure then pass. Add FaceNavigation.h with testable horizontal-swipe/tap selection.
- [ ] Add Nesso hardware adapter using M5GFX display/touch and read-only shared I2C register access. Generalize main.cpp to three device indices with per-peer histories; preserve Feather headless/persistence behavior. Reuse drawBattery and face renderer, add measured-current display and selected labels.
- [ ] Compile both board targets; all host suites. Review implementation independently, fix material issues.
- [ ] Install app only on identity-matched boards; verify radio values on all six paths, framebuffers and stale/resume; ask owner for actual touch swipes while recording telemetry. Restore default selections, no test sessions started.
- [ ] Save build/live evidence and personal checkpoint, report outcome/limits clearly.

Ruling: existing workspace is a dedicated non-Git hardware workspace; preserve v9 source/binary snapshots rather than introduce a Git checkout. User authorized adding/installing/testing all three; proceed inline without redundant design approvals. Orange original app remains in app0 (OTA sequence1), app limit0x140000; compiled1.1MB image fits, so no partition/bootloader changes. Original app reboot changed28NVSbytes during backup verification; refreshed head while halted, whole16MB snapshot now digest-verified. Use M5GFX directly, avoiding M5Unified's charger/gauge writes. Peer sparkline histories now update independently of selected view.

- [ ] User steering: back up Tab5P4, addBlueprivateAPUDPbridge preservingpeerages; addTab5landscape3carddisplay, verifycurrent3rawvalues+sourceTTL and screenshot.
- [ ] FixOrangefreeze bycachedoriginalpose/closedeyes; provepixelcompatibilityandrecordnewphysicalswipesequence.

Latest plan extension: confirmed printed-model Waveshare169 Yellow plus4-source Tab5monitor. Full16MB Tab5/Yellow backups complete and hardware digest matched; app0-only active slots preserved. Four-node host tests now10suitespass. Task-local M5GFX preserves charger-control bits before anywrite. V12 live test caught intermittent YellowTX and native-USB report stalls; v13 disables unsolicited reports/defaultTXwaiting, bounds requested command output, disconnects savedAP scanning, disables modem sleep and reports actual channel/ACKs. Validate all12wireless paths, all12full-size views, source loss/resume on all4displays, Tab5 source-matched values/nativeframebuffer,20secondno-readerUSB regression, and physical Orange/Yellow touch separately. No battery tests or modem firmware changes.
