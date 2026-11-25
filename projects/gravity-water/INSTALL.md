# Put Gravity Water on a Nesso

## Already using the playtest device?

The accepted **v0.3.0** is already installed. You do not need to download or flash
anything to keep playing. Tilt, hold the yellow button, touch, or shake.

## Download the source

Get the [v0.3.0 release](https://github.com/yury-g/OpenToy-Coursework/tree/gravity-water-v0.3.0).
The focused `Gravity-Water-v0.3.0-source.zip` contains this chapter with its
firmware, artwork, tests and guides. GitHub also provides the whole repository's
source archive. Both preserve the `projects/gravity-water` directory layout.
GitHub access is required while OpenToy is private.

You need an **Arduino Nesso N1**, a USB data cable, and a computer with
Arduino CLI installed. This chapter targets the Nesso's ESP32-C6 and its built-in
screen, touch controller, motion sensor, button and buzzer. Other boards are not
supported by this release.

## Build locally

Use these tested versions:

| Component | Version |
| --- | --- |
| Arduino-ESP32 core | 3.3.10 |
| M5Unified | 0.2.18 |
| M5GFX | 0.2.25 |

Install the ESP32 core through your Arduino setup, and install the two libraries.
With Arduino CLI configured for Espressif's board package, the pinned installs are:

```sh
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install 'M5Unified@0.2.18' 'M5GFX@0.2.25'
```

From the extracted archive's top-level folder, compile to a private folder
outside the source checkout:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:arduino_nesso_n1:PartitionScheme=app3M_fat9M_16MB \
  --build-path /absolute/private/gravity-water-build \
  projects/gravity-water/firmware/GravityWaterNesso
```

The app image is `GravityWaterNesso.ino.bin` in that build folder. Keep compiled
images and device backups private; this release deliberately distributes source.

## Preserve the device before installing

Installation replaces the running app. The tested workflow preserves the
bootloader, partition table, OTA selection and filesystem.

1. Identify the connected USB port, ESP32-C6 chip and intended device identity.
2. Save the existing flash and working app before replacing anything. Keep two
   matching full-flash reads for recovery.
3. Read the device's live partition table and OTA selection to identify its
   active app slot and capacity. The compile menu's layout is **not evidence**
   that another board already has that layout.
4. Write **only** the compiled app to that verified slot with esptool, then run
   an independent `verify-flash` against the same image and offset.
5. Reboot and check the battery bar, movement, button, touch and sound.

The playtest unit was verified to use app0 at `0x10000`, with `0x300000` capacity.
That address belongs to the tested unit's layout. **Do not assume it for another
board.** Avoid an automatic whole-flash upload or erase operation when preserving
an existing Nesso installation. If you cannot identify its active slot, stop at
the successful build and have an experienced installer inspect the board.

## First play

Stand the Nesso on its bottom edge. The beads should collect at the bottom.
Hold the yellow button: the pool should boil and warm in color. Release it:
heat should fall quickly. Drag a finger through the pool, then give it a shake.
Listen for the generated texture, and leave it still to let everything settle.

[Full controls](README.md#your-hands-are-the-controls) ·
[Validation and limitations](BENCH.md) · [Developer notes](DEVELOPMENT.md)
