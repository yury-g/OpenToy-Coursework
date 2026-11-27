# Waveshare ESP32-S3-Touch-LCD-1.69

Two bench boards were identified and backed up on 2026-11-27. ESP32-S3 identity, touch and QMI8658 communication were verified; the exact PCB revision still needs a physical check. This is the rectangular **touch** model, not the LCD-only board or the round 1.85C.

[Curious + Gravity Water firmware](../../projects/waveshare-1.69/README.md) (PLA enclosure CAD remains in historical OpenToy checkpoint `836d5ff`; not ported here)

## Manufacturer sources checked 2026-11-27

- [Product documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.69)
- [Resources and schematics, including V2.1](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.69/Resources-And-Documents)
- [Example source](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.69) (checked revision `f24f10d53d4cff5c2e2ac47a2e23da39a82454a6`)
- [Dimension drawing](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.69/esp32-s3-touch-lcd-1_69_size.pdf)
- [Manufacturer STEP model ZIP](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.69/ESP32-S3-Touch-LCD-1_69.zip)

The referenced upstream STEP model (`ESP32-S3-Touch-LCD-1_69.stp`, SHA-256 `e9f1555b02edee55548239b96e9a6610e9e49dbd6bb9bf6b85617b33687cb314`) is not redistributed here or relicensed. Model fit does not prove revision compatibility with the photographed boards.

| Function | Manufacturer mapping |
| --- | --- |
| MCU / memory | ESP32-S3R8, 8 MB PSRAM, 16 MB flash |
| Display | ST7789V2, 240 × 280; SPI CLK 6 / MOSI 7 / CS 5 / DC 4 / reset 8 / backlight 15 |
| Touch | CST816T at I2C 0x15; SDA 11 / SCL 10 / reset 13 / IRQ 14 |
| Motion | QMI8658C on shared SDA 11 / SCL 10 (`SAO=VCC3V3` -> `0x6B` in V2.1 schematic; `0x6A` also probed with `WHO_AM_I=0x05`); `INT1` GPIO 38; enabled in `0.2.0-dev` (physical tilt orientation pending owner verification) |
| RTC | PCF85063 on shared SDA 11 / SCL 10, `INT` GPIO 39; not enabled by current firmware |
| Power | PWR sense 40 / hold 41; battery ADC 1 with 3:1 divider |
| Buzzer | GPIO 42 (driven LOW / silent in current firmware) |
| Recovery | BOOT GPIO0, RESET CHIP_PU |
| Connectors | USB-C, rechargeable main battery, RTC battery, edge expansion pads |

The glass measures 33.13 × 41.13 mm in the drawing/model. Keep the vendor model coordinates for CAD: glass center Y = -1.273 mm, front toward +Z, buttons on +X. The three modeled button centers are Y = -9.758 / -0.846 / 8.065 mm, Z = -3.2 mm. In portrait `rotation(0)` (`240 × 280`), the PCB back silkscreen QMI8658 triad shows `+X` toward the PCB antenna (`C3`, screen top), `+Y` toward the side buttons (`RST`/`BOOT`/`PWR`, screen right), and `+Z` out of the PCB back; `BoardAxes.h` in 0.2.1-dev maps `(ax, ay)` to screen-down `(ay, -ax)`, reversing both directions after the owner reported the earlier mapping backwards. The corrected physical tilt recheck remains pending. Confirm switch labels, actuator positions and travel on the physical revision before finalizing tolerances.
