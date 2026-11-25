# Saved checkpoint: v26 strong haptics

Saved October20,2026 on `feature/qwiic-accessories`, tagged `checkpoint-v26-strong-haptics`. No further hardware test or firmware install is running at this checkpoint.

## Installed state

- Blue and replacement Green: **BatteryFaces-v26-strong-haptics**. Qwiic hold commands127/127 independent of diagnostic level; release stops. D2 toggles smooth seeded breathing with peaks64–96 and different rhythms for each board. Boot starts with breathing off.
- Tab5: **BatteryMonitor-v25-accessory-outline**. Live battery faces, matching motor outlines, mode/intensity/button status and Find Me remain available. Blue is the network gateway.
- Other members keep their previously installed battery firmware. The retired cracked/hot Green remains excluded; cameras remain decommissioned and archived separately.

## Verified

29 portable C++ test modes and38 Python tests passed. Both Feathers passed300ms64/127 command bursts, natural return to zero/inactive and eight seconds of stronger breathing; Tab5 showed fresh status and outlines. The final bench cleanup confirmed both motors off. Owner controls can change their state afterward. No battery race or charge/discharge cycle was started.

The127 burst initially raised a warning. A diagnostic repeat identified supply-limited acceleration (`EVENT1=20`, `warning_diag=40`). Fresh supply/acceleration limit warnings are reported while hardware limiting stays active. Critical faults and temperature/configuration/unknown warnings still stop output, including immediately after starting when detected. Manufacturer motor voltage/current settings are unchanged.

[Exact installed application/source fingerprints](../outputs/accessory-checkpoint-v26.json) · [Live smoke-test evidence](../outputs/strong-haptics-v26-live.json) · [Accessory controls and limits](qwiic-accessories.md) · [Roadmap](ROADMAP.md).

## Deferred work

- Full-power temperature/current and battery-only runtime measurements; the brief USB smoke test does not qualify sustained maximum-power operation or battery health.
- Runtime-log annotation of haptic duty, and an independent gateway if Blue loses power.
- Automatic LED cache recovery after a powered Qwiic module is physically disconnected/reconnected. Explicit LED reassertion is currently available.
- The longer uninterrupted two-board creature/Find-resume verifier remains incomplete after owner input interrupted it; earlier failed/investigative records stay labeled in [bench evidence](../outputs/README.md).

The README's actual hardware screenshots and all207 retained images remain in the image archive. No credentials, original private flash backups or compiled firmware binaries are published.
