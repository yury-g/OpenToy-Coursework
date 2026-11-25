# OpenToy on Arduino Nesso N1

Nesso runs the OpenToy Face v2 renderer on its built-in 135 × 240 display. Tap the left, center, or right third of the screen to tickle that side of the face. Turn the device upright for the gravity-driven face orientation. Shake it for a startled, then dizzy pose. The two programmable buttons shift the shade of orange.

The Nesso chapter combines the [Orange Face](https://github.com/yury-g/OpenToy-Coursework/tree/feature/orange-face/projects/orange-face) reactions and shared OpenToy curves with the [Face v2 command format](../../docs/PROTOCOL.md#live-commands). The [Nearby Buddy](https://github.com/yury-g/OpenToy-Coursework/tree/feature/nearby-buddy/projects/nearby-buddy) pair uses a separate, enrolled BLE proximity protocol.

| Feature | Nesso support |
| --- | --- |
| Display and animation | M5Unified/M5GFX with the shared OpenToy procedural bitmap renderer |
| Local inputs | Three touchscreen zones, two shade buttons, and onboard BMI270 motion |
| Cross-board control | Shared Face v2 `doodle` command over OpenToy BLE UART and USB. The optional Mac relay can forward Nesso touch poses to a Face v2 OLED class board. |
| Firmware updates | USB using the ESP32-C6 build. The XIAO's authenticated BLE firmware update service is specific to that target. |

## Build and load

You need Arduino CLI, Espressif Arduino-ESP32 **3.3.10**, **M5Unified 0.2.18**, and **M5GFX 0.2.25**. These are the versions used on the development Mac. See [Arduino's Nesso N1 manual](https://docs.arduino.cc/tutorials/nesso-n1/user-manual) and the [M5Stack board guide](https://docs.m5stack.com/en/core/Arduino_Nesso_N1).

From this directory:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:arduino_nesso_n1:PartitionScheme=app3M_fat9M_16MB \
  --build-path build firmware/NessoFace
```

Before modifying a board, identify the ESP32-C6 and expected factory MAC over USB, then preserve its original flash. Check that the backup matches a second full read. Confirm the partition table and currently selected application before writing. On the tested Nesso, the original 16 MiB layout had two 3 MiB application slots and selected the one at `0x10000`. This bench install wrote only that app slot. Other boards can select a different slot. Keep the backup and compiled images private.

For recovery, [enter download mode](https://docs.m5stack.com/en/core/Arduino_Nesso_N1#download-mode), verify the same hardware identity and restore its own saved flash. The backup was read and verified. Full restore has yet to be exercised on this unit.

## Live commands and the family link

Open a USB serial monitor at **115200 baud** and send a line with New Line or end commands with a semicolon.

| Command | Behavior |
| --- | --- |
| `status` | Firmware, frames, motion samples, touch/button counts, and BLE health |
| `doodle 20 -30 50 15 82 99 0 happy` | Send the same pose the Face v2 OLED chapter understands |
| `local` | Return to touch and motion expressions |
| `touch off` / `touch on` | Turn touchscreen reactions off or back on |
| `reset` | Return to local pose control and re-enable touch |

BLE advertises the OpenToy UART service as **OpenToy Nesso XXXX**, where XXXX is the last four hex digits of its factory MAC. The standard Device Information serial-number characteristic provides the full identity. Live controls are unpaired, as in the earlier chapter.

Install Bleak in a Python 3.11+ environment. Replace the example device names and MACs with your boards' own values. The Mac must have Bluetooth enabled and permission to use it.

```sh
python tools/live_face.py --target 'OpenToy Nesso BEEF' 02:00:00:00:00:0c \
  --command 'doodle 20 -30 50 15 82 99 0 happy'

python tools/live_face.py --follow 'OpenToy Nesso BEEF' 02:00:00:00:00:0c \
  --target 'Your OLED class toy' 02:00:00:00:00:0d
```

Use `--target` more than once to send a pose to multiple Face v2 boards, or to have multiple boards follow one Nesso. The relay verifies each connected board's factory identity before sending any commands.

## Bench check · November 15, 2026

Nesso v0.1.1 booted on the attached ESP32-C6 after an app-only USB write with
verified readback. Serial telemetry reported the display initialized, frame
and BMI270 sample counts rising, and a battery reading. A Mac Bluetooth
client verified hardware identity, sent a Face v2 pose, read the exact pose
back and reconnected. An out-of-range pose was rejected over USB and BLE.
Host checks covered touch-zone mapping and command parsing. A synthetic
Nesso event exercised the Mac relay's forwarding path. During a 25-second
run the device registered four touchscreen events and six button presses.
The screen's visual response and the orientation changes still need a bench
check. A second OLED toy was unavailable for a physical relay test.

## Credits

The shared bitmap renderer and command parser come from OpenToy Face v2. Orientation, touch reactions and color steps come from Orange Face. Original OpenToy source is MIT-licensed. The M5 libraries and Arduino core are external dependencies with their own licenses.
