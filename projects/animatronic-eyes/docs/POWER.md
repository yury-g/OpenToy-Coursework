# Always-visible battery-saving mode

The owner chose to keep the eyes visible, dim the screen and animate less. Feather Eyes 1 v1.3.0 implements this for Curious, Dreamy and Cheeky. It is an always-on power-saving mode, not deep sleep. No battery current or runtime has yet been measured.

## Features used

| Setting | Earlier normal build | Eco build |
| --- | --- | --- |
| CPU clock | 240 MHz configured | 80 MHz, reported by device |
| Backlight PWM | 115 / 255 | 28 / 255 |
| Animation update target | 30 Hz | 20 Hz |
| Screen transfers | Every update | Only when exact pixels, framing or status change |
| Unused NeoPixel supply | Enabled by board startup | Explicitly disabled; data pin low |
| Red user LED | Unused | Explicitly off |
| Connected gamepad polling | About 100 Hz | About 50 Hz |
| Missing-gamepad task wake | Every 10 ms | Every 100 ms; detection retry still once per second |
| Battery gauge read | Every 2 s | Every 10 s |
| Unrequested USB status | Every 1 s | Every 5 s |
| Wi-Fi / Bluetooth | Not started | Not started |

The backlight duty setting is 76% lower; this is **not** a measured 76% reduction in total device power. Actual battery draw also includes the CPU, LCD, regulators, gauge and any connected gamepad. The fuel gauge reports charge estimates, not the current needed to prove energy savings. The screen keeps its previous image between transfers. Blink/reaction timers keep advancing, and input can take over immediately.

`--power auto` selects eco for personality builds and normal for classic Eyes 1 / Eyes 2. `--power normal` retains the original speed/brightness for a personality build and provides a comparison/rollback option. No power mode is saved into NVS.

## Hardware capabilities and limits

- The backlight has PWM control. The NeoPixel has a separate switchable supply. On this Reverse TFT board, **TFT and STEMMA QT share `TFT_I2C_POWER`**, so turning that supply off would remove the visible face and gamepad power. [Adafruit pinouts](https://learn.adafruit.com/esp32-s3-reverse-tft-feather/pinouts) and [power management](https://learn.adafruit.com/esp32-s3-reverse-tft-feather/power-management).
- ESP32-S3 supports lower CPU frequency, automatic light sleep and deep sleep. Automatic light sleep needs power-management and tickless-idle support in the compiled SDK. The pinned Arduino 2.0.17 SDK has `CONFIG_PM_ENABLE` disabled; merely adding a runtime request cannot enable it. Manual light sleep would also require coordinating the two tasks, display PWM, I2C and USB. It is not enabled in this revision. [Espressif power management](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/power_management.html).
- Deep sleep can wake on a timer or suitable GPIO, and the ULP can do limited background sensing. It cannot keep this eye-rendering application running. The screen/backlight remain a load if kept powered; a manufacturer deep-sleep figure with peripherals off is not an always-visible runtime estimate. [Espressif sleep modes](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/sleep_modes.html).
- The board's built-in charging setup specifies a **250 mAh or larger** 3.7/4.2 V LiPo. A smaller cell requires checking its allowed charge rate and changing the charging arrangement accordingly. Do not treat “tiny battery” as a plug-in replacement of any capacity. [Adafruit battery connection guidance](https://learn.adafruit.com/esp32-s3-reverse-tft-feather/pinouts).

## Verification

Host checks verify exact frame reuse, every status/framing invalidation, unchanged rendered eyes at 20 Hz, distinct blink sequences, poke priority, timer wrap and controller behavior. All three eco builds, classic Eyes 2 and an explicit normal-power Curious build compile with the pinned toolchain. The complete OpenToy host suite passes. The firmware animation export now shows 14/10 left/right blinks for Curious, 3/3 for Dreamy and 8/8 alternating winks for Cheeky over 24 seconds. This is a simulation, not a physical-screen recording; it does not model backlight brightness.

The Curious pilot first ran 1.2.1 at normal power and passed live behavior/stress checks at 26.39 updates/s. Its subsequent 1.3.0 eco trial passed at 17.80 updates/s under the stress/recovery measurement. During a separate 24-second idle sample, it made 135 display transfers and skipped 299 identical frames (68.9% skipped). These are workload measurements, not current measurements.

The live test now waits for observed recovery, up to 12 seconds, instead of assuming the final status report arrives within a fixed 8.2 seconds. This accommodates the existing asymmetric recovery times and bounded USB reporting; no reaction timer was shortened to pass the test.

The USB stress check also discards queued pre-read reports before requesting a fresh device timestamp. An initial Dreamy result measured only 18.4 seconds of device progress because the reader was still draining old reports; the recorded animation rate during that interval was 17.93 updates/s. The final check requires fresh progress spanning at least 20 seconds and an update rate of at least 15 Hz, rather than applying the previous 30 Hz build's fixed frame-count threshold.

All three report `feather-eyes-1.3.0`, eco mode, 80 MHz, PWM 28/255 and NeoPixel supply off. Each passes changing gaze, both blink counters, synthetic left/right pokes, happy recovery, sensor pause and 20-second USB backpressure checks. No physical gamepad was attached; gamepad behavior is covered by host simulation.

| Personality | Transfers in idle sample | Skipped redraws | Skipped fraction | Updates during stress/recovery |
| --- | --- | --- | --- | --- |
| Curious | 135 | 299 | 68.9% | 17.80 Hz |
| Dreamy | 120 | 316 | 72.5% | 18.13 Hz |
| Cheeky | 114 | 322 | 73.9% | 18.07 Hz |

Dreamy stopped reporting through USB after the first flood test; the owner confirmed that its face was still animating. It accepted a software restart without reflashing, then passed the corrected fresh-timestamp test. This is recorded as a diagnostic recovery event, not a measured power failure or an established firmware fix. Final USB close/reopen checks passed on all three, including three successive reopen cycles on Dreamy; every capture showed continuing frame progress. The reader now explicitly clears both modem-control lines on close.

Each 342,512-byte final image was independently verified in its original selected application slot. All three writes occupied sectors 0x10000–0x63fff, with all flash outside that range verified unchanged. Fresh full 1.2.0 backups, the 1.2.1 comparison image and recovery instructions remain private.

## Measuring the next improvement

Measure at the battery connection, with USB disconnected and the intended gamepad configuration. Compare normal and eco builds over several complete blink/look cycles, using the same board and battery voltage. Record average current and peaks. Approximate runtime is usable battery capacity divided by average battery current; then verify it with a discharge test. Charge percentage changes during USB power are not a valid substitute. Until these measurements exist, longer runtime is expected from the reduced loads but no hours/days or improvement multiplier is claimed.

For substantially longer life while retaining a visible image, a different display technology and board design may be needed. For this hardware, allowing longer screen-off sleep would offer a different tradeoff from the owner's current always-visible choice.
