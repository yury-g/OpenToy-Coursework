# Camera adapter dependency evidence — 2026-11-17

`camera-idf5.4.2.lock` freezes the complete resolved dependency set, including
unchanged display component pins. Direct camera runtime: esp_video 2.0.0,
human_face_detect 0.5.2; resolved sensor 2.0.1, esp_ipa 1.3.1,
esp-dl 3.3.15. Build target stays IDF 5.4.2 / P4 revision range 0.1–1.99,
360 MHz CPU and 200 MHz PSRAM. No model partition is added.

Primary versioned sources:

- [esp_video 2.0.0](https://github.com/espressif/esp-video-components/tree/a534ddc65e09c6b02fc66e5e7c81cb1e4e16eb5e/esp_video)
- [esp_cam_sensor 2.0.1](https://github.com/espressif/esp-video-components/tree/52b416934c5ed92f2252e387609f586b1e20b256/esp_cam_sensor)
- [human_face_detect 0.5.2](https://github.com/espressif/esp-dl/tree/eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6/models/human_face_detect)
- [esp-dl 3.3.15](https://github.com/espressif/esp-dl/tree/512ed0b0dc5f5320f3d9ca5a5aea189803f87f31/esp-dl)

`firmware/main/sc2336-p4-rev1.json` is an unmodified copy of the sensor component's
`sensors/sc2336/cfg/sc2336_default_p4_eco4.json`, SHA-256
`3b10e9474baf180f901624891a9f2e52aee8dba3db0c01d2091423ee7649146a`.
This component is Apache-2.0 licensed; the unmodified license is included as
`esp-cam-sensor-LICENSE.txt`. Explicit selection is required because its
project_include.cmake checks `CONFIG_ESP32P4_SELECTS_REV_LESS_V3`, unavailable in
IDF 5.4.2, and would otherwise select the newer ECO5 tuning. This is upstream
revision-specific tuning, not a claim of vendor optical calibration.

The pinned video API has a 250 ms DQBUF timeout configured and read back. Its
VFS ignores O_NONBLOCK, so the adapter does not use that flag as a timeout.
This exact release returns ESP_FAIL on an empty timed-out dequeue (mapped to
negative errno in adapter status), rather than necessarily ETIMEDOUT.
Three consecutive capture failures stop the capture worker; no automatic retry,
reset, SCCB scan or XCLK generation is performed. Only SC2336 is compiled for
autodetection, following the initial bounded 0x30/cb3a identity check.

RGB565 is little endian in video 2.0.0 and is passed as RGB565LE to ESP-DL and
native uint16 pixels to LVGL. The short-swap preprocessing remains enabled:
the driver applies it only when the sensor reports its required data sequence.
It swaps adjacent 16-bit words, not the RGB565 byte-order contract.

Two driver buffers are continuously drained on a capture worker. RAM preview
is center-cropped from 1024x600 to 1024x576 and downsampled to 320x180. A separate
inference worker copies only the newest preview and publishes the same image,
normalized boxes and five keypoints atomically. No lip/eyelid contours or mouth
activity are inferred; the five-point order is left eye, left mouth corner,
nose, right eye, right mouth corner. All camera image data stays in volatile RAM.

This video release does not populate a hardware capture timestamp. The adapter
uses dequeue-start uptime minus one 30fps frame interval (34ms), an estimated
age allowance, not an exposure timestamp or a guaranteed end-to-end bound.
Capture/inference never hold the UI mutex; UI snapshot locking is nonblocking.

Model inference has a finite graph and at most ten candidate refinements. It
is not hard realtime. ESP-DL's constructor uses asserting internal allocations;
a 4MiB PSRAM preflight is a precaution, not a recoverable-error guarantee.
Sensor initialization consists of bounded SCCB transfers and finite register
configuration; teardown and library-internal scheduling are not claimed as
hard realtime. Build verification does not establish physical capture quality,
orientation, face detection, latency or cold-boot behavior.

## Startup diagnostics

Version 0.2.1 reports the last startup boundary: 1 create SCCB bus; 2 attach
0x30 device; 3 read ID high register0x3107; 4 read ID low register0x3108;
5 verify identity; 6 initialize video; 7 open/query device; 8 pixel format;
9 dequeue timeout; 10 buffers; 11 stream start; 12 running. An error during
a transaction does not prove the camera is absent. Error259 is IDF
ESP_ERR_INVALID_STATE and can represent an unfinished I2C transaction.
Do not derive a sensor model or change physical wiring from this code alone.
