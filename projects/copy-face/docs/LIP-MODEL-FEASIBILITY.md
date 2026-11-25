# Dense Lip & Eyelid Landmark Model Feasibility (ESP32-P4 / ESP-DL 3.3.15)

Date: 2026-11-17
Target platforms: **Elecrow CrowPanel 9-inch P4 V1.0** and **M5Stack Tab5** (`ESP32-P4 rev 1.3`, dual-core 360 MHz, 32 MiB PSRAM @ 200 MHz, ESP-IDF `5.4.2`, `esp-dl 3.3.15`, `human_face_detect 0.5.2`).

## 1. Current Baseline & Why 5-Point Detection Cannot Drive Mouth Copying

- `espressif/human_face_detect 0.5.2` (`MSRMNP_S8_V1`) outputs a face bounding box plus **5 keypoints** only: left eye center, left mouth corner, nose tip, right eye center, and right mouth corner.
- Two mouth corners establish horizontal width and roll, **not** inner-lip separation, lip contact vs. open aperture, or central lip curvature (a wide closed grin and an open vowel can share identical corner coordinates).
- Per `core/Tracking.h` and `core/LipLandmarkAdapter.h` (`applyFivePointOnly()`), 5-point detections **must leave `mouthValid = false`**, driving smooth eye gaze only while keeping mouth state explicitly unavailable.

## 2. ESP-DL `3.3.15` Operator & Quantization Audit

Verified directly against local cached `espressif/esp-dl == 3.3.15` (`commit 512ed0b0dc5f5320f3d9ca5a5aea189803f87f31`):

- **Upstream Model Zoo Status**: `esp-dl` does **not** currently ship a pre-packaged dense 68/98/468-point facial landmark `.espdl` component (only 5-point `MSRMNP_S8_V1` and box-only `ESPDet_Pico`).
- **Supported Operators (`dl/module/include/`)**: `Conv2D` & `DepthwiseConv2D` (dual-core P4 SIMD accelerated: `6.2 ms` for `224×224×3 -> 112×112×16`), `Add`, `Mul`, `Concat`, `PReLU`, `LeakyRelu`, `Clip` (`ReLU6`), `HardSwish`, `HardSigmoid`, `Sigmoid`, `AveragePool`, `MaxPool`, `GlobalAveragePool`, `Gemm`, `MatMul`, `Reshape`, `Flatten`, `Resize`.
- **Model Format & Conversion**: FlatBuffers `.espdl` zero-copy rodata/partition format quantized via [`espressif/esp-ppq`](https://github.com/espressif/esp-ppq) (Apache-2.0), supporting mixed precision **`w8a8` backbone + `w8a16` regression head** (recommended so 8-bit output quantization does not coarsen inner-lip contact thresholds).

## 3. Candidate Licensed Dense Landmark Models

| Candidate | Source & License | Input ROI | Landmarks & Mapping to `Copy-Face` (`9` inner lip pairs + `6` per eye) | Estimated `.espdl` Footprint & ESP32-P4 Speed (Hypothetical until `.espdl` exported) |
| --- | --- | --- | --- | --- |
| **1. MediaPipe `face_landmark` (`BlazeFaceMesh`) — Recommended** | Google MediaPipe (`Apache-2.0`), `face_landmark.tflite` / ONNX `192×192×3` | `192×192` RGB aligned face crop from `MSRMNP` | **468 3D points + presence score**. Direct native 9-station inner upper (`[78,191,80,82,13,312,310,415,308]`) & inner lower (`[78,95,88,87,14,317,318,324,308]`) lip margins + 6-point left/right EAR eye rings (`[33,160,158,133,153,144]`, `[362,385,387,263,373,380]`). Zero polyline interpolation needed. | ~`620–750 KiB` Flash (`w8a8` conv + `w8a16` head), `< 1.2 MiB` PSRAM scratch, estimated **~15–25 ms** dual-core on P4 @ 360 MHz (`~0.21 GFLOPs`). |
| **2. PFLD-0.25X (`68-pt` 300W/iBUG or `98-pt` WFLW)** | `polarisZhao/PFLD-pytorch` (`MIT` code; verify training-dataset commercial terms if redistributing weights) | `112×112` RGB face crop | **68 or 98 2D points**. Exact 6-point eye contours (`36..41`, `42..47`); 8 inner-lip points (`60..67` / `88..95`, 5 upper + 5 lower sharing corners) resampled piecewise-linearly along verified inner-lip segments via `adaptIbug68()`. | ~`320–700 KiB` Flash, `< 0.8 MiB` PSRAM scratch, estimated **~8–14 ms** dual-core on P4 @ 360 MHz. |

## 4. Why Direct Image Ridge / Dark-Valley Extraction Is Not a Validated Replacement

Scanning vertical luminance valleys between the 5-point detector's mouth corners (`keypoints[1]` and `keypoints[4]`) cannot reliably separate closed-lip shadow, facial hair, teeth specular reflections, tongue brightness, or off-axis head pitch. Under OpenToy's honesty rules, raw pixel ridge heuristics may only appear as developer diagnostics and **must never set `Detection::mouthValid = true`**.

## 5. Delivered Portable Adapter & Remaining Blocker

- **Delivered & Host-Tested (`core/LipLandmarkAdapter.h`, `tests/lip_landmarks.cpp`)**:
  - `adaptFaceMesh468()` and `adaptIbug68()` transform ROI landmarks to normalized camera coordinates, enforce `Geometry.h` `normalizeLips()` and `validEye()` invariants, compute `aperture` (`0` in lip-contact zone `<= 0.03`) and `bend` (`[-1, 1]`), and reject 5-point-only (`applyFivePointOnly()`), low-confidence (`< 0.60`), NaN/Inf, inverted, or out-of-frame inputs.
- **Exact Remaining Step Before Live Firmware Mouth Copying**:
  1. In an offline Python quantization workspace with `torch`, `onnx`, and `esp-ppq` installed, export MediaPipe `face_landmark` (`192×192`, Apache-2.0) to ONNX (replacing any TFLite custom `Landmarks2TransformMatrix` crop op with host/C++ ROI affine crop before inference) and quantize with `esp-ppq` (`target="esp32p4"`, `w8a8` backbone + `w8a16` final `Conv`/`Gemm` head) to `face_landmark_p4.espdl`.
  2. Link `face_landmark_p4.espdl` behind `HumanFaceDetect` on **Tab5** (which already has a working `SC202CS` live camera) and measure real P4 inference latency and lip-contact accuracy on live faces before enabling `mouthValid = true`.
