# Setup for an instructor or authorized AI agent

These commands make the workflow reproducible; students can ask their local AI agent to execute them and then use the native app. Routine use does not require Arduino IDE, code copying or student-operated terminal commands.

## Establish physical access

Check `uname -s`, `sw_vers` and the actual `/dev/cu.usbmodem*` ports on the target Mac. A cloud Linux executor cannot see a student's physical board or use the Mac's Bluetooth merely because it has “full permissions.” Run through a local Mac executor with USB and Bluetooth access. macOS Bluetooth and project-folder access may need approval for the launching app/runtime. The bench launcher ran through the local executor; background LaunchServices startup of the new app stalled while reading the Python environment, so normal first-launch behavior still needs investigation. Do not call the alias launcher a validated portable installer.

Use a data-capable USB cable. Identify the chip through Espressif's ROM tool before writing. This project targets XIAO ESP32-S3 with8MB flash; do not use the C3 board target. Inventory/preserve existing source and circuit documentation. For the OLED example, establish the controller and wiring before provisioning that firmware.

## Install pinned tools

Known bench environment: Arduino CLI1.4.1, Arduino-ESP32 core3.3.10 (ESP-IDF5.5.4), Python3.14, requirements below. Install Arduino CLI from [its official release](https://github.com/arduino/arduino-cli/releases/tag/v1.4.1) using the archive matching the Mac architecture, or an equivalent verified package. Do not silently accept unrelated license agreements as a workaround for unavailable tools.

From the repository root:

```sh
python3.14 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.10 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit SH110X@2.1.14" "Adafruit GFX Library@1.12.6" "Adafruit BusIO@1.17.4"
```

The Adafruit libraries are needed only for the OLED example. The build helper checks installed versions before compiling. Replacing versions creates a new test obligation; do not call an untested combination validated.

## Prepare one new board

Substitute its actual serial port. Inspect first:

```sh
.venv/bin/python tools/provision.py --project bare --port /dev/cu.usbmodemEXAMPLE
```

Then preserve the full original8MB image and generate local files:

```sh
.venv/bin/python tools/provision.py --project bare --port /dev/cu.usbmodemEXAMPLE --prepare
```

Choose `--project oled-touch` only for the documented class circuit. Preparation refuses an existing profile/key/backup. Each example represents one toy; use a separate checkout for another toy of the same kind. The helper reads flash through the ROM downloader without a stub, checks size, saves a SHA256 and generates a unique key/profile. It writes no flash. If interrupted, preserve and inspect partial artifacts before retrying.

For an already provisioned OpenToy toy, preserve/import its existing local profile and key. Generating a new key would make it differ from the installed updater. Keep original source and flash backups private; do not import them into tracked Git files.

## Compile and USB seed once

Bare example:

```sh
.venv/bin/python projects/xiao-toy/scripts/project.py build --label Seed
.venv/bin/python projects/xiao-toy/scripts/project.py upload-usb --label Seed
.venv/bin/python projects/xiao-toy/scripts/serial_check.py --seconds 5 --send status
```

Use `projects/xiao-class` for the OLED example. FQBN is `esp32:esp32:XIAO_ESP32S3:EraseFlash=none`. The upload helper identifies the expected ESP32-S3/factory MAC again immediately before writing. The initial USB seed writes needed bootloader/partition/application sectors; it does not issue a whole-flash erase. Inspect partition compatibility before reusing this process on a previously partitioned device that differs from the tested default8MB layout.

## Build/open the Mac app

```sh
.venv/bin/python projects/xiao-toy/scripts/build_mac_app.py
open "projects/OpenToy Control.app"
```

The app discovers local profiles for the bare/OLED examples. Connect checks the factory MAC through BLE. Live controls return acknowledgments. **Build + update** compiles selected-toy source, creates a manifest, transfers, reboots, reconnects and verifies the build label.

This is a local alias launcher. Keep the project and Python runtime at their paths. It is not a signed, portable installer and has not been validated on other Macs.

## Repeat tests

Host tests do not flash or control hardware:

```sh
.venv/bin/python tools/check.py
```

A C++ compiler is needed for pure parser/image-bound/retry tests. `CXX` can name a suitable compiler invocation. On the bench, Apple compiler access was unavailable and an independently verified Zig0.16.0 compiler was used; this does not require Zig for the firmware build.

With the corresponding toy present:

```sh
.venv/bin/python projects/xiao-toy/scripts/ble_client.py --test
.venv/bin/python projects/xiao-class/scripts/peripheral_test.py
.venv/bin/python projects/xiao-class/scripts/face_test.py
.venv/bin/python projects/xiao-class/scripts/eyes_test.py
.venv/bin/python projects/xiao-class/scripts/ota_negative_tests.py
```

The last test authenticates deliberately invalid images and writes only the inactive application slot. Run it only on an identified, backed-up test toy. It never intentionally activates those images. It uses the latest local built image as a fixture, so build first. Do not run multiple clients/tests concurrently against the same board.

CLI update alternative:

```sh
.venv/bin/python projects/xiao-class/scripts/ble_client.py --fast --upload projects/xiao-class/firmware/ToyBLE/build/Seed/ToyBLE.ino.bin
```

`--fast` uses four-packet flow-controlled groups. Without it, each data write requests a GATT response. Both modes were exercised on the bench. Retain the matching manifest beside the image. Stop controller streaming before diagnostics, manual controls or recovery tests.

## Inspect real controller input

After connecting a controller to the Mac, the instructor or authorized AI agent can record native input without changing the toy or controller firmware:

```sh
.venv/bin/python tools/gamepad_probe.py --seconds 30
```

The probe pumps the Cocoa main run loop so discovery and input reports can arrive. It prints a JSON record when controller state changes, including both sticks (`x/y`, `rx/ry`), D-pad, triggers and A/B/X/Y. An empty list is no controller; a stationary neutral record establishes recognition but does not prove live ranges. This probe performs no BLE writes. End-to-end qualification uses the native app and independent serial output, following CONTROLLERS.md.

`eyes_test.py` checks15 exact display-buffer hashes from `tests/eye_frames.json`,20 combined poses, atomic rejection and disconnect-to-neutral through real BLE and independent USB. `tests/eye_frame_reference.cpp` generates the corresponding reference command/hash pairs using the same bounded pixel geometry in a host framebuffer; regenerate reviewed fixtures when intentionally changing that rendering. The earlier `face_test.py` remains a compatibility test for the original no-pupil command.
