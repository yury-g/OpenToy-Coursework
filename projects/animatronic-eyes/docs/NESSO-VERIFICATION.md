# Nesso touch verification — November 15, 2026

## Source and build checks

The full OpenToy suite and Animatronic Eyes tests pass. Touch tests cover both eyes, the top status strip, held/repeated pokes, quarter-second reopening, monotonic six-second recovery, timer wrap and exact original v1 expression endpoints. Presentation tests cover all four gravity rotations, steady-pose delay, flat/shake rejection, both portrait/landscape hit maps, bounded anger-only jitter and calm immobility.

Build v1.1.0 succeeds with Arduino-ESP32 3.3.10, M5Unified 0.2.18 and M5GFX 0.2.25: 425,974 bytes reported program usage; 19,256 bytes global data. The 426,128-byte image uses ESP32-C6/DIO/80 MHz/16 MB headers. Feather code and builds are unchanged.

## Backup and installation

ROM independently identified the connected Nesso as ESP32-C6 revision 0.2, 16 MB flash, security flags 0. Two fresh 16 MB reads match; the snapshot also matches the full hardware digest. Its previous app exactly matches the accepted Gravity Water v0.3.0 image. The selected original app0 is at 0x10000, capacity 0x300000.

The default read transfer repeatedly stalled at a live block whose SLIP encoding is 4160 bytes, exactly divisible by USB's 64-byte packet size. Cable/port changes did not fix that framing case. A 1024-byte-block reader with acknowledgments completed both full reads and digest checks. Failed read logs are retained; none of those attempts wrote flash.

Both v1.0.0 and v1.1.0 app-only installations passed independent image verification and unchanged-region checks. Final written sectors span 0x10000–0x78fff. Bootloader, partition table, OTA state, NVS, second app, filesystems and coredump bytes outside that range remain unchanged. Raw identities, backups, binaries, installer and logs stay in the private local `2026-11-15/animatronic-nesso-private` directory.

## Live checks and owner observations

The initial touch build passed simulated left/right pokes, reopening and six-second mood recovery. A sampling pause expired input and resumed; 20 seconds of no-host-read USB pressure did not stop rendering (731 frames over 24.116 seconds including drain, about 30.3 fps). Battery telemetry and the indicator worked.

The owner explicitly confirmed both real eyes react correctly. A subsequent physical capture recorded 11 real poke events, correct left/right masks and changing face pixels, with zero read errors in that capture. The earlier synthetic/pressure run had transient I2C read failures; these are treated as release, not cached contact. No zero-error claim is made for all operation.

The v1.1.0 orientation/jitter update passed synthetic left/right pokes, six-second recovery, fresh IMU readings, sampling pause/recovery, and a 20-second no-read USB pressure test: 728 frames over 24.030 seconds including drain (30.3 fps). The owner explicitly confirmed the face stayed upright in all four directions, the correct eye closed, and angry trembling faded away. Visible trembling is rendered eye motion; no physical vibration motor is driven.

## v1.2.0 — larger eyes, continuous turns and separate reactions

At the owner's request, the next revision replaces cardinal switching with continuous gravity-angle easing. Tight 90×36 eye bounds include the maximum jitter. At cardinal angles the logical scale increases from 1.875 to 2.6 in landscape, and about 1.05 to 1.433 in portrait. All intermediate angles fit inside the fixed 240×135 framebuffer. Touch uses the displayed transform, including the moving status strip.

The directly poked eye gets full anger and a 6–8 second recovery; the other eye responds more mildly, briefly flinches, and normally recovers in about 3 seconds. Reopening and trembling timing differ between eyes. Original renderer files remain unchanged.

Host coverage includes continuous full turns and the ±180-degree seam, flat/invalid/stale input, clock wrap, inverse touch mapping at every integer degree, raster guard bytes and clipping bounds, asymmetrical reactions and recovery, and original expression endpoint equality. The full OpenToy host suite passes. Arduino build v1.2.0 reports 436,158 program bytes and 19,648 bytes of global data; the application image is 436,304 bytes. The bounded app-only write and independent image verification passed, with all flash outside 0x10000–0x7afff verified unchanged.

Live v1.2.0 diagnostics passed synthetic left/right closure, unequal mood levels, the neighbor reaching happy first, full recovery within eight seconds, changing angry frame hashes, fresh IMU readings, sampling pause/recovery and 20 seconds of no-read USB pressure. Rendering continued at 731 frames over 24.123 seconds including drain (30.30 fps). Observed peak preparation was 9,233 µs and display push 15,548 µs. The final test capture had zero touch read errors. The owner separately confirmed v1.2.0: the larger eyes turn smoothly, stay on screen, and react differently, with the poked eye angrier for longer. The accompanying physical capture records real pokes, intermediate rotation angles and unequal recovering eye moods; these observations are separate from the synthetic diagnostic run.


## v1.3.0 — idle blinks and glances, animated device family

Nesso now resumes autonomous play 900 ms after the last face touch. Both eyes glance toward related targets with small offsets and different response delays. They blink on separate schedules, with different closure/reopening durations. Any face poke immediately suppresses idle motion, leaving the existing poke reaction in charge.

The new host checks cover a minute of idle motion, full blinks from both eyes, unequal blink phases, bounded smooth gaze, touch interruption, restart delay, composition with reaction lids and timer wrap. The complete OpenToy host suite passes. The build reports 437,596 program bytes and 19,776 bytes of global data; the application image is 437,728 bytes. App-only installation and independent verification passed; flash outside 0x10000–0x7afff remains unchanged.

Live diagnostics confirm changing idle gaze, separate blink counters/phases, immediate synthetic-poke interruption, idle resumption, unchanged asymmetric anger/recovery, fresh IMU readings and sampling-pause recovery. During 20 seconds of USB no-read pressure, rendering continued at 728 frames over 24.024 seconds including drain (30.30 fps). The final capture showed zero touch read errors, peak draw 9,320 µs and display push 15,598 µs. The owner separately confirmed the physical idle check: natural-looking glances, slightly staggered blinks, and poking taking over immediately.

The repository includes two 24-second looping animations: the Nesso/Feather Eyes 1/Feather Eyes 2 family and a closer Nesso behavior tour. They use actual firmware behavior/renderers with scripted inputs; they are not physical screen recordings. Status strips are omitted. Generation verifies both full idle blinks, unequal moods, output duration and looping; `animation-manifest.json` records source and GIF hashes. Feather firmware remains unchanged and does not gain Nesso's autonomous behavior.


## Subsequent Feather port

The same idle/reaction code was subsequently ported to Feather Eyes 1 with QT gamepad input. That installation, controls and checks are recorded in [Feather verification](VERIFICATION.md#eyes-1-companion-port-from-nesso). The Nesso v1.3.0 installation itself is unchanged.


## Nesso v1.3.1 — matching 2× pupils

The Nesso pupil radius is now 8 rather than 4 logical pixels, matching the owner-approved Feather size. The existing expression/lid masks clip the pupil, and fully closed eye lines remain intact. Touch, idle timing, per-eye reactions and continuous orientation are unchanged. The full OpenToy host suite passes. Both family and Nesso-specific animation previews are regenerated from the new renderer. The build reports 437,680 program bytes and 19,776 global bytes; the compatible image is 437,824 bytes. The app-only write and independent digest verification pass, with all flash outside 0x10000–0x7afff unchanged.

Live readback identifies `nesso-eyes-1.3.1`, fresh touch/IMU samples, changing idle gaze and separate blink counters.


## Nesso v1.3.2 — raised upper lids

The happy upper-lid cutoff moves from logical y=16 to y=8 and lifts further during upward gaze when needed to avoid covering the top of the larger pupil. Angry masks and full closures remain unchanged. Both Eyes 1 platforms share this adjustment; the complete host suite and firmware builds pass. The build reports 437,706 program bytes and 19,776 global bytes; the image is 437,840 bytes. The app-only write at 0x10000–0x7afff and independent image/unaffected-region checks passed. Live readback confirms v1.3.2, fresh touch/IMU readings, idle gaze and separate blinks.
