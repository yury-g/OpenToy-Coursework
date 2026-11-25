# Verification & Bench Status (November 25, 2026)

[← ditherCam overview](../README.md) · [Setup & build](SETUP.md) · [USB & stream protocol](PROTOCOL.md)

This page records what was verified on host and on the prototype boards as of **November 25, 2026**, separating synthetic/USB checks from physical observations.

---

## Host verification (`tests/run_all.py`)

Compiled against [`shared/`](../shared/) with `clang++ -std=c++17 -Wall -Wextra -Werror`:
- Partition layout invariants (`rlcd_16MB_media.csv` 16 MiB and `xiao_8MB_media.csv` 8 MiB with final 64 KiB preserved) and dummy-config repository hygiene.
- Full-screen `400 × 300` edge-to-edge coverage and UI-over-image draw order.
- Exact `90° CW` Selfie baseline and `180°` Rear rotation symmetry (not horizontal mirror), plus unchanged `68 × 51` mini OLED preview pixels across mode switches.
- `ButtonController` tap vs ~1 s hold separation, boot-held suppression, and Gallery no-op hold (old photos never re-rotated).
- `FlipCamSettings` backwards compatibility (`16`-byte layout), LittleFS media catalog bounds (256-item cap, traversal rejection), and `FMB1` PackBits RLE / `RAW_1BIT` fallback + CRC32 checks.

---

## Hardware bench verification (private paired build, Nov 25, 2026)

- **App-slot flash integrity:** Both private builds compiled, fit their existing app slots, and flashed with matching flash hashes. Every byte outside each application region was verified unchanged; all 6 pre-update RLCD catalog entries and 1 pre-update XIAO entry retained metadata and CRCs.
- **USB-injected control & orientation checks:**
  - `TESTMODE` USB-injected holds switched `SELFIE` ↔ `REAR` once without triggering a capture on release; tap `Snap` and `Browse` gallery playback passed.
  - Stationary-scene `Selfie` and `Rear` captures exported with matching CRCs (`600 × 450` grayscale JPEGs, ~16.7 KB in the tested scene). Rotating one capture 180° reduced mean pixel difference from **31.82** to **1.80**, confirming the intended rotation.
  - `REAR` setting survived an RLCD reboot; both boards were returned to `SELFIE` with `TESTMODE OFF`.
- **Measured frame rates (current test scene):**
  - RLCD: **10.19–10.58 fresh fps** across four 5-second windows (matching the ~10.2–10.6 fps pre-update baseline; full-screen rendering introduced no material slowdown).
  - Mini OLED: **10.38–10.60 fps** across the same windows.
  - Note: Earlier **12.2–12.4 fps** readings came from a different scene/lighting setup; a **12 fps minimum is not established** for current conditions. Still capture briefly pauses live preview.
- **Battery & storage limits:**
  - RLCD shows ADC4 voltage-estimated battery level (`?` outside valid range); XIAO OLED always shows `?` (no battery measurement path). Do not claim calibrated percentage or charging status.
  - Storage: `8 MiB` RLCD media / `3.9375 MiB` XIAO media, **256-item catalog limit per board** (also bounded by file sizes and filesystem overhead).

---

## Pending physical confirmations

The following remain open for physical owner testing (not claimed complete):
- Physical `KEY` (`Snap`) 1-second hold button feel and outward-facing `REAR` shot orientation check on the assembled unit.
- Audible shutter speaker/buzzer confirmation (ES8311 I2S codec and `D3` buzzer TX paths verified in firmware; physical listening test pending).
- Physical long-term enclosure clip/hinge wear (see [enclosure limitations](../../3d-printed-designs/ditherCam/README.md#owner-fit--honest-limitations)).
