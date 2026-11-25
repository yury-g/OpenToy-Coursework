# Camera transport (firmware v17)

`firmware/FaceCamera` is a separate sender for the XIAO ESP32S3 Sense. It captures a QVGA RGB565 frame every two seconds and by default averages 2×2 cells into a **160×120 four-level grayscale** snapshot. A shared quality toggle selects **80×60 RGB332 color** instead. It then broadcasts two identical complete copies on ESP-NOW channel 1. Each chunk waits for its send callback and then leaves a 4 ms gap; copies have a 12 ms gap. A snapshot occupies 4,800 bytes before chunk headers. Frames remain in RAM only. The sketch does not initialize the microphone, SD card, an IP server, or a Wi-Fi association.

The station MAC, verified by the parent installation workflow, is **02:00:00:00:00:0b**. `facecamera::CAMERA_MAC` is separate from the battery registry. The family registry assigns the board source index 7 and a teal presence tile. Every two seconds the sender also broadcasts the existing v3 `PeerPacket` with `valid=0`. A standard XIAO ESP32S3 has no firmware-readable battery voltage without an external measurement circuit, so this source must never display a made-up battery percentage.

## Build

The installed ESP32 Arduino core is 3.3.10 and supplies `esp_camera.h`. Build without connecting to or resetting a device:

```sh
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3:PSRAM=opi \
  --build-path /tmp/face-camera-v17-build firmware/FaceCamera
```

This profile selects 8 MB flash and explicitly enables OPI PSRAM. The sketch requires at least 8 MB PSRAM; failure is reported in status. Compiling the sketch does not authorize changing a device's existing partition table, bootloader or OTA selection. Installation uses the separately verified original layout.

The camera map follows [Seeed's official camera guide](https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/): D0–D7 are GPIO 15, 17, 18, 16, 14, 12, 11, 48; XCLK 10, PCLK 13, VSYNC 38, HREF 47, SCCB SDA 40 and SCL 39. PWDN and RESET are unused. Camera initialization selects RGB565 QVGA, one PSRAM framebuffer and a 20 MHz XCLK. [Espressif's driver guidance](https://github.com/espressif/esp32-camera) explains the PSRAM requirements and possible RGB capture pressure with Wi-Fi enabled. The low frame rate and single buffer reduce this pressure; physical image and delivery verification still matter.

`rgb565beTo332()` reads high byte first, matching [Espressif's RGB565 conversion implementation](https://github.com/espressif/esp32-camera/blob/master/conversions/to_bmp.c). The host fixture checks red, green, blue, white and the last sampled pixel. Physical testing should include a known color target.

## Wire format

All multibyte integers are little endian, encoded byte by byte. No wire struct or C++ object layout is shared. Each datagram is at most **248 bytes**, within the classic ESP32 ESP-NOW 250-byte limit.

| Offset | Bytes | Value |
| --- | ---: | --- |
| 0 | 4 | ASCII `FCAM` |
| 4 | 1 | Version 1 |
| 5 | 1 | Chunk index, 0–21 |
| 6 | 1 | Chunk count, exactly 22 |
| 7 | 1 | High nibble source: 0=XIAO, 1=Tab5; low nibble format: 1=RGB332, 2=packed 2-bit grayscale |
| 8 | 4 | Snapshot sequence |
| 12 | 4 | Sender uptime in milliseconds at capture start |
| 16 | 4 | CRC-32/ISO-HDLC of the entire 4,800-byte image |
| 20 | 1 | Width: 80 color or 160 grayscale |
| 21 | 1 | Height: 60 color or 120 grayscale |
| 22 | 2 | Payload bytes: 220 for indices 0–20; 180 for index 21 |
| 24 | 4 | Random sender boot session |
| 28 | 220 or 180 | Pixel data in row-major order; grayscale packs four pixels per byte, highest bits first |

RGB332 uses RRR GGG BB, most significant bits first. `rgb332To565(v)` expands it for existing TFT renderers. CRC uses reflected polynomial `0xedb88320`, initial and final XOR `0xffffffff`; the `123456789` check value is `0xcbf43926`.

## Receiver integration

Include `firmware/FeatherFace/CameraProtocol.h`; it has no Arduino dependencies or heap allocation. Filter the actual source MAC **before** passing packets to the assembler. Do not route battery `PeerPacket`s into it. One `FrameAssembler` occupies under 12 KB, including separate staging and complete image buffers.

```cpp
facecamera::FrameAssembler camera;
// In the receive path, after source MAC and packet-kind filtering:
bool completed = camera.receive(data, length, millis());
// In the UI/gateway, under the same external lock:
uint8_t rgb332[facecamera::FRAME_BYTES];
bool available = camera.copyPixels(rgb332, millis());
```

`receive()` returns true only when it publishes a new complete, CRC-verified frame. False can mean an accepted partial packet or a rejection. Accessors are `fresh(now)`, `sequence()`, `senderSession()`, `senderUptime()`, `completedAt()`, `completedFrames()` and `rejectedPackets()`. `copyPixels(out,now)` requires a 4,800-byte destination and leaves it untouched on failure. `buildChunk(...)` computes the CRC; `buildChunkWithCrc(...)` accepts one precomputed CRC for the sender. Both return the exact datagram length or zero for invalid arguments/capacity.

Use the same external critical section/mutex for receive, copy and metadata reads when the ESP-NOW callback and UI run in different tasks. The class itself is deliberately platform independent and does not provide synchronization. Keep drawing and network I/O outside that lock.

The assembler tracks a22-bit received-chunk bitmap for matching session, sequence, uptime and checksum. It accepts reordered chunks and merges gaps across the sender's repeated copies. Identical duplicates are harmless; conflicting duplicate data or a failed whole-image CRC cannot publish. An incomplete or corrupt new image never replaces the last complete one. Exact complete duplicates and older frames do not refresh image age. Both pending assembly and displayed freshness expire after5,000ms, with wrap-safe time arithmetic.

Sequence and sender uptime use modular ordering within a sender session. A random 32-bit boot session is carried in bytes24–27. Once the old complete frame expires, a different session may rejoin at any sequence, including when receivers missed the first frames after reboot. Old sequences within a session and exact repeated frames cannot refresh image age. Source MAC filtering, session IDs and CRC detect accidental source/corruption/stale-copy problems; they are not cryptographic authentication. `resetSource()` explicitly invalidates all previous state.

## Quiet serial diagnostics

At 115200 baud, setup prints one JSON status and never waits for a USB reader. Normal streaming is quiet. Each recognized newline-terminated command returns one JSON status:

| Command | Effect |
| --- | --- |
| `status` | Report firmware 17, MAC, sensor PID, camera initialization error, PSRAM, radio/stream state and frame/error counters |
| `frame` | Capture immediately; transmit if radio is enabled; otherwise inspect capture/CRC without radio |
| `stream off` / `stream on` | Pause/resume periodic captures |
| `radio off` / `radio on` | Stop/reinitialize ESP-NOW and the Wi-Fi radio |

`frames_captured` counts valid converted images. `frames_sent` counts snapshots with at least one full copy submitted successfully through the radio callback, **not confirmed receiver delivery**; ESP-NOW broadcast has no receiver ACK. Initialization, capture and send failures have separate counters. No SSID, password, microphone data or image bytes appear in status.

## Verification

`tests/test_camera_protocol.cpp` exercises independently built wire fixtures, a standard CRC vector, exact packet lengths, format/dimension rejection, missing/duplicate/out-of-order/mixed chunks, corrupted payload, last-image preservation, repeated-copy recovery, old replay rejection, bounded reboot recovery, sequence/millisecond wrap, input conversion validation, colors and the memory ceiling. It is discovered by `python3 tools/run_host_tests.py` using the expanded `*test*.cpp` pattern. Host tests and a sketch build do not establish camera hardware operation, color correctness in the real sensor, radio delivery or receiver display behavior; these are separately checked during installation.

Each sender boot generates a random 32-bit session in header bytes24–27. After image expiry, a different session can rejoin at any frame sequence, even if the first frames were missed. Within a session, old sequences and exact repeats cannot refresh the image age. Session IDs are not authentication.

Hardware startup uses `main.cpp` with an empty `.ino`: Arduino's automatic prototype rewriting inserted recursive setup calls into the original anonymous-namespace sketch. Radio initialization waits for the STA interface, send callbacks have a bounded500ms deadline, and an unintended radio timeout retries after5seconds. Explicit `radio off` stays off until `radio on`.

Before transmission, the camera checks the driver's original DMA timestamp and discards queued images older than500ms. This also applies after a long pause. Status exposes `capture_age_ms` and `stale_discarded`, so a resumed old image is not relabeled live.


## Tab5 relay uses TCP

The Tab5's hosted Wi-Fi did not sustain the small relayed UDP camera fragments reliably, even though the ESP-NOW displays received complete frames. Final firmware uses a separate private-AP TCP snapshot connection on Blue port4244. Tab5 requests `F`, receives a32-byte FCAS header (version1;sequence;sender uptime;boot session;CRC;source age;4800-byte length), then exactly4800 RGB332 bytes. The receiver validates header/length and the same image CRC, preserves source age plus the entire request duration, and does not re-age repeated snapshots. Network work runs outside rendering and battery ingress. Battery UDP remains4242; small screens still receive ESP-NOW directly. Camera pixels remain in RAM and no HTTP/internet service is exposed.

USB diagnostics use a unique `sync TOKEN` marker before status JSON; host readback ignores queued reports from previous commands. A regression test injects an old queued JSON response and confirms it cannot be returned as fresh evidence.


## v17 pages, quality and second source

XIAO is the startup camera view when live. A short info press (Blue D1 / Pink B) or a tap leaves the default view for that device’s battery face; subsequent information pages include trend, XIAO, Tab5 camera and face. A next-face press or horizontal swipe first leaves a camera for the local battery face, then browses other reporters. Touch displays also support up/down camera swipes. Tab5 taps/swipes cycle the grid and live cameras. Explicit browsing suspends the automatic startup choice until reboot or `view default`. Camera loss returns to battery faces; a default-following view automatically resumes XIAO after it returns. Battery collection and trends run independently on every page.

Hold the XIAO camera image, or hold D1/B, for at least600ms and release to request color ↔ grayscale. There is no upper hold limit. The setting is shared by all viewers because they receive one feed. It defaults to grayscale on a XIAO reboot. Both modes use exactly4800bytes,22chunks and the same two-copy pacing. Four shades trade tonal precision for four times as many spatial samples. It is a still preview, not HD video. The format shown on a screen comes only from a complete CRC-verified frame, never a pending request.

`FCFG` controls are exactly8bytes: ASCII magic, version1, desiredformat1/2, twozeroreservedbytes. XIAO accepts them only from enabled enrolled display MACs, excluding Green and itself. This is source filtering, not cryptographic authorization. Tab5 requests `Q` plus a desired-format byte through the private TCP server; Blue forwards the same request over ESP-NOW. A request is not a delivery acknowledgment; the next complete frame establishes the effective format. Serial `quality color` / `quality gray` exposes the same path for hardware verification.

`FCAS` retains its32-byte size. Its version word contains version1 inbyte4, source inbyte5, format-minus-one inbyte6, andzero inbyte7. The remaining fields and fixed4800-byte payload length are unchanged. Invalid sources/formats are rejected.

Two separate assemblers isolate sequence, boot session, staging, complete pixels and freshness. Small displays accept source0 only from XIAO’s stationMAC, and source1 only from Blue’s stationMAC. Tab5 captures its own native camera in a separate worker, preserves its16:9 view in an80×45 image letterboxed within80×60color, publishes locally, and uploads `P` +32-byteFCAS +4800pixels toBlue. Blue accepts uploads only from privateAPclient192.168.4.2, checks source1/CRC/age/length, and relays fresh new sequences over ESP-NOW. The private connection carries no internet service. Tab5 continues fetching source0 with `F`.

The native [Tab5 driver](tab5-camera-driver.md) uses the manufacturer’s SC202CS/SC2356 table and PID0xeb52 atSCCB0x36, native CSI/ISP,24MHzGPIO36clock and bounded fresh capture. It leaves the sensor and its own CSI in standby between requests, and retries failed initialization after five seconds. It does not change the shared MIPI LDO, display brightness or charger controls. Status exposes separate capture/upload/errors, per-source frame age/format, and read-only Tab5 charger-pin masks. Full image data is volatile RAM only.
