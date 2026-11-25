# Building Battery Faces

Use Arduino-ESP32 3.3.10 and the libraries selected by `tools/build.sh`. Create the ignored private `firmware/FeatherFace/FaceLinkConfig.h` from its example; never commit credentials or compiled binaries. Run `tools/build.sh` without any USB connection required. It builds Feather, Nesso, Waveshare1.69, CYD, Tab5, StickS3 and reflectiveSilver profiles. Camera firmware is not built or active; its last working state is on [archive/camera-test-v17](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/archive/camera-test-v17).

The internal cache names preserve earlier board naming: `build-pink` is the physical StickS3, now **Orange**; `build-orange` is Nesso, now **Pink**. `build-silver` is the N16R8 ST7305 board; its logical200×150 image scales2× to400×300 landscape. Physical identity slots and MACs remain stable through the color swap.

## Installing

First obtain a complete original flash backup and matching hardware digest for each new device. Installers assert the enrolled MAC, verified backup and original application-slot capacity before writing only the app image. They preserve bootloader, partition table, OTA selection, NVS and filesystems.

- `tools/install_pair.py blue` / `green`: confirmed Feather app at0x10000. Green is replacement MACac:27:6e:b1:cb:b0, never the retired board.
- `tools/install_pair.py nesso`: Nesso/Pink profile, when USB-connected.
- `tools/install_pair.py yellow` / `rainbow`: existing Waveshare1.69 profiles.
- `tools/install_purple.py`: dualUSB ST7789 CYD, same-ROM-handle identity check.
- `tools/install_s3_member.py stick`: Orange StickS3, original1MiB factory slot; LCD rail writes preserve its M5PM1 charger byte.
- `tools/install_s3_member.py silver`: Silver, originalapp0 at0x10000, size0x330000; original OTA sequence1 remains selected.
- `tools/install_tab5.py`: app-only P4 update with the charger-preserving M5GFX patch applied by `tools/setup_m5gfx.py`. No C6 modem update or M5Unified Power initialization.

Never open a serial reader while an installer/backup owns the same port. Native diagnostic opens use port=None, RTSfalse/DTRtrue before opening, without1200-baud reset. Purple uses460800baud; opening its CH340 can reset it, so keep its diagnostic session open through verification.

## Verification

Run `python tools/run_host_tests.py` and `python -m unittest discover -s tests -p 'test_*.py'`. The host runner covers battery logic, new/retired identities, packet validation, relay source age, navigation, live tile bounds, face geometry and Silver's native monochrome packing/rotation. Adapt its compiler path on another machine.

`tools/verify_faces_family.py` is the current hardware verifier: own-face startup, Green/Silver readings on each connected peer and Tab5, face/trend browsing, captured screens and20seconds without USB reads. It records USB-unavailable members instead of claiming they were updated. It does not flash, start battery tests or access the retired Green MAC. Earlier `verify_family.py` and versioned JSON files describe historical configurations, not the current family.

`tools/compare_devices.py` reads fresh command-barrier status. A missing USB member is normally unavailable. A reading is not a battery health/capacity test. Silver/Orange/Yellow/Rainbow use voltage-derived estimates, whereas Blue/Green use MAX17048 and Pink/Nesso can report signed cell current.

[Silver manufacturer driver sources and native mapping](silver-rlcd-driver.md) · [Retired Green hardware hold](../GREEN-HEAT-HOLD.md).

## Qwiic accessories branch

Blue/Green use v26 and Tab5 uses v25 on `feature/qwiic-accessories`; other profiles retain v18. The SparkFun DA7280 driver is vendored under `firmware/FeatherFace/src/haptic` with its original notice. The local Wire bus is reused, and Qwiic Button writes stay in volatile LED registers. [Controls, deliberate hardware checks and limits](qwiic-accessories.md) · [Find Me](find-me.md).
