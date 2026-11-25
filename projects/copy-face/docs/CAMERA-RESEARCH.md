# Camera and vision evidence — 2026-11-17

**Owner clarification:** no camera is connected to the panel (2026-11-17).
The first prototype had treated the fitted camera as unknown.
For current physical results see [verification](../VERIFICATION.md); for the
implemented/pinned adapter see [camera notes](../locks/camera-notes.md).
The manufacturer sources establish an optional supported path, not what is
physically attached. Do not substitute a V1.2 board definition for this V1.0 unit.
The [official product listing](https://www.elecrow.com/crowpanel-advanced-9inch-esp32-p4-hmi-ai-display-1024x600-ips-touch-screen-wifi-6-support.html)
describes its 2 MP MIPI-CSI camera as optional.

## V1.0 primary sources

The [Elecrow wiki](https://media-cdn.elecrow.com/wiki/CrowPanel_Advanced_9inch_ESP32-P4_HMI_AI_Display_1024x600_IPS.html)
identifies DHE04209D and links revision-specific examples. Some labels on that
page are inherited from other panels, so the versioned source and schematic are
the wiring authority for this work.

Vendor source is pinned to
[`342a40719c98601fbd282f72e7fc2cd413133dd8`](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/tree/342a40719c98601fbd282f72e7fc2cd413133dd8/example/V1.0/idf-code/Lesson13-Camera_Real-Time).
The V1.0 Lesson13 camera example is the relevant demo; its generic template README
is not proof of compatibility with every chip listed in its template table.

| Question | Source evidence | Consequence |
| --- | --- | --- |
| Camera interface | `bsp_camera.c` opens `ESP_VIDEO_MIPI_CSI_DEVICE_NAME` using esp_video/V4L2; requests RGB565 buffers | CSI sensor path, not a USB camera or XIAO camera |
| SCCB bus | `bsp_camera.h`: controller 1, SCL GPIO13, SDA GPIO12; initialization 100 kHz | Separate from touchscreen controller 0 on GPIO45/46 |
| Sensor configured | `sdkconfig`: SC2336 enabled with MIPI autodetect; RAW8 1024×600 at 30 fps selected | Candidate driver only; must read actual sensor identity |
| Reset in demo | CSI configuration sets reset and power-down to -1 | Do not invent a reset GPIO from a misleading net label |
| Toolchain | `CONFIG_IDF_INIT_VERSION="5.4.2"`; manifest permits IDF >=5.4.0 | Keep the known display baseline at IDF 5.4.2 initially |
| Dependencies | Demo requests esp_cam_sensor ^1.2.0, esp_sccb_intf ^0.0.5, esp_video ^1.1.0 | These ranges are research evidence, not a resolved Copy-Face camera lockfile |
| ISP setup | SC2336 custom IPA JSON is selected by relative path | Preserve and audit that configuration before a camera build |

Read [V1.0 EAGLE schematic](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/342a40719c98601fbd282f72e7fc2cd413133dd8/Eagle_SCH%26PCB/1.0/ESP32-P4%20Display%209.0%20inch%20V1.0.sch)
as XML to trace actual pin endpoints, not just net names:

| FPC3 schematic pin | Signal/path |
| --- | --- |
| 4 / 5 | CSI_SCL / CSI_SDA, level shifters Q7/Q6, resistors R89/R90 to P4 GPIO13/12 |
| 15 / 16 | CSI data lane 0 positive / negative |
| 21 / 22 | CSI data lane 1 positive / negative |
| 18 / 19 | CSI clock positive / negative |
| 8 | CSI_RESET through Q4/R88 to P4 GPIO11; MCU-side net is misleadingly named `IO46_CSI_RESET` |
| 11 | XVCLK from oscillator Y3 |
| 9 | DOVDD_1V8 |
| 1 / 2 | AVDD_2V8 |

These are schematic pin numbers, not an insertion/orientation guide. Do not
rewire or drive resets based on this table alone. Verify module markings,
connector orientation and driver requirements first.

The vendor demo also needs adaptation before reuse: a task is given the address
of its creator's local `video_fd`; several initialization/allocation results are
ignored; the capture callback performs display work; receive failure loops can
retry without delay. Copy-Face must own stable task state, bound failures, release
buffers reliably and publish only copied observations to the renderer.

## Espressif detector and mouth limitation

[ESP-DL HumanFaceDetect](https://github.com/espressif/esp-dl/blob/master/models/human_face_detect/README.md)
lists MSR+MNP and ESPDet face detectors for P4. Upstream MSR/MNP measurements
suggest a useful starting point for gaze; they are not this panel's end-to-end
latency. The published setup uses 400 MHz CPU and 250 MHz PSRAM, unlike the
prototype's 360/200 MHz settings. Camera conversion, scheduling, multiple face
candidates and display traffic add work. Measure on the actual board.

The [official example](https://github.com/espressif/esp-dl/blob/master/examples/human_face_detect/main/app_main.cpp)
prints five points in this order: left eye, left mouth corner, nose, right eye,
right mouth corner. The inspected example blob is
`9a7b36549fc234bcffdac683ea26f1303b42dd73`; model README blob is
`9e2757791e81650b43a8030708d7eaaaa070280b`. These inspected blobs are evidence,
historical research references. Version 0.2 pins its runtime separately.

Two mouth corners do not locate upper/lower lips or center curvature. A smile,
frown or opening classifier made from those points would require separate
validation and is not implemented. Real mouth capability must remain false.
The prototype's scripted lip contours demonstrate presentation only.

Next evaluate a lightweight lip-landmark model with upper/lower lip centers and
corners, including operator support, quantization, RAM, license and training/eval
coverage. Measure geometry under roll/yaw, lighting, glasses/facial hair and
occlusion; establish a neutral calibration and deadband. Do not report emotional
state. A suitable native model has **not yet been verified**, which is different
from proving native mouth analysis impossible.

## Hardware continuation after release

1. Read explicit private handoff, enrollment and recovery pointers. Verify its
   current installed Eyes source/build; preserve the recoverable checkpoint.
2. Verify the actual module safely. A missing/unclear camera needs a physical
   check; an absent handoff alone needs no cable action. No broad port or bus scan.
3. Build a dedicated compatible capture probe with finite failure handling. Read
   sensor identity and frame dimensions, retain no frames automatically, and
   leave the camera unavailable on failure. Prefer model data embedded in the app
   (`FLASH_RODATA`) rather than adding model partitions to the preserved layout.
4. Add bounded face detection on an independent worker, latest-result handoff,
   age rejection and explicit orientation transform. Verify left/right/up/down
   using actual movement and test face loss/multiple faces. No recognition model.
5. Measure camera-to-gaze latency, presentation cadence, heap, capture failures
   and cold boot. Evaluate a lip model separately; keep mouth neutral until then.
6. Continue genuine battery measurement if device-owner verification is pending.
   A physically attached battery is not a reading. Retain `--` until qualified.

## Later audio

Follow the [family roadmap](../../../docs/ROADMAP.md). Qualify microphone and
speaker paths independently; add local sound alertness with sensitivity,
debounce, cooldown and suppression of the toy's own audio. Gaze/mouth copying
has priority. Quiet idle stays visible; true low-power acoustic wake requires
hardware qualification and measured current. No audio implementation is included.
