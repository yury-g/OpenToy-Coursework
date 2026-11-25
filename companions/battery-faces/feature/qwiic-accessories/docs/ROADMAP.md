# Battery Faces roadmap

Work is organized around checkpoints so hardware experiments do not replace the battery application.

| Stage | Scope | Checkpoint |
|---|---|---|
| Battery family | Faces, trends, modular Tab5 tiles, passive log and optional runtime race | Preserved on `main` at `af0b776` |
| Local accessories | Qwiic Button, locator LED, D2 pulses, activity and intensity display | v21; 18 USB burst checks passed |
| Adjustable haptics | D2 toggles seeded creature breathing; Qwiic holds buzz until release; explicit stop/fault handling | v26 Feathers / v25 Tab5 on accessories branch |
| Live accessory status | Tab5 shows motor mode/intensity and button up/down/recent press and matching face outline; Q LED fades with breathing | v26 Feathers / v25 Tab5 on accessories branch |
| Remote Find Me | Tab5 card controls, stop from any page, physical acknowledgement, lease expiry | v22 on accessories branch |
| Runtime workload annotation | Record breathing/manual haptic duty alongside battery observations | Future; existing CSV has no haptic duty column |
| Battery-only power characterization | Measure actual motor/rail current and repeat controlled, pack-labeled runtime observations | Future; USB voltage samples cannot establish this |
| Independent gateway | Keep other reporters observable when Blue loses power | Future; Blue remains gateway |

The current accessories focus is Blue, replacement Green and Tab5. Other family members retain their battery functionality; Orange and Silver are not programmed during this step. The retired Green stays excluded. Camera experiments remain archived on `archive/camera-test-v17`.

Nothing here enables an automatic charging/discharging cycle. Tab5 continues its passive background log; a real comparison race remains an explicit owner action. The host tests cover missing drivers, stop retries, readback faults, packet replay/reordering, lease expiry and session caps. Hardware evidence is linked from the [accessory notes](qwiic-accessories.md), with measurement limits kept alongside results.
