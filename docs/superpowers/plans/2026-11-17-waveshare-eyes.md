# Waveshare Eyes implementation plan

Goal: deliver Eyes 1 with the existing companion behavior on the enrolled round Waveshare V2, then document the catalog's compatibility.
Architecture: unchanged shared eye renderer and Companion, target-only round projection/touch/controls, common canonical metadata, bounded hardware adapters. Arduino ESP32 3.3.10; native host C++ tests.
Spec: owner's dedicated-device handoff and docs/APP-STATUS-BAR.md (read in coordinator checkout pending publication).

Constraints: app-only write into verified existing selected slot; preserve all other flash and fresh private backup; no microphone/radio/cloud; amplifier off between brief quiet sounds; no other device access; source/records stay in OpenToy.

- [x] Round presentation and controls: failing host tests for circular bounds, matching poke mapping, malformed/failed touch releases, hold cancellation/retrigger, header bounds and canonical identity. Implement Round.h, Status.h, Metadata.h, Sound.h. Preserve existing behavior tests. Draft sound test checks capped samples, fades and suppression.
- [x] Hardware adapter/build: vendor attributed ST77916 case2 table and driver, checked shared I2C, 360x360 strips with DMA completion before reuse, CST816 polling, ES8311 output only with capped PCM and timed amplifier off, battery voltage. Build outside Git. Test init failures and bounded diagnostic handling via host logic/build/live telemetry.
- [x] Installation: fresh ROM/security/size/layout check; full read and hardware digest; verify factory snapshot; inspect image/fit; sector-aligned app-only write; verify image and unchanged ranges. Keep script and outputs private.
- [x] Live verification: boot/frames/display completions, touch transactions, left/right synthetic independence, all personas, hide/restore, mute, bounded audio, USB backpressure and reboot default. Physical appearance/touch/audio remain explicitly pending owner observation where unavailable.
- [x] Catalog/docs: source-rendered labeled animation, Waveshare guide and complete catalog matrix, public sanitized verification and history, family links. Fetch coordinator main, reconcile concurrent edits, run full checks, independent review before publication.

Review focus: stale touch holding a poke; control hold sliding in from an eye; DMA timeout reusing memory; missing peripherals blocking animation; audio output/microphone power isolation; flash changes outside app. Do not equate telemetry with physical observations.

Execution: autonomous inline as requested. Review once before installation/publishing. No new approval gate; installation and docs updates explicitly authorized.

Execution checkpoint: Waveshare1.0.2 installed and automated tests pass; owner observations remain pending, explicitly tracked in WAVESHARE-VERIFICATION.md. Initial full ROM backup plus hardware digest preserved; later verified snapshots reused only after a new whole-chip digest. Live findings led to integer projection,60-row strips and correct codec defined-bit validation. Final independent review reported no actionable regressions.
