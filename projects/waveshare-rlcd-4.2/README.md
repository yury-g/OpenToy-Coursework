# Waveshare ESP32-S3-RLCD-4.2

Dedicated OpenToy target for the rectangular reflective LCD. Follows the [family strategy](../../docs/DEVICE-FAMILY-ARCHITECTURE.md) and [status-bar standard](../../docs/APP-STATUS-BAR.md).

**Eyes 1 v0.2.0**, created 2026-11-17, adds native contours and the requested black background; the installed revision and live results are in the verification record. Owner confirms the v0.2.0 label and cleaner appearance. Live checks pass; physical KEY/menu confirmation is tracked separately. See [verification](VERIFICATION.md) for the actual installed/tested boundary and the [per-app compatibility matrix](COMPATIBILITY.md). This does not include a universal app launcher.

## Hardware and presentation

[Official board documentation](https://docs.waveshare.com/ESP32-S3-RLCD-4.2) and pinned [vendor source](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/tree/9e754fdf431ef4fce21e4e8b19bd4079ba043e86) describe an ESP32-S3-WROOM-1-N16R8, 16 MB flash, 8 MB PSRAM and 300 × 400 ST7305 reflective LCD. This port uses landscape **400 × 300**, one bit per pixel, black background and light eye shapes with dark large pupils, drawn as native analytic curves. No backlight exists. Physical V1/V2 QC revision remains unconfirmed; the vendor describes their change as mechanical spacer thickness, with the electronics unchanged.

The existing Curious, Dreamy and Cheeky behavior is reused directly: distinct idle pacing/blinks, independent anger, trembling and recovery. Curious is the boot default. The header displays app, installed version, fixed version creation date and local measured voltage (or `--`), with the personality on the second row. It makes no battery percentage, presence or charging assertion. ADC calibration is not an independent battery-capacity test.

The hardware adapter owns SPI pins SCK11/MOSI12/DC5/CS40/RST41; its initialization comes from the exact vendor example. The 15,000-byte panel buffer uses the vendor's interleaved landscape packing. The native renderer evaluates each contour at physical pixel centers instead of enlarging a small bitmap; the 1-bit panel retains single-pixel edges and ambient-light-dependent contrast. [Source comparisons and rendering/battery details](docs/NATIVE-RENDERING.md). Behavior advances between frames; rendering is scheduled at 20 Hz with unchanged transfers skipped. The panel remains in vendor high-power display mode; skipping transfers is not a measured battery-runtime improvement or a low-power panel mode. Display RAM is not assumed to persist with power removed.

## Controls

The v0.2.0 KEY profile follows the [family controls standard](../../docs/CONTROLS.md). Vendor factory button source identifies exposed KEY as active-low GPIO18, separate from BOOT GPIO0. BOOT, RESET and PWR have no normal app mappings.

| Action | Result |
| --- | --- |
| Short press KEY during play | Brief reaction in both eyes |
| Hold KEY for 0.8 seconds | Open Menu; restore the header if in performance mode |
| Short press KEY in Menu | Next item |
| Hold KEY in Menu | Select; a hold never also triggers a short press |
| Menu → Apps | Explicit Poke left / Poke right; current EYES1 only, more apps require installation |
| Menu → Personality | Curious, Dreamy or Cheeky |
| Menu → Display → Performance | Read the restore gesture, then explicitly enter performance |
| Back / Return to play | Exit a submenu / return to eyes |
| Serial `?` / `H` | Bounded diagnostics / session header toggle |
| Serial `1`, `2`, `3` / `L`, `R` | Personality selection / explicitly synthetic 300 ms eye pokes |
| Serial `k`, `K` | Explicitly synthetic short/hold KEY events for menu tests |

Normal boot restores the visible header. Performance hides the header/menu until a deliberate hold of KEY restores them. Physical button use is separately confirmed by the owner; synthetic events are not physical verification. Previous v0.1.1 BOOT-poke and two-button header chord are replaced. The app never samples BOOT. PWR retains hardware power behavior.

This target has no implemented touchscreen or automatic motion orientation. Audio and radio are off; the amplifier stays disabled. Sound explains this limitation rather than exposing working-looking controls. About reports honest battery-node volts; no percentage, charging or battery-presence inference.

## Build and recovery

Arduino-ESP32 **3.3.10**, Adafruit GFX **1.12.6**, BusIO **1.17.4**. The helper stages existing OpenToy behavior/renderer/telemetry headers into an external temporary sketch and builds only. No generic Arduino Upload.

```sh
python3 projects/waveshare-rlcd-4.2/tools/check.py
python3 projects/waveshare-rlcd-4.2/tools/build.py --build-path /private/path/rlcd-build --revision SOURCE-REVISION
```

Set `CXX` to a working C++14 compiler. On this Mac the CommandLineTools compiler requires an explicit `-isysroot` to its installed MacOSX SDK. Builds, keys, identities, logs and backups stay outside Git.

Before installation, verify native USB mapping and ROM chip/MAC, security and flash capacity. Preserve two full matching reads with a hardware digest, inspect the actual partition table and boot selection, and verify the image fits the selected original slot. Write only that bounded app range, verify its digest and every byte outside the sector-aligned write range before reboot. Do not overwrite bootloader, partition table, OTA selection, NVS or assets. Vendor downloadable factory firmware is not a substitute for the unit's own original backup. Current original firmware already contains an RLCD-specific board label; it is not claimed to be factory firmware.

[Implementation plan](PLAN.md) · [Licenses/provenance](firmware/RlcdEyes/THIRD-PARTY.md)
