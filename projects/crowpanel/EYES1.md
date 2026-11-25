# Eyes 1 on CrowPanel V1.0

Last verified standalone ESP32-P4 installation: **0.2.2**, created **2026-11-17**.
This page describes that installed checkpoint. Current source is the [0.3.0 Menu
candidate](MENU.md), not a verified new deployment.
The panel runs independently; XIAO is not used. Keep the XIAO physically separate.

![Native eye renderer preview](images/eyes-smooth.gif)

*Actual native-resolution drawing code with scripted timing and a left-eye poke;
not a device video or speed measurement. Header and footer omitted.*

Version 0.2.0 introduced analytic curves and one-pixel edge coverage for crisp eyes,
pupils and lids. It renders two bounded native-size regions, preserving the
existing personalities and reactions. Historical pixel renderers remain unchanged.

## Use

| Control | Result |
| --- | --- |
| Touch the left or right half of the eyes area | Poke that eye; it closes, becomes angry, then recovers after release |
| Touch both halves | Poke both eyes |
| Curious / Dreamy / Cheeky | Select the existing OpenToy personality and its distinct blink rhythm |
| ECO ON / ECO OFF | Change animation cadence and backlight; the control remains visible |
| Hide info / Show info | Hide or restore the app/version/date/battery bar for this session |
| Restart | Restore Curious, ECO ON and the visible information bar |

The screen stays in landscape. Battery is neutral `BAT --`: its controller responds, but the exact panel
protocol and calibration remain unqualified. Candidate voltage is available only
in diagnostics; percentage and charging are unknown. See [battery evidence](BATTERY.md). Motion, audio and radio features are outside this first port.
ECO requests a 66 ms animation interval and 35% backlight setting; normal mode
requests 33 ms and 65%. Scheduler and display work determine actual cadence.
These are driver settings, not measured brightness or battery-life estimates.
Measured cadence for this native renderer is recorded in the verification log.

## Family controls migration

This installed version uses the footer controls above. The shared
[family controls standard](../../docs/CONTROLS.md) is a future migration here:
Menu and the full performance mode are implemented in the [0.3.0 source candidate](MENU.md),
with deployment pending hardware handback. Hide info hides
only the information bar; the footer remains. BOOT/RESET are never app controls.

## Build and installation

Current source builds the uninstalled **0.3.0 candidate**. The verified 0.2.2
source checkpoint is commit `fd90e20`; its private image/recovery are preserved.
Activate ESP-IDF **5.4.2**, then from OpenToy run:

```sh
python projects/crowpanel/tools/build.py --work /private/path/crowpanel-eyes
python projects/crowpanel/tools/check.py
```

This stages unchanged Eyes behavior/renderer headers and builds outside Git.
The canonical version/date are in `firmware/metadata.json`; the helper includes
a source digest in diagnostic output and records the image digest privately.
Pinned direct dependencies are in `firmware/main/idf_component.yml`, with the
resolved transitive dependencies in `locks/idf5.4.2.lock`.

**Do not run the whole-device flash command printed by ESP-IDF.** Follow the
[private recovery and app-only procedure](README.md#recovery-before-replacement).
The build's partition entries match this enrolled panel's original layout, but
the partition table and bootloader must remain untouched. A different panel must
be enrolled independently before using these assumptions.

## Diagnostics and evidence

UART0 accepts `?` for status, `1`–`3` for personality, `H` for information bar,
and `E` for ECO. `L`, `R` and `B` generate explicitly labeled 300 ms synthetic
pokes; `!` pauses touch sampling for 1.5 seconds to exercise input expiry.
These commands do not write settings or flash. Reports use a bounded queue and
nonblocking UART output; incoming commands are bounded per loop.

The adapter uses the GT911 ready bit to distinguish fresh packets from cached
coordinates. Missing packets expire after 100 ms; read failures release contact.
Each I2C transaction is limited to 20 ms. Header geometry changes are retried
under the LVGL lock and input uses the last successfully displayed layout.

See [verification](EYES1-VERIFICATION.md) for build, installation, live telemetry
and owner observations; these are distinct evidence categories.
