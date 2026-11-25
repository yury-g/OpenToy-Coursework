# Bench evidence

Current source and binaries are fingerprinted in [accessory-checkpoint-v25.json](accessory-checkpoint-v25.json). The completed current outline check is [outline-live-v25.json](outline-live-v25.json): both sources and Tab5 reported active outlines, actual framebuffers were captured, and both outputs returned to verified zero/inactive. The longer [creature verifier attempt](creatures-v25-live.json) is incomplete: physical button presses interrupted it. It must not be described as a passed two-board continuous-cycle/Find-resume run. [Host results](host-tests-v25.json) cover the portable behavior. These are USB-powered observations, not measured current, cell capacity or battery-only endurance.

The earlier completed [v21 short bursts](qwiic-burst-tests-v21.json), [v22 finite breathing](haptic-breathing-v22.json) and [v22 Find Me](find-me-v22-live.json) remain separate checkpoints.

Development failures are retained explicitly:

- [Broadcast relay gap](creatures-v23-first-relay-gap.json): battery data aged while direct accessory packets stayed fresh; the dashboard relay now uses its fixed private unicast address.
- [Sampling gap](creatures-v23-first-sampling-gap.json): synchronous Tab5 reads interrupted the host sampler; metadata now runs in a separate worker.
- [Find pulse missed by sampler](creatures-v24-first-find-observation.json): the source pulse counter advanced, but sparse host samples missed the 180ms drive interval. Find sampling now uses the same independent collector.

A `complete: false` development record is not a successful bench result. Physical button behavior also has host regressions and owner feedback; no synthetic button presses are presented as physical tests.

- [Resume observation timing](creatures-v25-first-resume-observation.json): source and dashboard samples straddled the stop/resume transition; the final collector samples both concurrently through the full lease and quiet interval.
- [Cable interruption readback](accessory-fault-readback-v25.json), [explicit recovery](blue-explicit-reprobe-v25.json), and [LED reassertion](button-led-reassert-v25.json): Blue safely latched its motor off after I2C errors. The owner subsequently confirmed moving/disconnecting the Qwiic cable and fixing it. Recovery was explicit, with outputs off; historical error counters were retained.
- [Physical press interruption](creatures-v25-physical-press-interruption.json): the owner pressed Blue during the automated envelope test. The sample records held mode, drive20 and LED128; the test stopped and preserved its partial result.

The idle LED value is cached between changes. After physically disconnecting/reconnecting a powered button module, an explicit LED reassertion may be needed; automatic detection of every module reset is not claimed.

## Stronger v26 profile

[Completed USB smoke test](strong-haptics-v26-live.json) covers brief64/127 bursts, natural stop, stronger breathing peaks and Tab5 status. [Checkpoint](accessory-checkpoint-v26.json) fingerprints installed source and binary. The [initial warning](strong-haptics-v26-first-warning.json) and [diagnostic repeat](strong-haptics-v26-warning-diag.json) remain separate failed/investigative records: EVENT1 `20`, warning diagnostic `40` means supply-limited acceleration. Manufacturer limits remain unchanged; temperature/configuration/unknown warnings and critical faults still stop output.
