# The boards

| Face | Manufacturer / model | Screen | Battery reporting |
|---|---|---|---|
| Blue | Adafruit Feather ESP32-S3 Reverse TFT | 135×240 portrait | MAX17048 estimated SOC and cell voltage; gateway |
| Orange | Arduino Nesso N1 | 135×240 touch | BQ27220 SOC, voltage, signed cell current; AW32001 charger status |
| Yellow | Waveshare ESP32-S3-Touch-LCD-1.69, printed-model revision | 240×280 touch | ADC voltage and generic voltage-derived **~SOC** |
| Rainbow | Second Waveshare ESP32-S3-Touch-LCD-1.69, same printed-model revision | 240×280 touch | ADC voltage and generic voltage-derived **~SOC** |
| Purple | ESP32-2432S028 CYD, classic ESP32, micro-USB + USB-C | 240×320 resistive touch | USB only; no battery/gauge attached |
| Pink | M5Stack StickS3 | 135×240 portrait | M5PM1 voltage, generic **~SOC** |
| Teal | Seeed XIAO ESP32-S3 Sense | Headless camera | No onboard battery measurement |
| Monitor | M5Stack Tab5 | 1280×720 touch display; onboard SC2356/SC202CS camera | Shows source readings; does not measure their batteries |
| Green — retired | Second Adafruit Feather ESP32-S3 Reverse TFT | Cracked, held off in previous firmware | Excluded after renewed ESP-chip/board heat; keep USB and LiPo disconnected |

These are nine physical boards: five active battery reporters, the USB-only Purple companion, the Teal camera, one monitor, and the retired Green board. Preserve Green's identity/index; do not reuse it for Rainbow.

## Manufacturer design references

- [Adafruit Reverse TFT documentation](https://learn.adafruit.com/esp32-s3-reverse-tft-feather/downloads), [official PCB repository](https://github.com/adafruit/Adafruit-ESP32-S3-Reverse-TFT-Feather-PCB). Local original Eagle board: [adafruit-reverse-tft.brd](references/adafruit-reverse-tft.brd), CC BY-SA 3.0, copyright/credit Adafruit Industries; [license](references/adafruit-hardware-license.txt).
- [Arduino Nesso N1](https://docs.arduino.cc/hardware/nesso-n1/), [official CAD archive](https://docs.arduino.cc/static/589c38114821d4417fb0f3418b0325e6/TPX00227-cad-files.zip). Unmodified [local archive](references/nesso-n1-official-cad.zip) includes original license: CC BY-SA 4.0, Arduino. Contains manufacturing CAD, not a claim of a complete enclosure STEP model.
- [Waveshare 1.69 documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.69), [revision/pin FAQ](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.69/FAQ), [official hardware repository](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.69). Local [V2.1 schematic](references/waveshare-169-v2.1-schematic.pdf) is a manufacturer reference, not mechanical CAD. Hardware licenses/copyright remain with Waveshare.
- [CYD community hardware pin reference](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md), including the original documentation links in that project. A [local copy](references/cyd-community-pins.md) and its [MIT license](references/cyd-community-license.txt) retain Brian Lough’s attribution. This is a community reference rather than a manufacturer model certification. Its dual-USB panel uses the [CYD2USB ST7789 configuration](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/DisplayConfig/CYD2USB/User_Setup.h), retained [locally](references/cyd2usb-display-config.h). Purple’s original factory app partition is only 1 MiB; the installer enforces that size and preserves the partition table.
- [M5Stack Tab5 documentation and dimensional references](https://docs.m5stack.com/en/core/Tab5). No manufacturer STEP file is bundled.

The README's SVG board drawings are original, editable **illustrations**, with real firmware framebuffer inserts. Component positions are simplified; they are not fabrication drawings, photographs, or precision CAD renders. Screen aspect ratios match their native pixel dimensions. Third-party design files retain their respective licenses; project MIT terms do not relicense them.


## Pink — M5Stack StickS3 (October 20)

ROM-matched MAC `02:00:00:00:00:08`; ESP32-S3-PICO-1, 8 MB flash and 8 MB OPI PSRAM. Native ST7789P3 135×240 portrait display: MOSI39, SCLK40, CS41, DC45, reset21, backlight38. Buttons A/B GPIO11/12. Panel rotation2 corrects the upside-down orientation reported by the owner; owner confirmed it upright. Logical screen remains135×240. M5PM1 at I²C0x6e, SDA47/SCL48; voltage is little-endian millivolts at0x22. LCD rail initialization masks only GPIO2 controls; charger/power configuration0x06 is read before/after and preserved (observed0x17). No M5Unified Power.begin or charger reset. SOC is the shared generic voltage curve, explicitly approximate; current and charger-done stay unknown. Original factory application is only1MiB at0x10000 despite8MB physical flash, so installer enforces that original limit.

[Manufacturer board documentation, schematic and structure files](https://docs.m5stack.com/en/core/StickS3) · [M5PM1 manual](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1207/M5PM1_Datasheet_EN.pdf). The included SVG is an original illustration, not precision CAD.

## Teal — Seeed Studio XIAO ESP32-S3 Sense (October 20)

ROM-matched MAC `02:00:00:00:00:0b`;8MB flash and8MB OPI PSRAM; installed sensor PID0x3660 (OV3660). No LCD and no onboard firmware-readable battery voltage/gauge. Reports camera presence with `valid=0`, never invented SOC. Camera pin map, RGB format and limits are in [camera transport](../docs/camera-transport.md). Original OTA sequence2 selects app1 at0x1d0000, size0x1c0000; preserve that selection/table/bootloader/NVS. First and stopped-app digest-verified full backups are kept locally;65 changing bytes were confined to one original NVS sector before the stopped snapshot.

[Official camera guide](https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/) · [Official board capabilities](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/). No microphone or SD capture is initialized. Camera preview exists in RAM; only explicitly requested demonstration screen captures are archived.


## Tab5 onboard camera — October20

The installed Tab5 identifies its native camera as PID`0xeb52` at SCCB`0x36`. Camera reference clock is24MHz onGPIO36; one576Mbps MIPI lane captures1280×720 RAW8 through the P4 ISP. Network previews retain16:9 as an80×45 letterbox inside80×60RGB332. Native sensor exposure/color processing is preliminary; this is an experimental second view. The shared MIPI LDO and display backlight remain owned by M5GFX. Camera code modifies only expander0x43’s camera-enable bit and never charger expander0x44. [Driver sources and pinned manufacturer references](../docs/tab5-camera-driver.md).
