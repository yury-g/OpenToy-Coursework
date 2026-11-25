# Firmware seeds and complete builds

Use this index to find the public source for a particular device, check whether a target was built or installed, and match a locally retained image to a build. The [build inventory](firmware-builds.csv) records sizes and full SHA-256 digests for retained application images and Arduino merged images from this bench. Build labels include experiments and intermediate revisions. Installation status comes from the device records below.

## USB seeds and earlier projects

The original bare XIAO and OLED class toy each have a complete sketch, example device profile, licensed libraries or dependencies, build helper and tests. Start with the [bare XIAO project](../projects/xiao-toy/README.md) or the [OLED class toy](../projects/xiao-class/README.md), then follow [Compile and USB seed once](SETUP.md#compile-and-usb-seed-once). An Arduino file ending in `.ino.bin` contains the application. A `.ino.merged.bin` file combines multiple flash regions. They have different contents and are catalogued separately. Each provisioned XIAO uses its own local profile and signing key. Device-specific images, keys and recovery dumps stay in the private archive.

The [source releases](https://github.com/yury-g/OpenToy-Coursework/tags) preserve eye puppetry, doodle puppetry, visual design history and Gravity Water. The [project directory](../projects/README.md#projects-and-source-locations) also links the Orange Face, Nearby Buddy, Battery Faces and Nesso Face sources, including their separate repositories and feature branches. The [October checkpoint](STOPPING-POINT.md) identifies the earlier class-toy image by digest.

## Device builds and source stages

| Device | Full source and build entry | Last recorded hardware or build result |
| --- | --- | --- |
| Feather Eyes 1 and 2 | [Sketch, shared behavior and Arduino build](../projects/animatronic-eyes/README.md) | Curious, Dreamy and Cheeky 1.3.0 deployed. Feather Eyes 1 Curious and Eyes 2 Doodle 1.4.0 compiled for ESP32-S3. Those menu candidates await deployment. |
| Nesso N1 | [Touch/orientation firmware](../projects/animatronic-eyes/NESSO.md) | 1.3.2 deployed. The ESP32-C6 1.4.0 menu candidate compiled. |
| Round Waveshare | [Board setup, renderer and controls](../projects/animatronic-eyes/WAVESHARE.md) | 1.1.1 installed. Tab5 captured the enlarged menu and a burst of eyes moving in Play. Physical tap feel remains open. |
| Reflective Waveshare | [RLCD device source](https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/waveshare-rlcd-4.2) | Eyes 0.2.0 deployed with native contours and KEY menu. The 0.2.1 battery-label candidate passed host tests. |
| CrowPanel P4 | [Standalone Eyes](../projects/crowpanel/README.md) and [Copy-Face source](https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/copy-face) | Eyes 0.2.2 previously deployed, 0.3.0 menu candidate built. Current installation is Copy-Face 0.2.0-proto with a simulation available. The 0.2.1 diagnostic image was built. Camera input and mouth tracking on this board remain open. |
| M5Stack Tab5 | [Firmware and board guide](../projects/tab5/README.md), [ESP-IDF build helper](../projects/tab5/tools/build.py), [Camera Eye evidence tools](../projects/vibe-coding-camera-eye/README.md) | 0.1.1 delivered remote camera capture. The 0.2.0 multi-mode build added display/touch, gaze tracking and battery telemetry. USB snapshots of both targets passed. See the latest [display checkpoint](ROADMAP.md) for the on-device preview. |
| Arduino UNO Q with 2.8-inch TFT | [Native sketch, write-only display driver, portable renderer and tests](../projects/uno-q/README.md) | The STM32 color diagnostic and Eyes 0.1.0 were built and installed. Readback and fresh Tab5 photos confirmed both stages on the bench shield. |

For Feather and Nesso, the build helpers are [Feather](../projects/animatronic-eyes/tools/build.py) and [Nesso](../projects/animatronic-eyes/tools/build_nesso.py). The [round Waveshare build](../projects/animatronic-eyes/tools/build_waveshare.py) targets a different screen and audio revision. Tab5 uses ESP-IDF 5.4.2 and its own component versions and partition layout. Consult the corresponding board guide before selecting a target.

## Checking the UNO Q source stage

From the repository root, compile and run the portable renderer tests with a host C++14 compiler. This exercises shield qualification, gaze, pupil geometry, blink behavior and battery display independently of the native sketch and the optical check.

```sh
c++ -std=c++14 -Wall -Wextra -Werror \
  -Iprojects/animatronic-eyes/firmware/AnimatronicEyes \
  -Iprojects/animatronic-eyes/firmware/NessoEyes \
  projects/uno-q/tests/uno_q_eyes_test.cpp -o /tmp/opentoy-uno-q-eyes
/tmp/opentoy-uno-q-eyes
```
