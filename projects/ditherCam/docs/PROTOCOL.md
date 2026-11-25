# FlipCam Field Firmware — USB Command Grammar, Stream & Network Protocol

## 1. USB Serial Command Grammar (115200 baud, HWCDC, newline-terminated)

Both `RlcdCamera` and `SenseCamera` accept line-oriented ASCII commands over USB CDC (`\n` or `\r\n` terminated) and respond while the peer is offline. This prototype is **photos-only** (`PHOTOS-ONLY.md`).

| Command | Description | Response |
| --- | --- | --- |
| `STATUS` | One-line JSON telemetry (versions, `pair_cfg`, flash/PSRAM/heap/stack HWM, media partition, FS bytes, `media.latest_id`/`latest_crc32`/`latest_bytes`/`latest_w`/`latest_h`, link, raw button levels, `phys_*` vs `inj_*` counters, `camera` incl. `preview_res` `320x240` / `still_res` `600x450` / `grab_us` / `mono_encoded` / `mono_encode_us`, `stream` incl. `port` / `rx_fps_x10` / `rx_seq` / `last_age_ms` / `reconnects` / `rx_lock_miss` / `http_info_last_ms` / `http_info_max_ms` / `cap_http_last_ms` or `frames_sent` / `rle_hits` / `raw_fallbacks` / `fps_x10`, `display` incl. `rlcd_frames` / `rlcd_new_frames` / `present_fps_x10` or `oled_frames` / `oled_fps_x10`, `audio`, `battery`) | Single-line JSON |
| `DIAG` | Extended `STATUS` with `last_error`, `panel_max_us` (RLCD), `cached_preview_bytes` (XIAO) | Single-line JSON |
| `CAPTURE` | Trigger real camera capture (RLCD requests peer `/peer/capture`; XIAO spools locally to `/media`) | `{"ok":true,"cmd":"CAPTURE"}` |
| `CAPTURE SYNTH` | Trigger deterministic synthetic test pattern JPEG (ONLY allowed when `TESTMODE ON`) | `{"ok":true,"cmd":"CAPTURE SYNTH"}` or `TESTMODE_REQUIRED` |
| `RECORD START` / `RECORD STOP` / `PLAY <id>` / `STOP` | Retired in photos-only prototype | `{"ok":false,"error":"UNSUPPORTED_PHOTOS_ONLY"}` |
| `LIST` | List stored still photos (`id`, `type`, `width`, `height`, `bytes`, `crc32`, `synthetic`) | Single-line JSON |
| `DELETE <id>` | Delete media item `<id>` (positive integer ID validated; rejects path traversal) | `{"ok":true/false,"cmd":"DELETE","id":...}` |
| `TONE [freq_hz] [ms]` | Diagnostic audio tone (`100..8000` Hz, `20..3000` ms): RLCD plays 16kHz PCM sine over ES8311+PA; XIAO chirps D3 buzzer | JSON with `i2s_tx_bytes` / `buzzer_gpio` |
| `SET <key> <val>` | Update `look` (`dither`/`threshold`), `contrast` (`low`/`normal`/`high`), `mirror` (`0`/`1`), `sound` (`0`/`1`), `orientation` (`selfie`/`rear`) — persists only when changed; RLCD pushes `/peer/config` | `{"ok":true/false,"cmd":"SET"}` |
| `GET SETTINGS` | Print active settings JSON (defaults: `look="dither"`, `contrast="high"`, `mirror=0`, `sound=1`, `orientation="selfie"`) | Single-line JSON |
| `SCREENSHOT` | Dump current native 1-bit framebuffer (`400x300` ST7305 `15000` bytes on RLCD; `128x64` SSD1306 `1024` bytes on XIAO) in bounded 120-char base64 lines with header & end CRC32 | `SCREENSHOT <w> <h> <bytes> <crc32_hex>` ... `SCREENSHOT_END <crc32_hex>` |
| `TESTMODE ON` / `TESTMODE OFF` | Explicitly enable/disable `INJECT` and `CAPTURE SYNTH` | `{"ok":true,"test_mode":...}` |
| `INJECT <SNAP\|BROWSE> <DOWN\|UP\|TAP\|HOLD\|CANCEL>` | Inject test input into `ButtonController` (increments `inj_*` counters, never `phys_*`; rejected if `TESTMODE OFF`) | `{"ok":true/false,"cmd":"INJECT"}` |
| `READ <id> <offset> <length>` | Export bounded chunk (`1..4096` bytes) of saved media `<id>` over USB with slice CRC32 & base64 | `READ_DATA <id> <off> <len> <total> <crc32> <b64>` |
| `FORMAT MEDIA <label>` | Explicitly format the LittleFS media partition ONLY when `<label>` equals exact `"media"` and not busy | `{"ok":true/false,"cmd":"FORMAT_MEDIA"}` |
| `REBOOT` | Flush settings, cleanly unmount `/media`, restart ESP32-S3 | `{"ok":true,"cmd":"REBOOT"}` |

## 2. Controls & Orientation (`STEERING-BROWSE.md`, `PHOTOS-ONLY.md`, `PREVIEW-STEERING.md`)

- **KEY (`GPIO18`, Snap on RLCD) / D1 (`GPIO2` on XIAO)**:
  - In `LIVE` (RLCD): Tap captures a still photo (switches sensor to `SVGA 800x600` JPEG, captures, decodes once, encodes `600x450` 90 deg CW landscape grayscale JPEG q72 — plus 180 deg rotation when `orientation=rear`, restores `QVGA 320x240` live preview; ES8311 + D3 buzzer shutter sound when enabled). Hold (~1000 ms) shows progress on the `SNAP` badge, toggles `orientation` (`selfie` <-> `rear`) once with toast feedback, and suppresses tap capture on release. Busy holds are ignored safely.
  - On XIAO `D1`: Long-hold is explicitly disabled (`allowSnapHold=false`); releasing `D1` triggers one tap capture.
  - In `GALLERY`: Tap returns to `LIVE` on the first tap (`SNAP:LIVE`). Hold `SNAP` in `GALLERY` is a deliberate no-op (stored photos are never rerotated).
- **BOOT (`GPIO0`, Browse on RLCD)**:
  - Reverse-chronological STILL photo browser only (newest to oldest, stops at `OLDEST` with no wrap, no menus, no hold action or autorepeat).
- **PWR**: Hardware power latch (static label removed from full-screen overlay).
- **Battery Icon**: Both RLCD (`400x300`) and OLED (`128x64`) render a graphical battery icon (`drawBatteryIcon`). RLCD scales fill from ADC4 (`GPIO4`, 3x divider) inside valid 18650 range (`2750..4230 mV`, spread `<= 120 mV`); otherwise shows `?`. XIAO always shows `?`.

## 3. Fast 1-Bit Stream (`FMB1`) & HTTP Control Plane (`FAST-MONO.md`)

- **RLCD SoftAP**: `192.168.4.1` (WPA2-PSK via `FlipCamPairConfig.h`, `WiFi.setSleep(false)`), Web Gallery on `:80`.
- **XIAO STA**: `192.168.4.2` (`WiFi.setSleep(false)`), HTTP control/capture API on `:8080` (`Connection: close` / `http.setReuse(false)`), persistent binary 1-bit stream server on `:8081`.
- **`FMB1` Stream Protocol (TCP `:8081`)**:
  - Client sends `"AUTH <FLIPCAM_PAIR_TOKEN>\n"` within 1s.
  - Server pushes 32-byte `FmbFrameHeader` (`magic=0x31424D46` `'FMB1'`, `width=304`, `height=228`, `codec` `0=RAW_1BIT` or `1=PACKBITS_RLE_1BIT`, `seq`, `ageMs`, `payloadLen <= 15000`, `payloadCrc32`, `headerCrc32`) followed by `payloadLen` bytes representing the exact 4:3 (`304x228`, `8,664` bytes unpacked, MSB-first) 90 deg CW Selfie-baseline 1-bit live frame.
  - Receiver validates header CRC32, dimensions, bounded `payloadLen`, payload CRC32, and bounded PackBits decompression (rejecting `0x80` and truncated/oversize streams) without per-frame JPEG decoding, and uses arriving stream frames as the link heartbeat (only polling `/peer/info` on connect or `>= 3.5s` stall).
  - RLCD renders `304x228` full-screen (`400x300`, exact 4:3 nearest resample, applying local 180 deg rotation when `orientation=rear`), with compact dark-backed / outlined UI labels overlaid after the image (no outer frame, reticle ticks, or full-width top/bottom bars). Mini OLED (`128x64`) keeps its `68x51` letterboxed preview and text orientation unchanged.
  - Display polarity & aspect-ratio consistency: RLCD uses a black background with light labels (`SNAP`/`BROWSE` invert light on press) while preserving non-inverted photograph tones. Live RLCD (`400x300` from `304x228`), live OLED (`68x51` letterboxed in `128x53`), RLCD `GALLERY` (`400x300` for 4:3 stills; aspect-preserving fit for non-4:3 stills), and saved JPEG (`600x450`) all maintain exact proportions without stretching.
- **Photos-only Web Endpoints (`:80`)**: `/api/status`, `/api/media_list`, `/api/capture`, `/api/delete`, `/api/settings` (syncs `/peer/config` incl. `orientation` to XIAO), and `/api/media?id=<id>` (returns `image/jpeg` for stills; `/api/clip_meta` and `/api/clip_frame` return HTTP `410`).
