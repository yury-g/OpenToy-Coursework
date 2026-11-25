# Feather verification

**Current assignment:** the connected former Eyes 2 / Gravity Water Feather is now running the Eyes 1 companion port described below. Blue’s earlier Eyes 1 installation is unchanged. The following pair is historical.

## Earlier installed pair

| Version | Physical assignment | Screen label | Existing application slot |
| --- | --- | --- | --- |
| Animatronic Eyes 1 | Blue Reverse TFT Feather, previously Battery Faces | EYES 1, cyan | ota_0 at 0x10000; capacity 0x2c0000 |
| Animatronic Eyes 2 | Reverse TFT Feather previously running Gravity Water | EYES 2, orange | ota_0 at 0x10000; capacity 0x160000 |

Each was independently identified through ESP32-S3 ROM as revision 0.2 with 4 MB physical flash and security flags 0. Both connected controllers returned Mini I2C Gamepad QT product 5743 at 0x50. Neither board matches the retired damaged Green identity. Factory identities and USB paths stay in private recovery records; USB paths are not identity checks.

Full 4 MB backups were hardware-digest verified before replacement. Blue's running old application changed 5,956 NVS bytes after the first snapshot; the guard stopped the first install without writing. A second full snapshot was taken and verified with that application stopped. Both snapshots remain preserved. The successful app-only writes were independently verified; every flash byte outside each sector-aligned write range was checked unchanged, including TinyUF2, partition table, OTA selection and filesystems.

## Automated checks

- The full upstream OpenToy host suite passed, including the new mapping/freshness tests.
- The named host suite passes: pupil containment/lid edges/eye travel, six distinct doodle expressions, spring settling and wraparound, framebuffer bounds, stale/failing input reset, stick directions, held layers, sticky expression edges, USB queue bounds/disconnect, and source digest preservation.
- All six mapped v2 presets match original settled frame hashes. Happy retains jaw 25; Surprised retains jaw 92. The named mapping corrected these defaults after the first live port, where both had defaulted closed.
- Both application builds succeed with core 2.0.17 and the documented pinned libraries. TinyUF2-compatible images use DIO, 80 MHz, 4 MB, maximum-revision field 0 and ELF digest offset 0xb0. Both real devices boot their selected existing application slots.
- On-device `!` pauses sampling for 1.5 seconds: telemetry shows stale input, neutral target, then fresh product-5743 input after automatic resumption. This is deliberate sampling interruption, separate from unplugging a physical cable.
- Both continued rendering and reading controllers during 20 seconds of repeated status requests with no host reads. The initial battery-enabled port measured approximately 29.3 frames/s for Eyes 1 and 27.8 for Eyes 2 over the interruption and drain interval. Final named builds repeated the checks: 704 frames in 24.012 s for Eyes 1 and 670 frames in 24.015 s for Eyes 2. Readbacks are kept privately; timing is instrumented, not an optical display measurement.
- Real stick motion and A/B/X/Y events were present in captured USB telemetry on both boards, with changing frame hashes and the matching expressions. Battery gauge readback worked on both; the small icon/percentage is at the top right. These are gauge estimates, not capacity measurements.

## Owner observations

The owner explicitly confirmed **both displays look right and respond well**, including the stick, expression buttons and held control layers. The owner also confirmed **both return to neutral and recover correctly after unplugging/reconnecting the gamepad cable**. Captured Eyes 2 telemetry independently shows missing-product/stale reports followed by product 5743 and fresh samples. Eyes 1's capture begins after its brief interruption, with one read error and subsequent fresh samples; the observed unplug behavior for Eyes 1 is the owner's confirmation.

These confirmations cover the battery-enabled Face v1/v2 port immediately before the named rename. The final named build changes screen labels to EYES 1/EYES 2 and restores the original v2 happy/surprised jaw defaults; its automated checks and boot readback are separate evidence. No new claim of complete physical qualification is inferred from compilation.

## Recovery records

Private local folder: `2026-11-15/face-feather-private` under the owner's Codex workspace. Its `RECOVERY.md` contains exact backup/image hashes, enrolled identities, flash layouts, build/installation commands and logs. No recovery binary, key, raw identity or device log is committed here. Source-only rebuilds are supported; automatic provisioning of an unknown student's board is not claimed.


## Eyes 1 companion port from Nesso

The connected Reverse TFT Feather was independently identified through ESP32-S3 ROM and observed running Eyes 2 with a fresh product-5743 QT gamepad. A new 4 MB stopped snapshot matched its hardware digest. Its existing active ota_0 remains at 0x10000 with capacity 0x160000; no bootloader, partition, OTA, NVS or filesystem replacement is used.

Eyes 1 reuses Nesso's Reaction, Idle and Presentation code for larger fitted eyes, automatic glances, separate blink schedules, per-eye anger, trembling and recovery. Existing stick, modifier and A/X/Y controls remain. B makes both eyes angry then recovers; Select + X/Y pokes the left/right eye. Controller activity interrupts idle immediately; missing/stale input clears to neutral. The Feather stays in landscape: it has no built-in touchscreen or motion sensor.

The complete OpenToy host suite passes, including new companion tests for idle/takeover, targeted pokes, separate recovery, presets, manual blink/wink layers, disconnection/reconnection and full eye-shape travel within the screen. Both Feather versions compile with the pinned 2.0.17 toolchain. Eyes 1 reports 339,329 program bytes and 34,048 global bytes. The Eyes 2 code path retains its original controls and rendering. The compatible Eyes 1 image is 339,696 bytes. Its app-only write at 0x10000–0x62fff passed independent image verification; every byte outside that sector-aligned range matched the fresh snapshot. Live checks passed fresh product-5743 input, autonomous gaze and separate blink counters, synthetic left/right pokes and recovery, deliberate stale-input neutral/recovery, and 20 seconds of USB no-read pressure. Rendering advanced 646 frames over 23.941 seconds including drain (26.98 fps). The final capture showed zero controller read errors; peak drawing was 7,713 µs and display push 28,546 µs. The owner confirmed the physical stick/idle, Select + X/Y poke controls and unplug/reconnect checks all work and recover correctly.

The family animation is regenerated from the actual companion renderer and shows idle/pokes on both Eyes 1 devices. The unchanged Nesso firmware remains installed separately. New private backup/install/verification artifacts are in `2026-11-15/feather-ani-companion-private`, outside Git; older recovery records remain intact.


## Feather Eyes 1.1.2 — larger pupils

At the owner’s request, Feather Eyes 1 pupil radius is now 8 rather than 4 logical pixels, doubling the geometric diameter. Existing eye/lid masks clip the pupils and fully closed eye lines remain intact. Nesso and Eyes 2 pupil rendering are unchanged. The family animation uses the adjusted renderer. The full OpenToy host suite and both Feather builds pass. Eyes 1 reports 339,465 program bytes and 34,048 global bytes; the compatible application is 339,824 bytes. The app-only write and independent image verification passed with all flash outside 0x10000–0x62fff unchanged.

Live readback identifies `feather-eyes-1.1.2`, fresh product-5743 controller input, active idle play and changing frame hashes after the final update.


## Feather Eyes 1.1.3 — raised upper lids

The happy upper lids now sit higher, using the same pupil-clearance calculation as Nesso v1.3.2. Upward glances lift the lid as needed, while angry masks, full closures and controller behavior are unchanged. The complete host suite and both Feather builds pass. The build reports 339,517 program bytes and 34,048 global bytes; the compatible image is 339,888 bytes. Its app-only write at 0x10000–0x62fff passed independent image verification and unchanged-region checks.

Live readback confirms `feather-eyes-1.1.3`, fresh product-5743 gamepad input and changing idle frames after the raised-lid update.
