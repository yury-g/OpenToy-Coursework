# Verification — November 15, 2026

## Installed pair

| Version | Physical assignment | Screen label | Existing application slot |
| --- | --- | --- | --- |
| Animatronic Eyes 1 | Blue Reverse TFT Feather, previously Battery Faces | EYES 1, cyan | ota_0 at 0x10000; capacity 0x2c0000 |
| Animatronic Eyes 2 | Reverse TFT Feather previously running Gravity Water | EYES 2, orange | ota_0 at 0x10000; capacity 0x160000 |

Each was independently identified through ESP32-S3 ROM as revision 0.2 with 4 MB physical flash and security flags 0. Both connected controllers returned Mini I2C Gamepad QT product 5743 at 0x50. Neither board matches the retired damaged Green identity. Factory identities and USB paths stay in private recovery records; USB paths are not identity checks.

Full 4 MB backups were hardware-digest verified before replacement. Blue's running old application changed 5,956 NVS bytes after the first snapshot; the guard stopped the first install without writing. A second full snapshot was taken and verified with that application stopped. Both snapshots remain preserved. The successful app-only writes were independently verified; every flash byte outside each sector-aligned write range was checked unchanged, including TinyUF2, partition table, OTA selection and filesystems.

## Automated checks

- The full upstream OpenToy host suite passed, including the new mapping/freshness tests.
- The standalone host suite passes: pupil containment/lid edges/eye travel, six distinct doodle expressions, spring settling and wraparound, framebuffer bounds, stale/failing input reset, stick directions, held layers, sticky expression edges, USB queue bounds/disconnect, and source digest preservation.
- All six mapped v2 presets match original settled frame hashes. Happy retains jaw 25; Surprised retains jaw 92. The standalone mapping corrected these defaults after the first live port, where both had defaulted closed.
- Both application builds succeed with core 2.0.17 and the documented pinned libraries. TinyUF2-compatible images use DIO, 80 MHz, 4 MB, maximum-revision field 0 and ELF digest offset 0xb0. Both real devices boot their selected existing application slots.
- On-device `!` pauses sampling for 1.5 seconds: telemetry shows stale input, neutral target, then fresh product-5743 input after automatic resumption. This is deliberate sampling interruption, separate from unplugging a physical cable.
- Both continued rendering and reading controllers during 20 seconds of repeated status requests with no host reads. The initial battery-enabled port measured approximately 29.3 frames/s for Eyes 1 and 27.8 for Eyes 2 over the interruption and drain interval. Final standalone builds repeated the checks: 704 frames in 24.012 s for Eyes 1 and 670 frames in 24.015 s for Eyes 2. Readbacks are kept privately; timing is instrumented, not an optical display measurement.
- Real stick motion and A/B/X/Y events were present in captured USB telemetry on both boards, with changing frame hashes and the matching expressions. Battery gauge readback worked on both; the small icon/percentage is at the top right. These are gauge estimates, not capacity measurements.

## Owner observations

The owner explicitly confirmed **both displays look right and respond well**, including the stick, expression buttons and held control layers. The owner also confirmed **both return to neutral and recover correctly after unplugging/reconnecting the gamepad cable**. Captured Eyes 2 telemetry independently shows missing-product/stale reports followed by product 5743 and fresh samples. Eyes 1's capture begins after its brief interruption, with one read error and subsequent fresh samples; the observed unplug behavior for Eyes 1 is the owner's confirmation.

These confirmations cover the battery-enabled Face v1/v2 port immediately before the standalone rename. The final standalone build changes screen labels to EYES 1/EYES 2 and restores the original v2 happy/surprised jaw defaults; its automated checks and boot readback are separate evidence. No new claim of complete physical qualification is inferred from compilation.

## Recovery records

Private local folder: `2026-11-15/face-feather-private` under the owner's Codex workspace. Its `RECOVERY.md` contains exact backup/image hashes, enrolled identities, flash layouts, build/installation commands and logs. No recovery binary, key, raw identity or device log is committed here. Source-only rebuilds are supported; automatic provisioning of an unknown student's board is not claimed.
