# CrowPanel V1.0 wiring evidence

Reviewed 2026-11-17. The owner identified the physical panel as DHE04209D V1.0.
Connector placement/orientation and the XIAO's exact hardware revision still
need inspection before designing a combined installation. This document does
not qualify direct insertion or simultaneous USB power with the XIAO inserted.

## Manufacturer sources

- [Elecrow 9-inch wiki](https://media-cdn.elecrow.com/wiki/CrowPanel_Advanced_9inch_ESP32-P4_HMI_AI_Display_1024x600_IPS.html): model, USB instructions, revision differences and vendor source link.
- [Elecrow V1.0 schematic](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/342a40719c98601fbd282f72e7fc2cd413133dd8/Eagle_SCH%26PCB/1.0/ESP32-P4%20Display%209.0%20inch%20V1.0.sch): primary connection evidence; parsed directly as EAGLE XML.
- [Elecrow versioned drawings](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/tree/342a40719c98601fbd282f72e7fc2cd413133dd8/Eagle_SCH%26PCB): V1.0, V1.1 and V1.2 compared; use V1.0 for this unit.
- [Seeed XIAO ESP32-S3 power pins](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#power-pins): 3V3 is regulator output; 5V is USB VBUS and requires an external diode arrangement when supplied externally.
- [Seeed linked schematic](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/new-res/202003751_XIAO%20ESP32S3_v1.4_SCH_260226.pdf.pdf): USB data terminates at the S3; header VBUS and 3V3_OUT are supply nodes. The download filename, sheet revision and internal filename disagree, so it does not identify the owner's XIAO revision.
- [Official WCH macOS driver](https://github.com/WCHSoftGroup/ch34xser_macos): driver installation/activation instructions and signed application.

## What the V1.0 drawing establishes

| Function | Drawing evidence | Consequence |
| --- | --- | --- |
| Panel programming USB | J1 USB data through R6/R7 to U1 CH340K; UART0 path to P4 | Use the panel socket labeled UART0 for initial ROM access |
| Other panel USB | J16 data goes to P4 DP/DM | Separate native USB interface; not the XIAO programming cable |
| Panel dual-USB supply | J1 VBUS passes D10/D11; J16 VBUS passes D007/D008 into panel supply nets | The two panel connectors have diode paths; this does not isolate an inserted XIAO's regulator |
| Wireless/add-on supply | J9 pin 5 directly on VDD_3V3; pin 6 GND | Shared panel supply, not an isolated companion power domain |
| Optional 5V at slot | J9 pin 7 on VDD5V_W; connection L4 to VDD5V marked **NC** | Do not assume the socket supplies 5V or that L4 is populated |
| Slot serial signals | J9 pin 1 through R49 (0R) to P4 GPIO53 TX; J11 pin 1 through R48 (0R) to P4 GPIO54 RX | Communication path, not an automatic USB programmer or reset/BOOT bridge |
| Slot SPI | J9 pin 2 SCK→P4 GPIO8; pin 3 MISO→GPIO7; pin 4 MOSI→GPIO6 | Future companion mapping must use actual contacts, not printed legacy GPIO names |
| Slot I2C | J11 pin 2 SCL; pin 3 SDA, on 3.3V-side I2C nets | Shared bus needs address, pull-up, ownership and stale-input checks |
| Other slot signals | J11 pin 5→P4 GPIO9; pin 6→GPIO10; pins 4/7 absent from net references | Do not treat every physical contact as a connected GPIO |

Pin numbers above are **schematic connector pin numbers**, not a wiring
orientation guide. Before insertion, match the board's silkscreen and physical
contacts to both manufacturers' pinouts. No USB D+/D− pair appears on J9/J11.

The reviewed V1.1/V1.2 drawings connect slot 3.3V through L3 marked 0R; V1.0
connects it directly. None of those connections is power-source isolation.
The V1.2 serial/expansion GPIO allocation also differs from V1.0. The wiki
mixes some older labels and examples; a generic P4 board profile is insufficient.

## Power decision

Keep the XIAO physically removed and give each board its own USB cable for
initial work. This keeps the two regulator outputs disconnected. A direct
socket installation may feed the XIAO through its 3.3V output pin, and adding
XIAO USB may parallel supplies. A fitted connector is not proof of a supported
power arrangement. Even panel-only power with an inserted XIAO remains
unqualified until that use and the exact XIAO revision are checked.

For a future companion, design and verify one power source per rail, common
ground, safe signal levels, correct orientation and behavior when either board
is off. Do not solder, cut jumpers or add power-isolation components based only
on this preliminary record. A separate-cable UART link with power contacts
excluded is a possible design, not a tested connection.

## Processor roles

- **P4:** native screen rendering, touch handling and app state. No integrated radio.
- **Factory C6:** existing onboard wireless companion; retain its firmware and
  use the revision-correct vendor transport. Wi-Fi examples do not prove every
  BLE or ESP-NOW feature needed by OpenToy is exposed to the P4.
- **External XIAO S3:** optional radio/controller/sensor bridge after a separate
  protocol and safe wiring exist. It does not automatically drive the panel,
  program the P4, add a motion sensor or make ESP32-S3 binaries run on P4.

## USB bring-up evidence

The physical Mac reports Darwin/arm64. The XIAO candidate was matched to its
USB descriptor, then independently verified with the ESP32-S3 ROM. The other
track's devices were excluded and no serial session was opened to them.

After the owner corrected the connection, the CrowPanel's bridge enumerated
as USB vendor/product **1A86:7522**, but no serial pathname appeared. No WCH
driver was enabled. WCH's official v2.1 application is Apple-notarized and
signed by Nanjing Qinheng Microelectronics; its driver explicitly matches
1A86:7522. The owner enabled the driver in macOS Settings and its serial port appeared.
P4 ROM then independently confirmed the expected target: ESP32-P4 revision 1.3,
16 MiB flash, security flags zero. The V1.0 PCB marking and chip revision 1.3
refer to different things. Large streamed reads failed. Independently checksummed
4 KiB reads, explicit flash-size setup and bounded retries completed two matching
16 MiB backups, each checked against the device digest. Small ROM and RAM-stub
reads also agree. The live partition table passed its MD5 check; its single
factory app and separate storage are preserved privately. Enrollment wrote no
flash; subsequent [Eyes 1 installation](EYES1-VERIFICATION.md) changed only the
application. Restoring the original backup has not been tested.

## Battery circuit

The exact V1.0 charger, connector polarity, divider and management-MCU routing
are recorded in [battery evidence](BATTERY.md). A working screen on USB does not
establish battery charge level or charging current.
