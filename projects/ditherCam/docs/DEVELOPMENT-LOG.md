# ditherCam Development Log

[← ditherCam overview](../README.md) · [Visual history](visual-history.md) · [Verification](VERIFICATION.md) · [3D enclosure](../../3d-printed-designs/ditherCam/README.md)

This log records how **ditherCam** evolved from an initial monochrome camera idea into the two-board wireless camera prototype deployed on **November 25, 2026**, preserving superseded directions and open checks honestly.

---

## Stage 1 — Choosing the camera hardware and display look

Development began with a request for a camera built around the **Waveshare ESP32-S3-RLCD-4.2** reflective monochrome screen, initially considering both video and stills. Possible camera companions—including the **Seeed Studio XIAO ESP32-S3 Sense** and **M5Stack Tab5**—were compared alongside RLCD look simulations. External lens upgrades were discussed as an exploratory option, with no add-on lens selected.

Rather than ordering new camera optics, the project reused the owner's existing **XIAO ESP32-S3 Sense** (`OV2640` module) plugged into a **Seeed XIAO Expansion Base** (`0.96"` OLED and `D3` passive buzzer). Early board-to-board wiring questions explored an inter-board SPI link, which was set aside in favor of direct Wi-Fi between the two ESP32-S3 boards—verified unplugged during owner testing.

## Stage 2 — Designing a print-in-place flip enclosure

To support both outward shooting and selfies without disconnecting boards or relying on the short camera flex cable, the mechanical design settled on a **top-flip captive pod** over an earlier side-swing study ([layout options](design-history/layout-options.png)) and early standalone camera-case retention prototypes ([Prototype v1–v6](visual-history.md#stage-1--earlier-standalone-xiao-camera-case-retention-studies)).

Key mechanical choices in [`Camera.scad`](../../3d-printed-designs/ditherCam/cad/Camera.scad):
- **Whole plugged stack rotates:** The pod carries the entire plugged XIAO + Sense + Expansion Base assembly (`hw_xiao_stack.stl`), with the printed shell taking hinge torque.
- **Captive cone pivot over flexing PLA:** An opposed 45° captive cone hinge (`(0, 0, 22 mm)`) was selected instead of a repeated-bend thin PLA hinge, using a flat `108 × 58 mm` standing base and targeting supportless PLA printing on a Snapmaker U1 (`0.4 mm` nozzle, `0.20 mm` layers).
- **Integral stepped faceplate:** A later revision exposed both the camera lens and recessed Expansion Base OLED while keeping `SNAP`, `PWR`, and `BROWSE` accessible on the RLCD crown ([baseline vs. faceplate renders](visual-history.md#stage-4--faceplate-revision-cad-deliverables--assembly-preview)).

The owner reviewed the folding motion in an interactive web dashboard ([3D flip viewer](../../3d-printed-designs/ditherCam/index.html)) and the design review included a Snapmaker Orca PLA preview project (`Camera-U1-PLA-preview.3mf`; U1 toolpaths were not generated, with a Bambu Studio reference slice used instead). After printing, the owner reported difficulty inserting both boards, prompting an interactive 6-step [assembly guide](../../3d-printed-designs/ditherCam/assembly/index.html) highlighting rigid top tabs vs. compliant clips and top-down stack loading. The owner later confirmed *"ok. i got these"*—recorded as an owner fit attempt, not formal dimensional or fatigue qualification.

## Stage 3 — Narrowing UI scope and internal flash storage

Before finalizing low-level firmware, clickable UI mockups embedded in the 3D enclosure explored video recording, gallery delete confirmations, and on-device settings screens ([Stage 3 UI mockups](visual-history.md#stage-3--retired-interactive-ui-mockups-video--settings--delete)). After reviewing the look, the prototype was narrowed strictly to **photos only**:
- **Single-purpose controls:** `KEY` (`Snap`) shoots or holds to toggle mode; `BOOT` (`Browse`) steps strictly reverse-chronologically from newest to `OLDEST` with no menus or hold action (documented as an explicit prototype exception to OpenToy's `CONTROLS.md`).
- **No SD card, internal flash only:** Initial file-size assumptions proved too large for internal flash. Output was tuned to `600 × 450` grayscale JPEGs (`~7–17 KB` scene-dependent), stored in separate LittleFS `media` partitions (`8 MiB` RLCD, `3.9375 MiB` XIAO `D1`, `256`-item catalog cap each).
- **Truthful icons & audio limits:** Graphical battery icons replaced numeric `BAT` text, with XIAO honestly displaying `?` (no battery sense path). Shutter sound uses codec/buzzer TX feedback; microphone recording and video clips remain disabled, and audible speaker confirmation stays pending.

## Stage 4 — Contrast, speed, full-screen RLCD, and 180° flip orientation

Iterating on live view responsiveness and legibility produced the deployed pipeline in [`shared/FlipCamDither.h`](../shared/FlipCamDither.h) and [`shared/FlipCamDisplay.h`](../shared/FlipCamDisplay.h):
- **90° CW baseline & high contrast:** High-contrast LUT + ordered 4×4 Bayer dithering and a packed 1-bit `304 × 228` wireless frame stream replaced heavier preview transfers; the owner confirmed the 90° CW landscape orientation upright.
- **Frame rate reality:** While `>= 12 fps` was targeted (and `12.2–12.4 fps` appeared under earlier lighting), current-scene bench checks measured **10.19–10.58 fresh fps** on RLCD and **10.38–10.60 fps** on OLED—matching pre-update throughput with no material slowdown from full-screen scaling.
- **Black background & full-screen overlay:** The owner first requested a black interface background like the OLED, then an edge-to-edge camera image. The RLCD live screen switched to full-screen `400 × 300` without outer frames or status bars (tracked as a status-bar exception), enlarging the `304 × 228` stream without increasing wireless payload.
- **Manual `SELFIE` / `REAR` toggle:** When flipping the pod outward, the owner noted the OLED looked right while the RLCD was upside-down. With no hinge sensor, [`FlipCamUiState.h`](../shared/FlipCamUiState.h) added a ~1 s `Snap` hold that persists `REAR` across restarts, rotating only the RLCD live preview and **newly saved** stills by 180° while keeping OLED preview pixels and previously saved photos untouched.

## Stage 5 — November 25, 2026 deployment & OpenToy integration

On **November 25, 2026**, host tests (`tests/run_all.py`) and USB-injected hardware checks passed for hold-vs-tap separation, gallery playback, CRC preservation, and 180° rotation symmetry (`31.82 → 1.80` mean pixel diff; full details in [VERIFICATION.md](VERIFICATION.md)). Physical `KEY` hold feel, outward `REAR` confirmation, and audible shutter output remain pending owner verification. The project was named **ditherCam** inside OpenToy, accompanied by this log, firmware UI renders, Blender flip/insertion animations, and the public [assembly webpage](https://dithercam-assembly.narwhal2025.chatgpt.site).
