# Rebuild the current family

Tested with Arduino CLI, Espressif ESP32 Arduino core **3.3.10**, Adafruit GFX **1.12.6**, BusIO **1.17.4**, ST7735/ST7789 **1.11.0**, MAX1704X **1.0.3**, NeoPixel **1.15.4**, XPT2046_Touchscreen **1.4**, and M5GFX **0.2.25**. Core profiles select each board's installed flash/PSRAM/USB configuration. Python tools require pyserial and esptool 4.11; artwork also uses Pillow. Host C++ tests need a C++17 compiler (the original machine uses its existing Zig compiler).

1. Install those Arduino libraries and the ESP32 core.
2. Copy `firmware/FeatherFace/FaceLinkConfig.example.h` to `FaceLinkConfig.h`, replacing the SSID/password with unique private values. This local file and binaries are ignored because they contain the network credentials.
3. Run `sh tools/build.sh`. `tools/setup_m5gfx.py` copies the installed library into ignored `vendor/M5GFX` and applies the included, reviewable patch. The patch preserves Tab5's existing charger control pins before any display-expander writes; do not replace it with an unpatched library for this build.
4. Enroll each board's MAC in `PeerBattery.h`, the native Waveshare identity selection, and identity-checked host installers. The shipped registry is this owner's hardware; it is not open enrollment for arbitrary nearby radios. Wave units share the same binary; local identity selects Yellow or Rainbow. Stable wire index 1 stays reserved for retired Green.
5. Before installation on new hardware, make and digest-verify a private full-flash backup, inspect its partition table and active app slot, and confirm its exact board revision. Host installers intentionally require these local backups. They write only the verified active application slot, not a merged flash image. No erase or charge-test start is needed. Do not run Green's historical tools; Green remains on a heat hold.

Use `tools/compare_devices.py` for fresh command-barrier readings on USB-connected members, including camera diagnostics; missing USB members are reported as unavailable. Use `tools/verify_camera_family.py` for current Pink routing, two camera feeds, shared grayscale/color controls, default camera pages, source-loss recovery and no-reader checks. Historical v15 verifiers describe the earlier complete-family20-path setup. `tools/run_host_tests.py` covers packet/snapshot validation, age preservation, camera CRC/reassembly, real TCP backpressure bounds, display geometry, navigation, tile packing and battery logic. Adjust its compiler path on another machine.

`screen` exports the actual framebuffer (135×240 on Blue/Orange, 240×280 on Yellow/Rainbow, and logical 135×240 on the CYD, uniformly scaled to 180×320 on its 240×320 screen). Tab5 `screen full` exports 1280×720. Screen captures pause serial command processing during export (about three seconds on the CYD’s 460800-baud UART); radio ingress preserves its original source timestamp.

Power wiring, battery charger settings, battery capacity and cell health are not configured by the monitor UI. It never automatically charges/discharges in cycles. No runtime/load/discharge test was started for this project closeout.

Purple uses classic UART through CH340; `tools/install_purple.py` verifies its MAC on the same connected ROM handle before writing. Its factory app slot is 1 MiB; the current app fits. Backup and native panel/physical touch verification remain required on a new CYD variant.

The classic CYD allocates its 64.8 KB logical framebuffer in `setup()` after heap initialization; early global allocation failed on this board despite sufficient later free memory. Normal UART debug is command-driven and uses a TX buffer, while native-USB targets retain bounded write timeouts.

The owner’s CYD has both micro-USB and USB-C: this is the ST7789 CYD2USB display variant, with BGR order, inversion off, and horizontal MX cleared for this panel’s physical orientation, not the original single-port ILI9341 model. Its application UART uses 460800 baud; ROM upload starts at 115200.


## v17 additions and actual installation scope

Pink uses the generic ESP32-S3 profile with `FlashSize=8M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=default_8MB` and compiler flag `-DFACE_STICK=1`. XIAO uses `esp32:esp32:XIAO_ESP32S3:PSRAM=opi`. Both new original8MB full backups must pass hardware digest and local SHA checks. `tools/install_s3_member.py pink` writes only its original factory1MiB slot; `... camera` writes its original active OTA app1 at0x1d0000, not the generated default partition layout. Its `.ino` is deliberately empty; implementation in `main.cpp` avoids an observed Arduino automatic-prototype recursion bug.

`tools/verify_camera_family.py` inventories connected viewers, records unavailable Orange/Yellow instead of pretending to test them, checks Pink telemetry/unchanged charger control, captures each connected face/camera view and Tab5, and tests20seconds without USB reads plus each camera’s loss/fallback/recovery and both-cameras-off battery operation. It only issues UI/readback/radio commands. Historical `verify_family.py`/`verify_usb_idle.py` describe the older complete-family v15 setup; use the v17 verifier for this mixed availability run. The source build includes Orange and Yellow camera support, but those devices require USB reconnection before it can be installed.

The v17 radio sends one battery/presence broadcast per sample rather than unicast traffic growing with family size. Broadcast completion is not delivery proof. Camera frames also use broadcast, two paced copies and per-boot sessions. Blue serves completed camera snapshots from a separate task/socket over TCP4244 for the hosted Wi-Fi Tab5 receiver, leaving UI/battery tasks independent. Its original battery relay remains UDP4242. The earlier small-UDP-fragment relay was replaced after actual packet loss. Frames are reconstructed completely and CRC-checked before display, and expire after5seconds.

Tab5’s onboard camera uses native Arduino-ESP32 CSI/ISP and clock-output libraries, not a new IDF component or C6 modem update. Read [the driver notes](tab5-camera-driver.md). Firmware uses SCCB0x36,24MHzGPIO36, PID0xeb52 and camera-only reset bit6 at expander0x43; shared display/charger rails remain owned by existing display setup.
