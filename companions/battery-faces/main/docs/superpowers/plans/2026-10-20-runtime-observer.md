# Runtime Observer Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task by task.

**Goal:** Prepare a passive background log and optional four-battery race without starting discharge now.
**Architecture:** Host-tested observer core, isolated persistence, Tab5 presentation/controls. Existing radio and reporters stay intact.
**Tech stack:** C++17, Arduino ESP32-P4, M5GFX, Preferences, Python serial tools; SVG/Pillow artwork.
**Spec:** ../specs/2026-10-20-runtime-observer-design.md

## Global constraints
- Only Blue0, replacement Green1, Orange6, Silver8 participate. Tab5 stays USB powered.
- >=90% start readiness;120s settling;<=3.30V for60s observation endpoint; never infer empty from silence.
- Preserve charger settings, flash partitions and old hot Green exclusion. No real race start in smoke test.

## Review focus
- Gateway loss: all peers unavailable, no false finish.
- Duplicate relays/stale packets: no fabricated observed time or slope.
- Reboot and clock wrap: invalidate rate continuity and interrupt restored active runs.
- Voltage estimates shift under USB/load: show ~ and suppress early ETA.
- Persistence full/corrupt: show unsaved/error rather than claiming durability.

### 1. Pure observation core
- [x] Write tests/runtime_observer_test.cpp, observe missing-header failure.
- [x] Implement firmware/FaceMonitor/RuntimeObserver.h: accept snapshots, track background/race curves, readiness, rates, gaps, low endpoint, restoration.
- [x] Run host suite including long synthetic races and malformed persistence.

### 2. Tab5 integration
- [x] Add RuntimeStorage.h, isolated checked NVS snapshot every300s; recovered active runs interrupted.
- [x] Add RuntimeView.h, tabs and deliberate start/stop confirmation, isolated Demo and numeric/CSV serial export.
- [x] Update main.cpp loop to sample independently of selected page, preserve face grid and source freshness. Add monitor_io capture helpers.
- [x] Compile Tab5 only; app-only install; verify four live sources, page/control flow, demo isolation, persistence readback and idle radio.

### 3. Physical-scale artwork and release
- [x] Source manufacturer outer dimensions, correct common-scale SVG family and PNGs, inspect visually.
- [x] Run independent code review and fix material findings.
- [x] Add screenshots, README controls/limits, versioned evidence and all new assets; package checks, commit/push private main and verify remote README.

## Execution record

19 host configurations and2 transport tests passed. Independent review findings were reproduced before repair: cadence jitter, stale reorder, fractional timing, first run interval, gap compaction and partial-key restore. Two app-only Tab5 installs validated checked NVS recovery using real charging data. Shared touch hit tests, waiting/start gate, isolated3hDemo, CSV and four live sources passed; physical finger gestures remain unconfirmed. No reporter reset, load/test start or unplugging. Final state Faces / WAITING / session0.

Ruling: use an explicit90%-ready arm and120s confirmation rather than sensing USB removal, because available four-board telemetry lacks a reliable common external-power flag. Background observations are not automatically labeled battery-only runtime. Blue gateway loss remains an explicitly censored observation; independent gateway is a future architectural step.
