# Nesso bench · November 15, 2026

**Installed and playtested:** Gravity Water v0.3.0, with thermal controls and PWM audio.

**Previously accepted baseline:** Gravity Water v0.2.0, 400 visible blue beads, on the
physical Nesso N1. Work is isolated on `feature/gravity-water`, based on Nesso
commit `7196c33`. The original Face chapter and `feature/nesso-n1` are unchanged.

## Thermal controls · v0.3.0

The November 15 follow-up adds quiet rest, hold-to-boil on KEY1/BtnA, fast heat
decay on release, energy-based blue-to-red coloring, shake scattering, fingertip
repulsion and procedural sound. The owner confirmed boiling, colors and cooling
on the physical device. On November 15, the owner also confirmed the final PWM
sound is audible ("Yes, I hear it") and both finger dragging and shaking move
the particles promptly ("Yes, both feel responsive"). Earlier particle direction
acceptance remains valid.

The initial integration exposed a display stall also reproducible with the exact
saved v0.2.0 image. USB-JTAG located the loop task at M5GFX `Bus_SPI::writeBytes`,
waiting for the DMA output FIFO at line 744. The display now bypasses that DMA
branch using SPI FIFO output, and transfers only old/new occupied particle rows.
Touch's display-sharing flag is disabled because it uses a separate I2C bus from
the LCD's SPI bus. FIFO output still contains library hardware waits; this is a
workaround for the observed failure, not universal fault recovery.

The original M5 speaker path also stalled: JTAG showed its task waiting inside
`i2s_channel_write` while the producer waited for its queue. Sound now uses native
LEDC PWM on GPIO11. A 2 kHz timer callback averages four 8 kHz synthesis samples;
the PWM carrier is 31.25 kHz. The callback exclusively owns its LEDC channel and
uses direct HAL register setters, with no audio queue, allocation or driver wait.
Both signal amplitude and PWM midpoint transition smoothly. Timer skip behavior
limits backlog; software timing cannot by itself establish audible quality.

The first 8 kHz callback implementation reduced active drawing to around 18fps.
The final lower-overhead callback measured a 23µs maximum in its initial run,
well below its 500µs output interval. The initial final-build run delivered 23.5–38.3fps during settling with the board
sideways, 195.8–198.4 fresh motion samples/sec and 57.9–60.5 physics steps/sec.
A subsequent USB capture had 54 reports through 135 seconds uptime: 30.7–30.9fps
at rest, 195.4–199.3 motion samples/sec, 59.5–60.4 physics steps/sec, zero reported
audio callback gaps over 1.5ms, and free heap stabilizing at 328,632 bytes. No
panic, watchdog, brownout or stale-sensor report appeared. This is a short bench
run, not endurance proof. Final sound audibility and touch/shake responsiveness
are owner-confirmed physical observations, separate from these counters.
The earlier all-controls run recorded four button presses, eight touches and
110 shake events, but its audio driver was stalled and it is not evidence of
final combined audio/visual performance.

The final firmware compiles to 441,278 bytes of code and 32,216 bytes static RAM.
Its exact image is independently verified against app0 after the final write.
The previously accepted v0.2.0 binary remains saved privately, as does original
Face recovery. Raw hardware identity, binaries and logs stay outside Git.

Host validation passes with AddressSanitizer and UndefinedBehaviorSanitizer:
exact stationary positions after settling; heating and rapid cooling; touch/
shake/tilt wake; all four gravity directions; 3,600 steps of simultaneous controls;
bounded PCM; smooth PWM startup/shutdown; and partial-frame output identical to
full frames over 600 steps with heat, touches and bursts. Independent review found
no remaining actionable issues after the PWM ramp and transfer changes.

## Baseline owner observations

The owner clarified that the intended aesthetic is individual water/sand-like
particles, using [UnityCoder's particle demo](https://unitycoder.com/blog/2013/04/12/water-particles-unity/)
as a visual reference. The connected polygon surface was superseded.

The first mappings were wrong. The decisive observation was: Nesso standing on
its bottom edge, battery bar correctly oriented, particles collecting on the
right. Fresh acceleration in that pose was approximately `(0.996, 0.005, 0.003)` g.
The sensor axes are rotated relative to the display. The final Nesso-only mapping
is **screen-down = `(ay, ax)`** for fixed portrait rotation 0.

After that change, the owner confirmed the particles collect at screen-bottom
when upright. After the final speed adjustment, the owner confirmed that the
look and direction are right during gentle left-and-right tilt. These are
physical observations, distinct from the timing below. The battery indicator's
normal appearance was also confirmed; full and nearly-empty states were not.

## Baseline particle timing

A USB capture covered **34.2 seconds** between its first and last reports, with
18 reports and 1,449 completed frames at the last report. The acceleration data
changed during this run, including the owner's motion check. The first five
seconds were excluded from the rate ranges. This is a short bench run.

| Quantity | Measured result |
| --- | --- |
| Fresh acceleration samples | 194.9–198.3 per second |
| Completed screen frames | 42.1–42.7 per second |
| Actual physics steps | 59.8–60.4 per second |
| Sensor read maximum | 0.854 ms |
| Fresh-sample gap, minimum / maximum | 3.962 / 19.971 ms |
| CPU simulation + drawing, average / maximum | 9.581 / 13.271 ms |
| Physics within drawing, average / maximum | 3.956 / 6.893 ms per rendered frame |
| Screen transfer, average / maximum | 13.228 / 14.185 ms |
| Sample age at completed transfer, average / maximum | 24.212 / 35.028 ms |
| Frame start gap, maximum | 30.360 ms |

Averages and extrema are from the last complete report and include startup.
The acceleration configuration is nominally 200 Hz; polling, scheduling and
periodic battery reads slightly reduce delivered samples. Only fresh accelerometer
bits increment the counter. The simulator targets 60 steps/sec; the actual step
counter confirms that target was met in this run. It limits catch-up to three
steps so overload cannot create an unlimited backlog.

No stale-sensor, panic, watchdog or brownout messages were seen in this capture.
USB output has zero transmission-wait timeout and a 2 KiB buffer. **These are
software timestamps, not motion-to-photon measurements.** Sensor group delay,
direction/strength filtering and LCD scanout are outside the reported sample age.

## Build, flash and recovery evidence

- Native macOS USB access; ROM verified the expected ESP32-C6 revision v0.2 and
  factory identity. Identity is retained privately, not published here.
- Two private 16 MiB original factory reads match. The live partition table and
  OTA selection matched that saved original; sequence 1 selects app0 at `0x10000`,
  capacity `0x300000`.
- Before replacement, a fresh read of installed Nesso Face v0.1.1 matched its saved
  working app byte for byte. Both are preserved privately.
- All writes targeted only app0, with esptool's write verification. A separate
  final `verify-flash` matched the installed image digest. Boot telemetry identifies
  v0.2.0 at `0x10000`, a 135 × 240 display, and BMI270 at I2C `0x68` with acceleration
  configuration `0xa9` and gyro configuration `0xe9` verified by readback.
- Built with Arduino-ESP32 3.3.10, M5Unified 0.2.18 and M5GFX 0.2.25. Compiler reports
  436,276 bytes code and 30,296 bytes static RAM; the complete application image is
  436,416 bytes. Framebuffer and task stacks are additional runtime allocations,
  with allocation success checked.
- The private Face restoration helper checks the board identity, image hash,
  partition table and OTA selection before restoring only that app. It is prepared
  but not exercised; the accepted particle version remains on the board.

## Host validation and design tradeoffs

The full repository host suite passes, including Nesso controls and the new
particle, gravity, mapping and pixel tests. AddressSanitizer and
UndefinedBehaviorSanitizer also pass. Coverage includes conserved particle count,
all four gravity directions, bounded positions, repeated reversals, first-sample
gravity response, near-flat noise, invalid samples, renderer buffer boundaries and
keeping the battery bar untouched by the bead renderer. An independent read-only
code review found no remaining actionable issues in the final implementation.

The first 864-bead particle solver with two collision passes measured about
13.6 fps. Reducing to 400 beads gave about 24 fps; one collision pass and a 60 Hz
physics target gave the accepted 42 fps result. Larger five-pixel shaded beads
retain the requested particulate look and approximately one-third settled fill.
Particle count is exact; the visible fill outline is approximate and changes with
gaps, overlap and droplets. This is a soft-particle toy, not a calibrated fluid solver.

## Remaining limits

Upside-down and almost-flat behavior have host checks but no separate owner
acceptance recorded. Full/low battery facial appearance, prolonged unplugged use,
restart under battery power, and execution of the restoration helper remain
untested. Full factory restoration is not claimed. Other OpenToy devices remain
explicitly deferred. Raw logs, binaries, identity and recovery artifacts stay
outside Git in the private bench folder.
