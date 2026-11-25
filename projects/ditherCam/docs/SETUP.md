# Setup, Pairing & Firmware Build

[← ditherCam overview](../README.md) · [USB & stream protocol](PROTOCOL.md) · [Verification](VERIFICATION.md)

The OpenToy repository (private; access required) includes the firmware source with **dummy** pairing credentials in [`shared/CompileConfig.example.h`](../shared/CompileConfig.example.h) (`FLIPCAM_PAIR_CONFIG_IS_DUMMY = 1`). Never commit real Wi-Fi PSKs, pairing tokens, compiled binaries, or flash backups.

---

## 1. Local private pairing configuration

[`shared/FlipCamPairConfig.h`](../shared/FlipCamPairConfig.h) checks for an untracked local override header before falling back to `CompileConfig.example.h`:

1. Create `shared/private-localPairConfig.h` (git-ignored) **or** place `private-localPairConfig.h` / `privatePairConfig.h` in an external directory pointed to by `FLIPCAM_PRIVATE_INC`.
2. Define your private AP credentials and pair token (matching [`shared/CompileConfig.example.h`](../shared/CompileConfig.example.h)):
   ```cpp
   #pragma once
   #define FLIPCAM_PAIR_CONFIG_IS_DUMMY 0
   #define FLIPCAM_WIFI_SSID   "your-local-ap-name"
   #define FLIPCAM_WIFI_PSK    "your-local-passphrase"
   #define FLIPCAM_WIFI_CHANNEL 6
   #define FLIPCAM_RLCD_IP_A   192
   #define FLIPCAM_RLCD_IP_B   168
   #define FLIPCAM_RLCD_IP_C   4
   #define FLIPCAM_RLCD_IP_D   1
   #define FLIPCAM_XIAO_IP_A   192
   #define FLIPCAM_XIAO_IP_B   168
   #define FLIPCAM_XIAO_IP_C   4
   #define FLIPCAM_XIAO_IP_D   2
   #define FLIPCAM_PEER_PORT   8080
   #define FLIPCAM_WEB_PORT    80
   #define FLIPCAM_PAIR_TOKEN  "your-local-pair-token"
   ```
3. Flash **both** boards (`RlcdCamera` and `SenseCamera`) with the same pairing config so SoftAP (`192.168.4.1` ↔ `192.168.4.2`), HTTP `:8080`, and the packed `304 × 228` 1-bit `FMB1` stream on TCP `:8081` authenticate cleanly.

---

## 2. Building with `tools/build.py`

Requirements:
- `arduino-cli` on `PATH` (or `ARDUINO_CLI` / `FLIPCAM_ARDUINO_CLI` environment override)
- ESP32 Arduino core `3.3.10` installed in Arduino CLI's data directory (provides `gen_esp32part.py`; override with `FLIPCAM_GEN_ESP32PART` if needed)
- External Arduino library **`ArduinoJson`** (verified installed at **`7.4.3`**):
  ```sh
  arduino-cli lib install ArduinoJson@7.4.3
  ```
- Python 3

Run from the repository root:

```sh
# Optional: point to an external private header directory outside the repo
export FLIPCAM_PRIVATE_INC=/path/to/private-inc

python3 projects/ditherCam/tools/build.py
```

`build.py` builds both targets into git-ignored `projects/ditherCam/artifacts/`:
- **`rlcd`** (`firmware/RlcdCamera`): FQBN `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=dio,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc`, partition table [`partitions/rlcd_16MB_media.csv`](../partitions/rlcd_16MB_media.csv) (`0x330000` app slots, `8 MiB` `media` LittleFS).
- **`xiao`** (`firmware/SenseCamera`): FQBN `esp32:esp32:XIAO_ESP32S3:FlashSize=8M,FlashMode=dio,PSRAM=opi,USBMode=hwcdc`, partition table [`partitions/xiao_8MB_media.csv`](../partitions/xiao_8MB_media.csv) (`0x1C0000` app slots, `3.9375 MiB` `media` LittleFS, final 64 KiB `0x7F0000..0x800000` untouched).

App-only updates preserve existing `/media` files, partitions, and NVS/settings.

---

## 3. Web gallery & host tests

- **SoftAP web gallery:** Connect to the RLCD SoftAP and open `http://192.168.4.1/` to browse, download (`600 × 450` JPEG), delete stills, or adjust `look` / `contrast` / `mirror` / `orientation` / `sound`.
- **Host test suite:** Run C++ logic, dithering, orientation, protocol, storage, and partition checks locally without hardware:
  ```sh
  python3 projects/ditherCam/tests/run_all.py
  ```
- **Regenerating UI previews:**
  ```sh
  uv run --with pillow --with numpy python3 projects/ditherCam/tools/generate_ui_previews.py
  ```
