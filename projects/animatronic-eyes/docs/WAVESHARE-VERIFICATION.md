# Waveshare Eyes verification — November 17, 2026

## Installed 1.1.0 checkpoint — common menu and curved header

Installed November 17, 2026 on the same enrolled V2 round board. Firmware source/build identifier **`8bb4430`**, app **Eyes 1.1.0**, created **2026-11-17**. Host rendering now curves app/version/date/voltage/battery-icon glyphs around two circular baselines, as requested by the owner. The visible Menu replaces hidden mute/personality holds; [current controls](../WAVESHARE.md) describe the migration.

| Feature or check | Result and limit |
| --- | --- |
| Full host suite and independent review | Pass, including release inside/outside, unknown release cancellation, drag-out/reentry, failed/lost input, fresh-contact gating, restore timing/wrap, all menu pages, bounded audio and non-overlapping round header/Menu. Shared original renderer hashes unchanged. Review findings were fixed and rechecked. |
| Builds | Waveshare plus both existing Feather versions pass. Target: 429,833 program bytes, 73,360 static RAM bytes; application image 429,968 bytes. |
| Identity, recovery and install | Fresh USB/ROM identity/security/flash/layout checks pass. A fresh whole-chip hardware digest verified the reconstructed current 16 MiB snapshot before reuse. App-only write `[0x10000, 0x79000)`; app readback and **all flash outside the write range unchanged** verified before reboot. |
| Menu/settings on actual firmware | USB-injected taps exercise the same release path: root pages, Apps, all personalities, mute, both volume settings, both brightness settings, performance reminder/entry, restore Menu, Controls, About and Back pass. This is synthetic input, not physical finger acceptance. |
| Animation/display | 721 frames / 4,332 transfers in final capture; zero display errors; maximum draw 56,903 μs. About **19.6 frames/s** over the play-mode USB-backpressure interval. Menu pages render faster; mixed-mode cadence is not reported as play cadence. |
| Touch/I2C | 691 polls, zero errors; no physical touch events captured in automated checks. Host tests cover CST816 lift coordinates and cancellation. Physical tapping/restore alignment remains pending. |
| Audio | Three bounded plays, zero errors, amplifier high during a pulse then off; muted poke did not add a play. Soft ≤450 PCM peak, Quiet ≤900, original fades/cooldown/independent off timer preserved. Audible level remains owner-check pending. |
| Stale input and USB | 1.5-second sampling pause cancels play input and recovers; animation continues through 2,048 requests and 10 seconds without draining USB. |
| Reboot/session defaults | ROM-identified reboot restores Play, visible curved header, Curious, unmuted Quiet volume, normal brightness, and zero boot sound plays. |
| Battery, capture, radios | Raw battery input still zero → `--`, no invented percentage. Microphone and radios remain off. Sound wake remains roadmap only. |
| Physical acceptance | Owner reconnect confirmed. Physical Menu tap, curved-text readability, eye alignment, sound and battery-only operation are still pending owner observation. Source previews are not photographs. |

The prior release's evidence below remains historical. No other USB device was opened during this migration.

## Previous 1.0.2 checkpoint (superseded)

| Item | Evidence |
| --- | --- |
| Target | Enrolled Waveshare ESP32-S3-Touch-LCD-1.85C-BOX-EN with V2 ES8311 audio and case2 ST77916 panel |
| App / version / creation date | `animatronic-eyes-1` / **1.0.2** / **2026-11-17**, from canonical target Metadata.h and live status |
| Firmware source | `9d73f83` (plus unchanged build helper/shared source); build identifier `ws102-candidate` identifies the binary that passed these checks |
| Toolchain | Arduino-ESP32 3.3.10; ESP32-S3, DIO/80 MHz/16 MB, OPI PSRAM, hardware USB CDC |
| Program footprint | Arduino reports 425,681 program bytes and 37,320 static RAM bytes; application fits the original 3 MiB factory slot |
| Installation | Application only at `0x10000`; written sector range `[0x10000, 0x78000)`; original bootloader/partition/NVS/model/test data retained |
| Recovery | Original full 16 MiB factory image and factory application retained privately; original and current snapshots hardware-digest verified. Full restore has not been exercised. |
| Current physical observations | **Pending owner check**: screen orientation/readability/colors, physical eye-poke alignment/hold controls, sound level/quality, battery switch/connection |

The application runs on the enrolled device. This is not a claim that the owner has visually or audibly accepted it, or that every other OpenToy app now runs on this board.

## Installation checks

Native macOS USB access and ROM factory identity were checked before every write. Security state, physical flash ID, original bootloader/table and complete partition layout were verified; there is one factory application and no OTA selector. The image's chip/header/revision range, size, checksum and SHA256 digest were checked before USB mutation.

A fresh ROM read of all 16 MiB matched its hardware MD5 before the first installation. Later application-only iterations reconstructed the exact preceding verified snapshot, then compared it with a **new whole-chip hardware digest** before reuse. A mismatch would require a new full read. Each run saved its current complete snapshot and the application privately.

Only the app's sector range was erased/written. Bytes after the image within its final sector were preserved. Each installation verified the written image, read it back, and compared hardware digests over **all flash outside the write range** before boot. The factory recovery data, private scripts, identities, image hashes/binaries and raw logs are excluded from this repository.

The faster stub backup attempt failed before any flash write; the verified ROM path was used instead. A subsequent pre-write digest attempt needed the explicit 16 MiB ROM flash-size parameter; it also stopped before writing. Neither attempt weakened the verification checks.

## Checks and their limits

| Check | Result | What it establishes / limit |
| --- | --- | --- |
| Full `tools/check.py` | Pass, including independent reviewer runs | Existing tests plus round bounds, touch decode/release, personas, hold cancellation/wrap, hidden header, battery formatting, bounded sound/retrigger and codec shutdown-bit validation. Original renderer/telemetry digests unchanged. |
| Waveshare build | Pass | Target compiles with pinned toolchain. |
| Existing Feather Eyes 1 and 2 builds | Both pass; no devices opened | Build regression check using existing 2.0.17 configuration and per-invocation native ctags/esptool overrides. Initial auxiliary-tool architecture failures were resolved without modifying the shared legacy installation. |
| Round presentation | Host bounds pass; generated compositor image inspected | All lit eye pixels remain inside the circle; header fits safe bounds. Source animation is not a hardware recording. Only fixed upright orientation is supported. |
| Display transfer/live cadence | 489 frames / 2,940 completed strip transfers, zero transfer errors; **17.26 frames/s** across the sampled stress interval | DMA activity and running animation confirmed. Largest measured draw 63,467 μs. Does not prove actual LCD colors/orientation or physical readability. |
| Touch bus | 462 successful polls, zero read errors in final capture | CST816 transaction path runs. **No physical touch events captured** in this automated test. Failed-read and invalid-coordinate release are host-tested. |
| Independent reactions | Synthetic `L`/`R` on actual firmware pass | Directly poked eye has stronger negative mood than its neighbor; separate flinch/recovery appears in telemetry. Not a physical poke test. |
| Idle personalities | `1`/`2`/`3` live selection; gaze/hash/blink counters progress | Curious, Dreamy and Cheeky use the existing shared behavior. Host tests cover their distinct timing. |
| Header/performance | Live hide and restore pass | Same eye geometry/mapping with metadata hidden. Hold-zone semantics are host-tested; physical holds remain pending. |
| Session defaults | Enrolled ROM-checked reboot pass | Visible header, Curious, unmuted state and zero sound plays restored; no boot chirp. |
| Audio | Codec ready; documented ADC/PGA shutdown readback `0x6a`; seven bounded plays, zero errors | TX DMA path runs; amplifier GPIO observed high and then low. Muted poke did not increase play count. Waveform is host-checked, 80 ms, ≤900 peak, faded, ≥500 ms retrigger interval; separate 110 ms off timer. Audible level/clicks not measured. |
| Sampling pause | 1.5-second diagnostic pause/recovery pass | Fresh flag becomes false then true; pause explicitly releases held control. Physical I2C disconnection was not induced. |
| USB backpressure | Animation continued through 2,048 request bytes and ten seconds without draining USB | Request-only bounded reporting does not stop the face. Heap remained 270,716 bytes in the captured reports. |
| Battery | ADC input returned 0 mV; header displays `--` | No fabricated percentage or charging state. Battery connection/switch state, calibration and battery-only runtime remain unverified. |
| Microphone/network | No RX channel/DIN, no ES7210 setup, ES8311 capture path powered down; no radio initialization | Source/codec state checked. No microphone, Wi-Fi, Bluetooth or cloud voice feature enabled. |

The preserved factory partition table has no core-dump partition; the framework prints that boot warning. No crash/reset was observed during the bounded live test. The warning was not “fixed” by changing recovery layout.

## Dated revisions during this port

- **1.0.0 / `b2df2b8`:** first app-only boot; host and compile checks passed, display/touch/status/personas responded. Live cadence was ~5.4 fps and audio readiness was false. Superseded.
- **1.0.1 / private diagnostic build:** integer projection improved cadence and exposed codec readback `0x7f` rather than `0xff`; battery input was zero. Reserved-bit equality was the cause of rejected audio readiness. Superseded; not advertised as accepted sound support.
- **1.0.2 / `9d73f83`:** uses the documented capture-off register default and defined-bit check, larger DMA strips, silent codec settling, current audio availability diagnostics and immediate hold release during paused sampling. Passed the final live checks above. Physical acceptance remains pending.

An independent review found no remaining actionable regression in the final changes or the snapshot-reuse verification. [Controls and build](../WAVESHARE.md) · [Complete device compatibility matrix](../../waveshare-1.85c/README.md).
