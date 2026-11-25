# RLCD verification — 2026-11-17

## v0.2.0 — native display and KEY menu

Created 2026-11-17; final firmware source `aab1bba`, integrating main `fd90e20`. Subsequent source-publication integration also preserves main `09281db` (README-only update; firmware unchanged). The owner explicitly reconnected the RLCD and authorized installation. Native USB identity, ROM chip/MAC, clear security flags and 16 MB flash capacity were freshly checked before each write; no other board was opened.

### Built and installed

- Native 400 × 300 analytic rounded/circular contours, black background, light eye shapes, dark 2x pupils and light header ink. Existing pose, independent anger/recovery, blinks, personalities and exact vendor controller initialization retained. Historical small bitmap renderer remains preserved; no spatial or temporal dithering.
- Exposed KEY/GPIO18 only: short reaction, 0.8-second hold Menu; short Next/hold Select; explicit Back, per-eye app actions, all three personalities, honest single-app/audio limitations, performance confirmation and a reachable restore gesture. BOOT/RESET are reserved; no power remapping.
- Calibrated four-sample ADC voltage every two seconds. Header and telemetry share formatting. Errors/invalid reads or age above five seconds become `--`; no percentage, presence or charging inference.
- Target and full shared Eyes host suites pass, including native circle area/one-pixel contours, pupils/lids/bounds, button bounce/stuck holds/release isolation/wrap, every menu/back path, performance restore, voltage conversion/stale/error/wrap and preserved upstream renderer/telemetry digests. Review found an installer optimization-mode safety gap; explicit startup rejection was added and tested before hardware access.
- Arduino-ESP32 3.3.10, Adafruit GFX 1.12.6, BusIO 1.17.4 build passes. Image 373,888 bytes; 55,916 bytes static RAM. Image checksum/hash, ESP32-S3/DIO/16 MB header checked. Normal C++ entry points avoid the historical Arduino preprocessing failure.
- Full live flash matched the expected prior installation before replacement. App-only sector range `[0x10000,0x6c000)` was written; device digests verified the image, erased padding and every byte outside the range. The two original backups, prior images/manifests and existing runtime coredump remain preserved privately. No partition table, bootloader, OTA data, NVS or asset overwrite.

### Live acceptance

Final live run passed on source `aab1bba`, 2026-11-17 12:52:46–12:54:17 America/New_York. Device reports EYES1 0.2.0, creation date 2026-11-17, app0 address, native renderer/black polarity, KEY-menu profile and 8 MB PSRAM. Synthetic checks traversed every menu page with Back/Return, entered/restored performance, selected all personalities, poked each eye and observed anger recovery. Serial resumed after ten seconds without host reads. These are software-driven input checks, not physical button assertions.

Over 90.049 seconds, frame evaluations advanced from 13 to 1,814 (approximately 20 Hz). Final totals were 931 transfers / 883 unchanged-frame skips. Maximum draw 42,952 µs including transfer; maximum SPI transfer 12,549 µs; heap remained 310,816 bytes. No optical-refresh or runtime-saving inference. Battery samples advanced from 1 to 46, calibration available, zero read errors, maximum read 296 µs. Node voltage was 4.138–4.151 V; the displayed text matched the same formatter used by the header. Nine spaced freshness checks stayed below 2.2 seconds age and followed the 3:1 pin conversion. Stale/invalid paths were host-tested, not forced on hardware.

After deliberately hiding the header, a freshly identity-checked hardware reboot restored the normal visible header and Curious/play defaults. Correct version/source and successful boot were queried again; no flash write was needed for this reset check. An initial v0.2.0 candidate booted but exceeded the 50 ms redraw budget in menus (80,415 µs). Final source skips obscured eye rendering and clears menu frame bytes directly; this cause/fix is checked by the same live timing assertion rather than weakening the limit.

### Owner observations and limits

Owner observed light background/enlarged pixels on v0.1.1 and requested this update. Trying all old buttons produced no confirmed response; the firmware label is not the physical QC V1/V2 label. The owner initially removed the supplied battery; on 2026-11-17, the owner subsequently reported CHG steadily on in the context of the supplied battery reinstalled and USB connected. Owner explicitly confirmed that EYES1 v0.2.0, light eyes/dark pupils/black background and the cleaner curves look right. Physical KEY/menu operation and a detailed readability check remain unconfirmed. Synthetic menu/poke checks do not establish a physical button test, and SPI transfer success cannot establish optical contrast or refresh rate.

Battery-node volts can exist while USB powers the charger node. Calibration corrects the ADC; independent voltage accuracy, SOC, charge current and full-charge state remain unverified. **Owner-observed hardware charging indication:** on 2026-11-17, with the supplied battery reinstalled and USB connected, the owner reported CHG steadily on. [Waveshare's onboard resources, item14](https://docs.waveshare.com/ESP32-S3-RLCD-4.2#onboard-resources) identifies CHG as the charging indicator and says it goes off when fully charged. This supports charging indicated by the hardware; it does not measure current/capacity or validate a percentage. Firmware cannot electronically read CHG in this build, so its `charge=unknown` remains correct. The earlier approximately4.14V is a voltage measurement, not proof of charging. An off indicator alone would not establish full charge without context. No charger settings are changed. Audio input/output, acoustic wake, sensors, other app ports and battery runtime remain outside this installed feature set; see [roadmap](../../docs/ROADMAP.md) and [compatibility](COMPATIBILITY.md).

## Historical v0.1.1 evidence

## Host and source evidence

- Current OpenToy base: `484a2fd`; shared family/status specifications read.
- Baseline and post-adapter existing Eyes host suites pass, including immutable upstream renderer/telemetry digest checks.
- RLCD adapter tests pass: all 120,000 pixels uniquely address 15,000 bytes; hand-checked corner packing, bounds guards, content viewport, debounce, independent button pokes, chord suppression/one-shot toggle, timer wrap and session header reset.
- Initial compile exposed an inherited GFX width-name collision; qualified canvas dimensions resolve it. Final v0.1.1 compilation passed: Arduino-ESP32 3.3.10, GFX 1.12.6, BusIO 1.17.4. Application binary 370,880 bytes, 56,916 bytes static RAM; source `16ef2b5`.

Review found interrupted/staggered chord issues. Regression tests reproduced each failure before fixes: continuous hold now restarts after interruption, and a 150 ms solo arbitration window allows a clean staggered chord.

## Hardware evidence

Fresh native USB replacement inventory and ROM identify a distinct ESP32-S3, matching the newly attached USB serial and excluding the other device chats. Original flash contains `board=waveshare-rlcd42`, landscape dimensions and monochrome/no-backlight diagnostics. Security flags are clear. Raw identity stays private.

Detected original layout: NVS 0x9000/0x5000; OTA data 0xe000/0x2000; app0 0x10000/0x330000; app1 0x340000/0x330000; SPIFFS 0x670000/0x180000; coredump 0x7f0000/0x10000. The table does not cover all physical flash: preserve the complete 16 MB device, including unpartitioned space. OTA record and app inspection select app0; app1 is blank. A compiled default partition table must never replace this layout.

Two complete 16 MB reads match by SHA-256 and each matches the device full-flash MD5. Partition-table checksum and OTA selection CRC independently verified. The application was replaced only within sector range `[0x10000,0x6b000)`. Device digests verified the image, erased tail padding, and every byte outside this range before reboot. Direct binary readback stalled over native USB; device-side digest verification succeeded. Exact physical QC revision is pending.

## Owner observations

Owner reports connecting the large non-backlit 4.2-inch display. No confirmation yet of this port's pixels, orientation, physical controls, reaction timing or header readability. Software/USB evidence cannot establish those visual results.

## Installation and live behavior

v0.1.0 passed build/host checks but failed live boot: Arduino prototype rewriting inserted recursive setup/loop calls. Disassembly and the panic trace established the cause. v0.1.1 places entry points in `Rlcd.cpp`; disassembly confirms normal calls and real firmware now boots. The failed build is explicitly superseded in CHANGELOG.md.

The failed boot wrote a runtime coredump at 0x7f0000. It was read, saved privately and verified before the second installation. All other non-app regions matched the original backup. The v0.1.1 installer preserved this immediate prewrite state outside the app; it did not erase the crash record. Therefore all non-app bytes match the original except that preserved runtime coredump. This distinction matters for exact original restoration.

Observed on v0.1.1: correct app/version/date/source and app0 address; 8 MB PSRAM; both header-toggle directions; labeled synthetic left/right pokes and recovery to happy; all three personality selections; advancing frame/blink counters; resumed telemetry after ten seconds without host reads. Physical button use remains separate from synthetic checks. A test-harness comparison initially selected an older queued report after the no-read interval; selecting the newest report corrected that test without firmware changes.

The panel driver has no readback, so `ready=1` and transfer counts prove successful software execution, not visible pixels. The hardware reported local battery-node voltage around 3.63 V and acknowledgments from the four expected I2C addresses (codec, audio ADC, RTC and environmental sensor). Those acknowledgments do not constitute audio playback or sensor-accuracy tests. Audio/radio remain off. No current draw or battery-runtime measurements were made.

Final live run: 716 frame evaluations over approximately 36 seconds; 284 transfers and 432 unchanged-frame skips. Largest reported draw time 26,951 µs, largest SPI transfer 12,556 µs; heap remained 309,816 bytes across samples. Local voltage ranged approximately 3.63–3.64 V. These are telemetry measurements, not a measured optical refresh rate. All live-check assertions passed after correcting queued-report selection.

An identity-checked hardware reboot after deliberately hiding the header restored the default-visible header in v0.1.1. No flash write was used for this check.
