// SPDX-License-Identifier: MIT
#pragma once
#include "NessoPixels.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace nesso_water {

// Overlaid 2-row status bounds on 135x240 displays:
// Row 1 (y=3..7, halo 2..8): "<APP> V<VER>" on left, tiny face + battery on right.
// Row 2 (y=11..15, halo 10..16): "<YYYY-MM-DD>" on left, "IMU?" on right when stale.
constexpr int kOverlayRows = 17;

inline const uint8_t* overlayGlyph3x5(char c) {
  static const char kChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.-:%/? ";
  static const uint8_t kRows[][5] = {
      {7, 5, 5, 5, 7},  // 0
      {2, 6, 2, 2, 7},  // 1
      {7, 1, 7, 4, 7},  // 2
      {7, 1, 7, 1, 7},  // 3
      {5, 5, 7, 1, 1},  // 4
      {7, 4, 7, 1, 7},  // 5
      {7, 4, 7, 5, 7},  // 6
      {7, 1, 1, 2, 2},  // 7
      {7, 5, 7, 5, 7},  // 8
      {7, 5, 7, 1, 7},  // 9
      {2, 5, 7, 5, 5},  // A
      {6, 5, 6, 5, 6},  // B
      {3, 4, 4, 4, 3},  // C
      {6, 5, 5, 5, 6},  // D
      {7, 4, 6, 4, 7},  // E
      {7, 4, 6, 4, 4},  // F
      {3, 4, 5, 5, 3},  // G
      {5, 5, 7, 5, 5},  // H
      {7, 2, 2, 2, 7},  // I
      {1, 1, 1, 5, 2},  // J
      {5, 5, 6, 5, 5},  // K
      {4, 4, 4, 4, 7},  // L
      {5, 7, 7, 5, 5},  // M
      {5, 7, 7, 7, 5},  // N
      {2, 5, 5, 5, 2},  // O
      {6, 5, 6, 4, 4},  // P
      {2, 5, 5, 7, 3},  // Q
      {6, 5, 6, 5, 5},  // R
      {3, 4, 2, 1, 6},  // S
      {7, 2, 2, 2, 2},  // T
      {5, 5, 5, 5, 7},  // U
      {5, 5, 5, 2, 2},  // V
      {5, 5, 7, 7, 5},  // W
      {5, 5, 2, 5, 5},  // X
      {5, 5, 2, 2, 2},  // Y
      {7, 1, 2, 4, 7},  // Z
      {0, 0, 0, 0, 2},  // .
      {0, 0, 7, 0, 0},  // -
      {0, 2, 0, 2, 0},  // :
      {5, 1, 2, 4, 5},  // %
      {1, 1, 2, 4, 4},  // /
      {6, 1, 2, 0, 2},  // ?
      {0, 0, 0, 0, 0},  // space
  };
  static const uint8_t kBlank[5] = {0, 0, 0, 0, 0};
  if (c >= 'a' && c <= 'z') c = char(c - 32);
  const char* p = std::strchr(kChars, c);
  return (c && p) ? kRows[p - kChars] : kBlank;
}

inline int overlayTextWidth(const char* text) {
  return (text && text[0]) ? int(std::strlen(text)) * 4 - 1 : 0;
}

inline void overlayPutHaloPixel(
    uint16_t* buf, int stride, int clipX, int h, int x, int y, uint16_t c) {
  for (int dy = -1; dy <= 1; ++dy) {
    const int yy = y + dy;
    if (yy < 0 || yy >= h) continue;
    for (int dx = -1; dx <= 1; ++dx) {
      const int xx = x + dx;
      if (xx >= 0 && xx < clipX) buf[yy * stride + xx] = c;
    }
  }
}

inline void overlayPutPixel(
    uint16_t* buf, int stride, int clipX, int h, int x, int y, uint16_t c) {
  if (x >= 0 && x < clipX && y >= 0 && y < h) buf[y * stride + x] = c;
}

inline void drawOverlayText(uint16_t* buf,
                            int w,
                            int h,
                            int x,
                            int y,
                            const char* text,
                            uint16_t color,
                            uint16_t halo,
                            int clipW = -1) {
  if (!buf || !text) return;
  const int maxX = (clipW >= 0) ? std::min(w, clipW) : w;
  int cx = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    const uint8_t* g = overlayGlyph3x5(text[i]);
    for (int r = 0; r < 5; ++r) {
      for (int c = 0; c < 3; ++c) {
        if (g[r] & (4u >> c)) overlayPutHaloPixel(buf, w, maxX, h, cx + c, y + r, halo);
      }
    }
    cx += 4;
  }
  cx = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    const uint8_t* g = overlayGlyph3x5(text[i]);
    for (int r = 0; r < 5; ++r) {
      for (int c = 0; c < 3; ++c) {
        if (g[r] & (4u >> c)) overlayPutPixel(buf, w, maxX, h, cx + c, y + r, color);
      }
    }
    cx += 4;
  }
}

inline void drawOverlayTinyFace(uint16_t* buf,
                                int w,
                                int h,
                                int x,
                                int y,
                                int battery,
                                uint16_t ink,
                                uint16_t halo) {
  // 7x5 tiny battery face (eyes + smile/neutral/frown) with 1px halo.
  static const int kEyes[2][2] = {{1, 1}, {5, 1}};
  for (int i = 0; i < 2; ++i)
    overlayPutHaloPixel(buf, w, w, h, x + kEyes[i][0], y + kEyes[i][1], halo);
  for (int mx = 1; mx <= 5; ++mx) overlayPutHaloPixel(buf, w, w, h, x + mx, y + 4, halo);
  if (battery >= 70) {
    overlayPutHaloPixel(buf, w, w, h, x, y + 3, halo);
    overlayPutHaloPixel(buf, w, w, h, x + 6, y + 3, halo);
  } else if (battery >= 0 && battery <= 15) {
    overlayPutHaloPixel(buf, w, w, h, x, y + 4, halo);
    overlayPutHaloPixel(buf, w, w, h, x + 6, y + 4, halo);
    for (int mx = 1; mx <= 5; ++mx) overlayPutHaloPixel(buf, w, w, h, x + mx, y + 3, halo);
  }

  for (int i = 0; i < 2; ++i)
    overlayPutPixel(buf, w, w, h, x + kEyes[i][0], y + kEyes[i][1], ink);
  if (battery >= 70) {
    overlayPutPixel(buf, w, w, h, x, y + 3, ink);
    for (int mx = 1; mx <= 5; ++mx) overlayPutPixel(buf, w, w, h, x + mx, y + 4, ink);
    overlayPutPixel(buf, w, w, h, x + 6, y + 3, ink);
  } else if (battery >= 0 && battery <= 15) {
    overlayPutPixel(buf, w, w, h, x, y + 4, ink);
    for (int mx = 1; mx <= 5; ++mx) overlayPutPixel(buf, w, w, h, x + mx, y + 3, ink);
    overlayPutPixel(buf, w, w, h, x + 6, y + 4, ink);
  } else {
    for (int mx = 1; mx <= 5; ++mx) overlayPutPixel(buf, w, w, h, x + mx, y + 4, ink);
  }
}

// Draws 2-row overlaid metadata AFTER the particle canvas using display byte-swapped RGB565.
// Writes only inside rows [0, kOverlayRows - 1] with no opaque full-width background bar.
inline void drawWaterOverlay(uint16_t* buf,
                             int w,
                             int h,
                             const char* appShort,
                             const char* ver,
                             const char* created,
                             int battery,
                             bool imuFresh,
                             bool showOverlay = true) {
  if (!showOverlay || !buf || h < kOverlayRows) return;
  const uint16_t halo = pixelColor(2, 6, 12);
  const uint16_t cyan = pixelColor(102, 232, 255);
  const uint16_t silver = pixelColor(214, 232, 236);
  const uint16_t green = pixelColor(115, 222, 188);
  const uint16_t red = pixelColor(239, 133, 100);
  const uint16_t warn = pixelColor(255, 198, 92);

  char row1Left[24] = {};
  std::snprintf(row1Left, sizeof(row1Left), "%s V%s", appShort, ver);
  drawOverlayText(buf, w, h, 3, 3, row1Left, cyan, halo, w - 40);

  if (battery < 0 || battery > 100) {
    const char* batTxt = "BAT --";
    const int bw = overlayTextWidth(batTxt);
    const int bx = w - 3 - bw;
    drawOverlayTinyFace(buf, w, h, bx - 10, 3, -1, silver, halo);
    drawOverlayText(buf, w, h, bx, 3, batTxt, silver, halo, w);
  } else {
    const int iconW = 11;
    const int iconX = w - 3 - iconW - 1;
    // Halo behind mini battery gauge (rows 2..8).
    for (int py = 2; py <= 8; ++py) {
      for (int px = iconX - 1; px <= iconX + iconW + 1; ++px) {
        overlayPutPixel(buf, w, w, h, px, py, halo);
      }
    }
    for (int px = iconX; px < iconX + iconW; ++px) {
      overlayPutPixel(buf, w, w, h, px, 3, silver);
      overlayPutPixel(buf, w, w, h, px, 7, silver);
    }
    for (int py = 3; py <= 7; ++py) {
      overlayPutPixel(buf, w, w, h, iconX, py, silver);
      overlayPutPixel(buf, w, w, h, iconX + iconW - 1, py, silver);
    }
    for (int py = 4; py <= 6; ++py) overlayPutPixel(buf, w, w, h, iconX + iconW, py, silver);
    const int fillW = ((iconW - 2) * battery) / 100;
    const uint16_t fillCol = (battery <= 15) ? red : green;
    for (int py = 4; py <= 6; ++py) {
      for (int px = iconX + 1; px < iconX + 1 + fillW; ++px) {
        overlayPutPixel(buf, w, w, h, px, py, fillCol);
      }
    }
    char pctTxt[8] = {};
    std::snprintf(pctTxt, sizeof(pctTxt), "%d%%", battery);
    const int tw = overlayTextWidth(pctTxt);
    const int tx = iconX - 2 - tw;
    drawOverlayTinyFace(buf, w, h, tx - 10, 3, battery, silver, halo);
    drawOverlayText(buf, w, h, tx, 3, pctTxt, silver, halo, iconX);
  }

  drawOverlayText(buf, w, h, 3, 11, created, silver, halo, w - 24);
  if (!imuFresh) {
    const char* imuTxt = "IMU?";
    drawOverlayText(buf, w, h, w - 3 - overlayTextWidth(imuTxt), 11, imuTxt, warn, halo, w);
  }
}

struct DirtySpan {
  int first = 0;
  int last = -1;
  bool valid() const { return first <= last; }
};

// Computes up to 2 dirty row spans so moving particles/cursors under overlaid metadata
// are always redrawn and overlay label changes never leave stale glyphs.
struct WaterDirtyTracker {
  Rows prevRows{0, 239};
  int prevBattery = -999;
  bool prevFresh = false;
  bool prevOverlay = true;
  bool initialized = false;

  void compute(const Rows& currRows,
               int height,
               int battery,
               bool imuFresh,
               bool showOverlay,
               DirtySpan& spanA,
               DirtySpan& spanB) {
    spanA = {0, -1};
    spanB = {0, -1};
    const int clampedFirst = std::max(0, std::min(height - 1, currRows.first));
    const int clampedLast = std::max(0, std::min(height - 1, currRows.last));
    if (!initialized) {
      initialized = true;
      prevRows = {clampedFirst, clampedLast};
      prevBattery = battery;
      prevFresh = imuFresh;
      prevOverlay = showOverlay;
      spanA = {0, height - 1};
      return;
    }

    int dirtyFirst = std::min(clampedFirst, prevRows.first);
    int dirtyLast = std::max(clampedLast, prevRows.last);
    const bool overlayDirty =
        (prevBattery != battery) || (prevFresh != imuFresh) || (prevOverlay != showOverlay);

    if (overlayDirty) {
      const int overlayLast = std::min(height - 1, kOverlayRows - 1);
      if (dirtyFirst <= overlayLast + 1) {
        spanA = {0, std::max(dirtyLast, overlayLast)};
      } else {
        spanA = {0, overlayLast};
        spanB = {dirtyFirst, dirtyLast};
      }
    } else {
      spanA = {dirtyFirst, dirtyLast};
    }

    prevRows = {clampedFirst, clampedLast};
    prevBattery = battery;
    prevFresh = imuFresh;
    prevOverlay = showOverlay;
  }
};

}  // namespace nesso_water
