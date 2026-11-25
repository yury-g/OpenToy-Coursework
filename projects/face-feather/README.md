# Face v1 and Face v2 on Reverse TFT Feather

> Continued as the OpenToy [Animatronic Eyes 1 and 2](https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/animatronic-eyes) project, named by the owner November 15, 2026. This folder preserves the OpenToy port checkpoint; use the linked OpenToy project folder for current builds.

Direct puppetry with an Adafruit Feather ESP32-S3 Reverse TFT (#5691) and Mini I2C Gamepad QT (#5743), connected with STEMMA QT. No Mac/Xbox/BLE relay is needed for play. Both remain active Feather projects. The saved `v1.0.0-eyes` and `v2.0.0-face` releases remain unchanged.

The display is landscape, 240×135. The original 128×64 monochrome face is enlarged to 240×120 with nearest-neighbor sampling and a 15-pixel status strip. `FACE V1` uses a cyan label; `FACE V2` uses orange. A small battery icon and gauge percentage sit at the top right, matching Gravity Water; `--` means the gauge is unavailable. The percentage is the MAX17048 estimate, not a measured battery-capacity test. The original eye geometry and procedural doodle springs/curves come directly from `projects/xiao-class/firmware/ToyBLE` at build time.

## Controls

All directions refer to the displayed face. Hold a modifier to use another layer; release returns that layer's continuous offsets to rest. Emotion choices stay selected until another choice or controller loss.

| Gamepad | Face v1 — eyes | Face v2 — doodles |
| --- | --- | --- |
| Stick | Move pupils | Up/down bends eyes; left/right bends brows |
| Hold Select + stick | Up/down moves white eye shapes | Up/down curves mouth; left/right pulls mouth sideways |
| Hold Start + stick | Left/right winks that eye; up closes both from bottom; down closes from top | Up opens jaw; left/right winks that eye |
| Hold Select + Start | Close both eyes | Blink both eyes |
| A | Happy | Happy |
| B | Angry | Angry |
| X | Surprised | Surprised |
| Y | Neutral | Sorry |
| Hold Select + tap A | Happy | Nervous |
| Hold Select + tap B | Angry | Neutral |

The single stick cannot manipulate all original Xbox axes simultaneously. These held layers preserve access to the original face parameters. V1 left/right eye-shape translation remains unused, as in the original mapping. Jaw and winks share the stick here, rather than independent analog triggers. Happy retains its original 25% jaw opening and Surprised 92%; Start + stick up adds to that opening. A roughly 12.5% center dead zone suppresses stick noise. Onboard Feather buttons are unassigned.

## Build

Pinned board core: Arduino-ESP32 2.0.17, `adafruit_feather_esp32s3_reversetft`, GNU C++14. Libraries: Adafruit GFX 1.12.6, BusIO 1.17.4, ST7735/ST7789 1.11.0, seesaw 1.7.9. Use an external build directory; the helper stages canonical renderers and the Gravity Water bounded telemetry implementation without copying their source into this project.

```sh
python3 projects/face-feather/tools/build.py --version 1 \
  --config-file /absolute/path/to/arduino-cli.yaml \
  --build-path /private/path/build-v1
# Repeat with --version 2 and a different build directory.
```

On Apple Silicon, older Arduino packages may contain Intel-only ctags/esptool binaries. The helper accepts repeated `--build-property` arguments for `runtime.tools.ctags.path` and `tools.esptool_py.path`; use installed native tools or a Python esptool wrapper. Do not replace another project's ESP32 toolchain.

For the two November 15 bench boards, convert the resulting ELF using the verified TinyUF2-compatible application header:

```sh
python -m esptool --chip esp32s3 elf2image --flash-mode dio \
  --flash-freq 80m --flash-size 4MB --max-rev-full 0 \
  --elf-sha256-offset 0xb0 -o /private/path/face-compatible.bin \
  /private/path/build-v1/FaceFeather.ino.elf
```

This builds an application only. **Do not use stock Arduino Upload or replace the generated partition table.** Identify each board through ROM, inspect security and flash size, preserve a new full-flash backup with a matching hardware digest, decode its partition/OTA selection, and bound the write to that selected application's original slot. These two boards have different app capacities. Verify the app after writing and verify all flash outside the sector-aligned write range remains unchanged. Keep backups, identity records, compiled files and raw logs outside Git.

## Runtime and checks

Gamepad reads run on core 0, rendering on core 1 at a target 30 frames/s. The driver verifies I2C address 0x50 and product 5743 before configuring pins: X ADC14, Y ADC15, active-low A5/B1/X6/Y2/Start16/Select0. Each read is checked, the bus has a 15 ms timeout, and missing controllers are retried every second. Invalid reads or samples 100 ms old clear continuous inputs and the emotion to neutral. Reconnection resumes local control. Doodle geometry eases toward neutral through its original spring.

TFT uses the correct board-variant pins, power rail high, rotation 1, SPI 24 MHz, byte-swapped RGB565. USB output uses the existing bounded Telemetry queue, with writes limited to `availableForWrite`; no blocking `printf` calls occur.

`python tools/check.py` includes the original eye/doodle suites and the Feather control mapping/freshness checks. Set `CXX` if the default compiler is unavailable. Serial `?` requests telemetry; `!` pauses input sampling for 1.5 seconds to exercise the stale-input path and automatically resumes. Neither command changes flash. There is no wireless update service in this local-control port; future installation uses the same identity-checked app-only procedure.

See [verification](VERIFICATION.md) for measured device results and outstanding physical observations. Historical Mac/Xbox guides: [Face v1](../../docs/V1-EYES.md), [Face v2](../../docs/V2-FACE.md).
