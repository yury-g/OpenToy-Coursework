# Qwiic accessories: Blue and Green

This work lives on `feature/qwiic-accessories`. The battery-only `main` checkpoint remains available. Only Blue, the replacement Green, and Tab5 receive this update; Orange and Silver keep their battery firmware.

Each Feather shares its existing STEMMA QT I2C bus with its MAX17048 gauge, SparkFun DA7280 haptic driver (`0x4a`, chip ID `0xba`) and SparkFun Qwiic Button (`0x6f`, ID `0x5d`, firmware 1.3). The bus and its power supply are not restarted. The retired cracked/hot Green is excluded.

## Controls

| Control | Action |
|---|---|
| D0 | Next battery; also acknowledges an active Find Me |
| D1 | Face/trend page; also acknowledges Find Me |
| D2 tap | Toggle continuous algorithmic creature breathing on/off |
| Qwiic Button | Full command127/127 for as long as held, independent of diagnostic level; release stops the override |
| Tab5 Blue/Green face card | Start Find Me; tap again to stop |
| Tab5 red STOP BUZZ | Stop both Find Me targets from any page |

The Qwiic LED is a dim locator when idle, bright while pressed, and fades as `min(command × 6, 192)` during breathing, reaching zero during a rest. Hardware LED readback is checked. The Feather status strip shows button state, buzz level, breathing or Find Me, plus an amplitude bar and face animation. D0/D1 and all battery pages remain available. The held override shows HOLD and its amplitude; release resumes creature breathing if D2 left it enabled. A Qwiic press acknowledges Find Me; a delayed remote STOP does not cancel the physical hold.

## Breathing and output limits

The motor is controlled through the DA7280's I2C direct-drive amplitude register, not a Feather GPIO PWM output. The USB diagnostic LOW/MED/HIGH selections are commands **20 / 64 / 127 on a 0–127 scale** (HIGH is the boot default), not calibrated force, current, speed or percentages of physical intensity.

D2 creature breathing uses an integer smoothstep envelope with a stable seed derived from each Feather's hardware identity. Each cycle varies its inhale 3.5–6 seconds, exhale 4.5–7 seconds, rest 1.5–3 seconds and peak 64–96. The two boards share the exact same firmware, with different reproducible rhythms. It runs until toggled off, interrupted by a fault/low reading, or rebooted; it starts off after reboot. Qwiic hold and Find Me temporarily override it.

A held buzz requires fresh, valid button reads; release stops it, and missing confirmations expire after250ms. It does not retrigger from a stale held state. The existing finite diagnostic breathing command still permits peaks4–32 and durations10–60seconds in whole ten-second cycles. A fresh cell reading below3.45V prevents a test start; observed low voltage stops breathing. A stop writes both zero amplitude and inactive mode, verifies readback, and retries if acknowledgement fails. Driver faults or unexpected drive/mode readback stop the session. Fresh DA7280 warning diagnostics are read on every fault check: temperature, configuration, unknown warnings and critical faults stop output; a warning containing only supply/acceleration limiting may continue with the hardware limiting intact. Full command does not guarantee unclipped physical output.

Driver register/fault checks run during both continuous modes. Local diagnostic short pulses are limited to50–800ms, with at least500ms of quiet after a confirmed stop. Find Me uses180ms MED64 pulses every1.5seconds, a renewable six-second lease and a fixed two-minute maximum. [Find Me protocol and controls](find-me.md).

## Verification and measurement limits

The v21 USB bench run completed **18 short bursts**: both boards, commands20/40/64, durations100/350/700ms. No reset, I2C error or DA7280 fault was reported, and both outputs returned to verified zero/inactive. Sampled cell voltage remained near4.162V on Blue and3.774–3.776V on Green. [Raw burst evidence](../outputs/qwiic-burst-tests-v21.json).

The v22 USB breathing run completed **ten full breaths** across both boards: peak20 for30seconds and peak32 for20seconds on each. Every session stopped naturally within106ms of its deadline, with verified zero drive/inactive mode and no reset, I2C error or driver fault. Minimum sampled cell voltage was4.160V on Blue and3.791V on Green. [Raw breathing evidence](../outputs/haptic-breathing-v22.json).

The v22 Find Me bench run passed on both targets:20seconds of renewed LOW pulses while navigating Race/Log/Demo, global STOP from Demo, and a separate six-second unrenewed lease that expired at the target. Both outputs were confirmed zero/inactive afterward, and the real race remained WAITING/session0. [Raw network evidence](../outputs/find-me-v22-live.json). Physical presses, unplug/reboot scenarios and the full120-second cap were not separately exercised on hardware in this run; their logic has host tests and review.

These checks observe software timing, driver registers and sampled cell voltage under USB power. They do not measure motor/3.3V rail current, fast voltage transients, temperature, battery-only endurance or cell capacity. The programmed DA7280 current limit is a configuration value, not a measured circuit current. No automatic charge/discharge cycle is introduced. Motor activity changes the runtime workload; record the chosen mode when comparing batteries. The existing runtime CSV does not yet annotate haptic duty.

Reproduce explicitly, with the appropriate firmware already installed:

```sh
.venv/bin/python tools/run_host_tests.py
.venv/bin/python -m unittest discover -s tests -p 'test_*.py'
.venv/bin/python tools/verify_creatures.py --run
```

The v21/v22 burst, finite-breath and Find-only evidence above remains a historical checkpoint. The v25 verifier is designed to check seeded continuous cycles, Find Me preemption/resume and explicit stop; the final two-board attempt was interrupted by owner button presses and remains explicitly incomplete. A separate completed low-intensity outline check captured both native screens and Tab5, confirmed fresh outline telemetry from both sources, and verified both outputs stopped. [Current outline proof](../outputs/outline-live-v25.json). Physical held/release paths have host regressions; the owner confirmed the earlier buttons worked before requesting the new hold behavior.

Tab5 uses the Feather's same rounded outline at exactly twice its native size: 270×134 pixels with an 18-pixel radius and a two-pixel stroke. The outline follows actual source telemetry: face-colored while active, gray during a short pulse afterglow, absent when idle or stale. Breathing keeps it visible through the quiet part of a cycle.

During development Blue stopped safely after I2C errors. The owner confirmed a Qwiic cable had been moved/disconnected and was fixed. An explicit driver probe recovered readiness without resetting the board or clearing counters; the subsequent verifier compares against those retained counters. A later physical press interrupted an automated run and was saved separately. [Bench records and limitations](../outputs/README.md). A physically reconnected button module can lose its LED setting while the host cache retains it; explicit `button led on` reasserts it. Automatic recovery from every module reset is future work.

The hardware tools are deliberate one-shot tests. They save partial evidence on a failure and attempt a stop during cleanup. Do not run multiple USB clients against the same device. A generic `status`/screen transfer deliberately stops local actuation before a potentially long serial operation; `haptic sample` collects short fresh readings without interrupting breathing.

## Source attribution

The vendored [SparkFun Qwiic DA7280 library](https://github.com/sparkfun/SparkFun_Qwiic_Haptic_Driver_DA7280_Arduino_Library) retains its public-domain/Beerware notice. Its chip-ID read is initialized, correcting an uninitialized upstream local variable. The built-in soldered LRA uses SparkFun's `defaultMotor()` configuration, with frequency tracking disabled as in its example. Other protection configuration is retained. Qwiic Button register access follows [SparkFun's official library](https://github.com/sparkfun/SparkFun_Qwiic_Button_Arduino_Library); only volatile LED registers are written, never its address/NVM register.

## Strong profile (v26)

Qwiic hold now commands127/127 regardless of the diagnostic level. D2 keeps the same seeded timing and smoothstep shape, with peaks64–96 instead of12–28. Find Me uses64/127 pulses; diagnostic LOW/MED/HIGH are20/64/127. Release, stale button reads, low cell readings and real driver faults still stop output.

An initial Blue127 burst raised EVENT1 `20`; one bounded diagnostic repeat identified warning register `40` (acceleration limited by the supply). This is a hardware clipping notification rather than a temperature/overcurrent fault. The firmware keeps the motor's configured nominal/absolute voltage and current limits and reports `warning_diag` rather than treating classified supply limiting as an unrecognized fault. A new temperature/configuration warning still causes immediate startup rejection or a stop on the next periodic check. [Renesas DA7280 datasheet, sections5.2.4 and5.6.6, Table22](https://www.renesas.com/en/document/dst/da7280-datasheet).

The completed v26 USB smoke test observed300ms bursts at64 and127 on each board, natural return to zero/inactive, and eight seconds of the new creature profile with observed peaks66(Blue) and79(Green). Tab5 showed fresh breathing status and active outlines. No I2C errors or critical driver faults were recorded; the classified acceleration-limit warning remained visible. Both outputs were explicitly confirmed off afterward. [Raw evidence](../outputs/strong-haptics-v26-live.json). This is a short smoke test, not a continuous full-power thermal/endurance qualification; physical release is covered by wrapper tests and owner use rather than synthetic physical presses.
