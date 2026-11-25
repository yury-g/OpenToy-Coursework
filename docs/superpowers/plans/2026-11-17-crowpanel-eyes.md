# CrowPanel Eyes 1 implementation plan

> Execute inline with superpowers:executing-plans; the owner explicitly requested finishing setup without another planning approval.

**Goal:** Install native Eyes 1 on the enrolled DHE04209D V1.0 P4, preserving recovery and verifying live behavior.

**Architecture:** Reuse the existing Companion, personalities, reaction and original renderer without modifying other targets. Add an ESP-IDF P4 board adapter and a landscape LVGL presentation. XIAO, radios, motion and audio are unnecessary for this milestone.

**Spec:** `projects/crowpanel/README.md`, `docs/APP-STATUS-BAR.md`, `docs/DEVICE-FAMILY-ARCHITECTURE.md`.

**Tech stack:** ESP-IDF 5.4.2, EK79007, LVGL 8.4, esp_lvgl_port 2.6.1, GT911 on V1.0 I2C GPIO45/46 with reset40/interrupt42. Manufacturer source pinned in BUILDING.md.

## Constraints and review focus

- Use private ROM identity and verified 16 MiB backup; app-only write at the live factory slot. No partition/NVS/storage/C6 writes.
- Header has EYES1, canonical version/date and right-aligned unknown local battery. Boot visible; footer Status button hides/restores for this session.
- Reserve footer controls for Curious, Dreamy, Cheeky, ECO and Status. Pokes use the same viewport as rendering in both header states.
- Failures/stale touch release immediately; two contacts can poke both eyes; out-of-bounds contacts do nothing.
- Keep diagnostics bounded and request-only; no serial backpressure in the animation loop. ECO remains visible and changes frame cadence/backlight, with no battery-life claim.
- Exercise footer/face boundaries, timer wrap, touch failure, unchanged non-app flash and reboot defaults. Physical display/touch observations require the owner and remain separate from telemetry.

## Tasks

- [x] Write failing host tests in `projects/crowpanel/tests/presentation.cpp` for viewport inverse mapping, control/face separation, two contacts, stale/failure release and session header defaults. Implement the minimal `firmware/main/Presentation.h`; run existing Eyes host suite too.
- [x] Implement `firmware/main/Board.c/.h` using verified manufacturer timing/pins and official ESP-IDF drivers. Implement `App.cpp` using shared Companion and canonical `Metadata.h`, bounded UART reporting, fixed layout and documented controls. Add build-only helper which stages sources and artifacts outside Git, exact live partitions and pinned dependencies.
- [x] Build and inspect P4 image/revision/capacity; freshly reverify ROM/security/live flash against saved recovery; write only padded app sectors; verify written digest and all unchanged flash outside the write range before boot.
- [x] Capture live version, frames, touch/error freshness, synthetic independent reactions, all personalities, header/ECO transitions, input timeout, serial flooding and reboot defaults. Request one owner screen/touch check once running.
- [x] Obtain a fresh code review, resolve material findings, update matrix/revision evidence, refresh main and publish the checkpoint without binaries or private identifiers.

## Execution decisions and review

- Reused the existing behavior headers unchanged; a scoped compiler warning suppression permits their compact formatting without weakening checks on the new code.
- Fresh reviewer identified stale cached GT911 packets, dropped layout updates, deadline wrap and a zero-tick delay. Fixed the portable state issues with failing/passing regression tests and changed the loop to delay one real tick. Footer edges now work independently of held eye contacts.
- IDF generic touch I2C waits indefinitely; a register-only transport sets 20 ms transaction bounds while keeping the official GT911 decoder/reset sequence.
- Live 0.1.0 exposed expensive generic zoom (~2.5 fps); 0.1.1 uses tested native RGB565 expansion and optimized compilation. Both installation records stay private.

## Owner-directed smooth rendering follow-up

The owner visually saw enlarged pixels and requested native crisp curves. Version0.2.0 adds analytic native rounded-eye, pupil and lid coverage, using two bounded eye images instead of a large scaled framebuffer. Tests cover native edge coverage, doubled pupils, all personalities/eyes, partial and bottom lids, and bounds. The original pixel renderer remains untouched. Source-derived PNG/GIF previews are labeled scripted simulations and omit UI overlays. A fresh review of this follow-up found no blocking defect; live timing at both scales and all personas passed in final 0.2.2 (12.6–15.1 fps).

After the final installed checkpoint, release all serial handles and write the private hardware handoff for the owner-requested Copy-Face chat. Do not re-open the panel once handed over.

## Final qualification

Final 0.2.2 passes host, full app readback, live bench, post-run full-flash digest
and reset defaults. Owner reported smooth 0.2.0 looked great. Battery controller
bytes are plausible but unqualified: neutral BAT--, diagnostic candidate voltage,
unknown charging, no charger writes. Review required a total20ms polling budget
and refreshed time. Family common-menu migration and audio remain future work.
