# OpenToy stopping checkpoint — November 17, 2026

**Latest bench result.** The USB reconnect restored the Tab5 camera on firmware 0.2.0. Fresh photos confirmed a four-bar diagnostic followed by Eyes 0.1.0 on the UNO Q. The 15,448-byte Eyes sketch matched its STM32 readback. UNO Q and the round Waveshare are parked with their verified Eyes builds installed. The owner reports the Tab5 screen is black. Tab5 was subsequently parked after its own camera captured the dark display in the mirror. 0.2.0 remains installed. The local 0.2.1 source was saved before build or install. All three devices are released.

**Earlier post-restart Tab5 attempt.** The previously installed 0.2.0 firmware and original recovery backup are preserved. No new camera frame arrived over the serial connection. JTAG responded during diagnosis. The bench's blank-screen report remains unresolved. Check the reflected Tab5 display in a fresh frame after the panel is visibly lit. See the [firmware build records](FIRMWARE-BUILDS.md) for the installed image digest and the [roadmap](ROADMAP.md) for open work.

The [current roadmap](ROADMAP.md) and [build index](FIRMWARE-BUILDS.md) include the later device work. The initial stop and subsequent hardware milestones follow chronologically.

Purpose: preserve current work and allow direct manual continuation in Antigravity, without a Codex monitoring loop. The [root matrices](../README.md) are the family overview; [roadmap](ROADMAP.md) lists unfinished work. This documentation update does not merge unfinished device code or deploy firmware.

**Earlier stop, before the Tab5 camera installation:** Google jobs and automatic dispatchers are stopped; the active queue is empty and saved for deliberate manual continuation. No Tab5 serial handle remains, its original app is intact, and it is safe to disconnect. No camera image or firmware write occurred. Tab5 work consists of research, two partial adapter headers and an installed ESP-IDF 5.4.2 toolchain, not a completed target build. UNO Q and Camera Eye implementation jobs had not started.

## Devices at the initial stop

| Track | Last established state | Not completed |
| --- | --- | --- |
| Three Reverse TFT Feathers | Curious / Dreamy / Cheeky Eyes 1.3.0 eco behavior and distinct blink patterns installed | Owner brightness acceptance, measured battery runtime and common-menu migration |
| Nesso N1 | Eyes 1.3.2; touch, smooth orientation, autonomous behavior and enlarged pupils | Shared-menu migration and other app ports |
| Round Waveshare 1.85C BOX-EN V2 | Eyes 1.1.0 installed; quiet reactions and common Menu | Google touch improvements passed host tests; 1.1.1 not installed or physically accepted |
| Waveshare RLCD 4.2 | Eyes 0.2.0: native smooth contours, black background, enlarged pupils, KEY-only Menu; appearance accepted | Physical KEY/menu acceptance and battery UI correction; audio/other ports |
| CrowPanel 9-inch P4 V1.0 | Copy-Face 0.2.0-proto, labelled simulation at stop; reboot returns live/neutral default; no camera fitted | Real camera/mouth inference; diagnostic 0.2.1 and standalone Eyes Menu 0.3.0 built but not installed |
| Tab5 | P4 target enrolled; full original recovery backup verified and original app rebooted | Camera firmware target build/install, first real image, Copy-Face and Camera Eye live behavior |
| UNO Q + 2.8-inch TFT | UNO Q confirmed and connected; TFT eyes requested | Shield identity/pins/voltage/touch compatibility and OpenToy implementation/deployment |
| XIAO ESP32-S3 / Sense | Optional companion; owner has camera-equipped Sense | No qualified CrowPanel camera transport or new deployment |

CrowPanel and RLCD were parked with no active hardware operations. Their previous source/recovery records remain authoritative. Full hardware identities, keys, original images, build outputs and raw logs remain private, outside Git. Tab5/UNO Q public pages are track descriptions, not released installers.

## Preserved source locations

- RLCD: [device branch](https://github.com/yury-g/OpenToy-Coursework/tree/device/waveshare-rlcd-4.2), checkpoint `d51472e`; installed source `aab1bba`.
- Copy-Face: [feature branch](https://github.com/yury-g/OpenToy-Coursework/tree/feature/copy-face), checkpoint `63b6388`; current real-camera work is unfinished.
- CrowPanel: [device branch](https://github.com/yury-g/OpenToy-Coursework/tree/device/crowpanel-p4), checkpoint `a90580f`; shared Menu candidate remains built-only.
- Round Waveshare: [device branch](https://github.com/yury-g/OpenToy-Coursework/tree/c0384472295069921210bd880cd742e57142e254), checkpoint `c038447`; later Google touch edits are preserved locally pending integration.
- Google source work, task prompts, conversation checkpoints and build attempts remain in the private development workspace. No unpublished partial implementation is represented as present on `main` by this documentation commit.

## Offloading assessment

Real Google execution occurred: family intake, round-screen touch implementation, deployment runner and offline firmware tooling, and Tab5 source/camera work. Reported independent validation includes two round-touch host suites, 15 runner tests and nine offline firmware-tool test groups. These do not establish a working Tab5 camera or a qualified universal flasher.

**Net OpenAI token savings have not been measured.** No comparable before/after baseline or attribution separates project work, setup and repeated coordinator monitoring. Moving development to Google did not make the continuing Codex conversation free. Do not claim a percentage saved from provider success or subscription remaining alone.

For the next session, the owner will paste one handoff directly into Antigravity. Antigravity should use saved files, keep one active owner per device/worktree, operate its UI if its harness blocks progress, and produce a compact result/checkpoint. Codex is not required to poll, supervise or relay status. Resume only when explicitly requested; no automatic queue restart is implied by this roadmap.

## Latest November 17 Antigravity continuation update

- **M5Stack Tab5 (`projects/tab5/` `0.1.1` -> `0.2.0`)**: Upgraded from headless Camera Eye `0.1.1` to multi-mode `0.2.0` (`cam_eye.bin`, original 16 MiB flash backup preserved outside Git), bounded-installed in `[0x10000, 0x30d000)` inside `[0x10000, 0x310000)` with whole-chip hardware MD5 and 1024-byte-ACK byte-for-byte readback. Live `HW`/`FACE`/`MIC`/`SNAP` qualified **`ST7123`** (`720x1280` MIPI-DSI `disp=1` + `0x55` capacitive touch `touch_fw=3`, `touch=1`), `INA226` (`0x2260`, USB-only `vbus_mv=1200` -> neutral `BAT --`), `PI4IOE5V6408` #2 (`0x44`), `ES8388` (`0x10`), `ES7210` (`0x40`, `0x1072` volatile 16 kHz RMS + `SoundWake` verified via `MIC 1` -> `MIC 0`, default OFF), `BMI270` (`0x68`, `0x24`), `RX8130` (`0x32`), `HumanFaceDetect` `0.5.2` (`MSRMNP_S8_V1` on `320x180`, `det_ready=1`, `mouth="unavailable_5pt_only"`), 3 local modes (`Camera Eye`, `Copy-Face`, `Animatronic Eyes 1`) with Common 7-entry Touch Menu and `0.8 s` Performance restore, and fresh `1280x720` `OTCAM1` `SNAP` (`age_ms=188`, `camera_eye_eval.py` `PASS`). Physical fingertip touch acceptance remains with the owner.
- **Copy-Face dense lip & eyelid feasibility (`feature/copy-face` `0.2.2-proto` source)**: Audited `esp-dl 3.3.15` operators/quantization (`LIP-MODEL-FEASIBILITY.md`), implemented portable C++14 `LipLandmarkAdapter.h` (`adaptFaceMesh468`, `adaptIbug68`, `applyFivePointOnly`) + `tests/lip_landmarks.cpp` (`check.py` PASS), and wired `applyFivePointOnly()` into `Camera.cpp` and Tab5 `FaceDetectorTab5.cpp`.
- **Shared `SoundWake.h` + Feather `1.4.0` & Nesso `1.4.0` candidates (`projects/animatronic-eyes/`)**: Implemented volatile-only acoustic wake & expressive listening (`SoundWake.h` with adaptive noise floor, 2-block debounce, Curious/Dreamy/Cheeky response curves, and speaker self-trigger holdoff), migrated Feather (`1.4.0`: `D1`/`D2` user buttons, `D0` BOOT excluded, stationary `Select+Start` `<0.8s` vs `>=0.8s` disambiguated) and Nesso (`1.4.0`: 360°-rotated `APP-STATUS-BAR` v1.0 + 2-page Common Touch Menu + `0.8s` Performance restore), passed all 16 C++14 host suites + upstream SHA digests, and verified real ESP32-C6 (`Nesso`) and ESP32-S3 (`Feather` Eyes 1 Curious & Eyes 2 Doodle) `arduino-cli` compiles outside Git while leaving installed `1.3.0`/`1.3.2` bench hardware untouched.
- **Vibe Coding Camera Eye (`projects/vibe-coding-camera-eye/`)**: Implemented C++14 grading core + Python provenance/crop/burst evaluator (`VERIFY.py` PASS). Captured and graded UNO Q 2.8-inch TFT and Round Waveshare (`1.1.0` and `1.1.1`) across both Tab5 `0.1.1` and `0.2.0`. All raw camera captures stay strictly local.
- **Round Waveshare `1.1.1`**: Integrated enlarged targets, 4-page Menu, 8 ms inter-strip polling, and `TOUCH` release telemetry (`worktrees/round`, synced to `main`). Passed full `tools/check.py`, Arduino-ESP32 build, bounded install, USB `?` telemetry (`version=1.1.1`), and live Tab5 `0.1.1`/`0.2.0` Camera Eye verification in Play mode. Physical fingertip tap feel and release-packet observation remain with the device owner.
- **UNO Q + 2.8-inch TFT (`0.1.0-src`)**: Portable C++14 large-pupil screen renderer and strict `ShieldDescriptor` / `NullUnqualifiedPanel` boundary pass host checks (`UNO Q` hardware untouched). Shield driver/pins/voltage remain blocked on physical back-of-shield markings/photos.
- **Battery UI fixes (`feature/copy-face`, `device/waveshare-rlcd-4.2` `0.2.1`)**: Replaced misleading empty-battery glyphs/rects with truthful `BAT --` / `BAT X.XXV` formatting and host tests; parked CrowPanel (`0.2.0-proto`) and RLCD (`0.2.0`) hardware untouched.
