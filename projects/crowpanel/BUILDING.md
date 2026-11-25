# CrowPanel build baseline

Checked on the physical Apple Silicon Mac on 2026-11-17. This is a
**manufacturer-example build**, not an OpenToy port or an installed demo.

For the installed OpenToy port, use the [Eyes 1 build guide](EYES1.md).

## Reproducible starting point

Use Elecrow source commit `342a40719c98601fbd282f72e7fc2cd413133dd8`, folder
[`example/V1.0/idf-code/Lesson07-Turn_on_the_screen`](https://github.com/Elecrow-RD/CrowPanel-Advanced-9inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/tree/342a40719c98601fbd282f72e7fc2cd413133dd8/example/V1.0/idf-code/Lesson07-Turn_on_the_screen).
Keep source downloads, dependency caches and build output outside OpenToy Git.
Preserve the vendor's source and license notices when adapting its drivers.

| Dependency | Baseline used |
| --- | --- |
| ESP-IDF | v5.4.2, target esp32p4 |
| ESP-IDF Python environment | Existing Python 3.9.6 environment |
| esp_lcd_ek79007 | 1.0.3 |
| esp_lvgl_port | 2.6.1 |
| LVGL | 8.4.0 |
| cmake_utilities | 0.5.3 |

The dependency versions above come from the vendor lockfile and completed
build. Use the ESP-IDF environment's Python when sourcing `export.sh`; the
Mac's newer default Python selected a nonexistent IDF environment on the first
attempt. Native Apple build-tool discovery used the installed Command Line
Tools directory, without accepting or changing the full Xcode license.

From that environment, build only:

```sh
idf.py -C /private/path/vendor-screen -B /private/path/vendor-screen-build build
```

Result: application image **573,312 bytes**, valid image checksum/digest,
ESP32-P4 chip ID, DIO / 80 MHz / 16 MiB header, chip revision range 0.1–1.99.
The identified panel processor is revision 1.3. This establishes a compile
baseline and chip-revision header compatibility only; display, touch, backlight and runtime
behavior remain untested. The sample's incidental Git-derived app version is
not an OpenToy release version and must not be reused as one.

## Do not use the sample's upload command

The sample builds a partition layout with three 0x3f0000 application slots and
OTA metadata. The live panel instead reports one **11 MiB factory application**
and a separate **4 MiB storage** partition, with different NVS boundaries.
Its printed whole-device flash command includes bootloader, partition table
and OTA initialization writes. Those writes would replace existing recovery
and storage assumptions.

Future OpenToy builds must account for the verified live layout and pass the
[application-only recovery checks](README.md#recovery-before-replacement).
Fitting the image in the application slot alone is insufficient: preserve the
bootloader, configuration and filesystems, and check the application's own
partition expectations before installation.
