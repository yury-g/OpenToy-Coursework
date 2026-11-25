#!/usr/bin/env python3
"""Render labelled synthetic 3-panel PNG preview of Tab5 0.2.0 UI modes."""
import os
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path


CPP_HARNESS = r"""
#include "adapter/Tab5Ui.h"
#include <cstdio>
#include <vector>

using namespace opentoy::tab5;

static void writeRgb24(const uint16_t* fb) {
  std::vector<uint8_t> rgb(kUiWidth * kUiHeight * 3);
  for (int i = 0; i < kUiWidth * kUiHeight; ++i) {
    uint16_t p = fb[i];
    uint8_t r = uint8_t(((p >> 11) & 0x1F) * 255 / 31);
    uint8_t g = uint8_t(((p >> 5) & 0x3F) * 255 / 63);
    uint8_t b = uint8_t((p & 0x1F) * 255 / 31);
    rgb[i * 3 + 0] = r;
    rgb[i * 3 + 1] = g;
    rgb[i * 3 + 2] = b;
  }
  std::fwrite(rgb.data(), 1, rgb.size(), stdout);
}

int main() {
  std::vector<uint16_t> fb(kUiWidth * kUiHeight, 0);
  std::vector<uint16_t> prev(320 * 180, 0x18e3);
  for (int y = 40; y < 140; ++y) {
    for (int x = 90; x < 230; ++x) prev[y * 320 + x] = 0x31a6;
  }

  BatterySample bat;
  Tab5LiveTelemetry tel;
  tel.cam_ready = true;
  tel.cam_streaming = true;
  tel.ae_warmed = true;
  tel.sensor_id = kSc202csSensorId;
  tel.frame_seq = 128;
  tel.encode_us = 9400;
  tel.detector_ready = true;
  tel.face_tracking = true;
  tel.face_count = 1;
  tel.infer_ms = 27;
  tel.gaze_x = -0.35f;
  tel.gaze_y = -0.10f;
  tel.primary_face = {true, 0.34f, 0.22f, 0.32f, 0.52f, 0.91f, false, {}, {}};
  tel.panel_name = "TAB5-DSI";
  tel.ina226_ok = true;
  tel.vbus_mv = 5040;

  // Panel 1: Camera Eye mode
  Tab5UiState ui;
  ui.mode = Tab5AppMode::CameraEye;
  renderTab5Frame(fb.data(), ui, bat, tel, prev.data(), 1200);
  writeRgb24(fb.data());

  // Panel 2: Copy-Face mode
  ui.mode = Tab5AppMode::CopyFace;
  renderTab5Frame(fb.data(), ui, bat, tel, prev.data(), 1200);
  writeRgb24(fb.data());

  // Panel 3: Apps Menu mode
  ui.openMenu();
  ui.page = Tab5MenuPage::Apps;
  renderTab5Frame(fb.data(), ui, bat, tel, prev.data(), 1200);
  writeRgb24(fb.data());
  return 0;
}
"""


def encode_png_rgb(width: int, height: int, rgb: bytes) -> bytes:
    def chunk(tag: bytes, payload: bytes) -> bytes:
        return (
            struct.pack(">I", len(payload))
            + tag
            + payload
            + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
        )

    rows = bytearray()
    stride = width * 3
    for y in range(height):
        rows.append(0)
        rows.extend(rgb[y * stride : (y + 1) * stride])
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b"")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    out_png = root / "docs/images/tab5-modes-preview.png"
    out_png.parent.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["DEVELOPER_DIR"] = "/Library/Developer/CommandLineTools"

    with tempfile.TemporaryDirectory(prefix="tab5-preview-") as tmp:
        src = Path(tmp) / "gen.cpp"
        exe = Path(tmp) / "gen"
        src.write_text(CPP_HARNESS)
        subprocess.run(
            ["clang++", "-std=c++14", f"-I{root}", str(src), "-o", str(exe)],
            env=env,
            check=True,
        )
        raw = subprocess.check_output([str(exe)], env=env)

    panel_w, panel_h = 640, 360
    assert len(raw) == 3 * panel_w * panel_h * 3
    # Stack vertically with 8-px dark separator bars -> 640 x (360*3 + 16)
    sep = bytes([20, 28, 36]) * (panel_w * 8)
    p0 = raw[0 : panel_w * panel_h * 3]
    p1 = raw[panel_w * panel_h * 3 : 2 * panel_w * panel_h * 3]
    p2 = raw[2 * panel_w * panel_h * 3 :]
    combined = p0 + sep + p1 + sep + p2
    out_png.write_bytes(encode_png_rgb(panel_w, panel_h * 3 + 16, combined))
    print(f"Wrote {out_png} ({out_png.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
