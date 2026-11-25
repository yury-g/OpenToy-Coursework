// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace opentoy_tiny_ui {

// Compact 3x5 uppercase/digit/symbol bitmap font (4 px advance at scale=1, 8 px at scale=2).
// Designed so all four required APP-STATUS-BAR.md fields ("EYES1 V1.4.0", "2026-11-17", battery)
// fit legibly without truncation even inside Nesso's 91-px 45-degree diagonal viewport or
// Feather's single 240x15 header strip.
inline const uint8_t* glyph3x5(char c) {
  static const char kChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.-:%/ ";
  // Each glyph has 5 rows of 3 bits (MSB = bit 2 = left column).
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
      {0, 0, 0, 0, 0},  // space
  };
  static const uint8_t kBlank[5] = {0, 0, 0, 0, 0};
  if (c >= 'a' && c <= 'z') c = char(c - 32);
  const char* p = std::strchr(kChars, c);
  return (c && p) ? kRows[p - kChars] : kBlank;
}

inline void drawText3x5(uint16_t* buf,
                        int buf_w,
                        int buf_h,
                        int x,
                        int y,
                        const char* text,
                        uint16_t color,
                        int scale = 1,
                        int clip_w = -1) {
  if (!buf || !text || scale < 1) return;
  const int max_x = (clip_w >= 0) ? std::min(buf_w, clip_w) : buf_w;
  int cur_x = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    const uint8_t* g = glyph3x5(text[i]);
    for (int r = 0; r < 5; ++r) {
      const uint8_t bits = g[r];
      for (int c = 0; c < 3; ++c) {
        if (bits & (4u >> c)) {
          for (int sy = 0; sy < scale; ++sy) {
            const int py = y + r * scale + sy;
            if (py < 0 || py >= buf_h) continue;
            for (int sx = 0; sx < scale; ++sx) {
              const int px = cur_x + c * scale + sx;
              if (px >= 0 && px < max_x) {
                buf[py * buf_w + px] = color;
              }
            }
          }
        }
      }
    }
    cur_x += 4 * scale;
  }
}

inline int textWidth3x5(const char* text, int scale = 1) {
  if (!text || !text[0]) return 0;
  return int(std::strlen(text)) * 4 * scale - scale;
}

inline void fillRectBuf(uint16_t* buf,
                        int buf_w,
                        int buf_h,
                        int x,
                        int y,
                        int w,
                        int h,
                        uint16_t color) {
  if (!buf || w <= 0 || h <= 0) return;
  const int x0 = std::max(0, x);
  const int y0 = std::max(0, y);
  const int x1 = std::min(buf_w, x + w);
  const int y1 = std::min(buf_h, y + h);
  for (int py = y0; py < y1; ++py) {
    for (int px = x0; px < x1; ++px) {
      buf[py * buf_w + px] = color;
    }
  }
}

inline void drawRectBuf(uint16_t* buf,
                        int buf_w,
                        int buf_h,
                        int x,
                        int y,
                        int w,
                        int h,
                        uint16_t color) {
  fillRectBuf(buf, buf_w, buf_h, x, y, w, 1, color);
  fillRectBuf(buf, buf_w, buf_h, x, y + h - 1, w, 1, color);
  fillRectBuf(buf, buf_w, buf_h, x, y, 1, h, color);
  fillRectBuf(buf, buf_w, buf_h, x + w - 1, y, 1, h, color);
}

// Truthful battery formatter for fuel-gauge boards (MAX17048 / M5.Power):
// - Never renders an empty-battery outline when battery < 0 (unavailable).
// - Renders "BAT --" when unavailable, or "XX%" + proportional gauge when [0, 100].
inline void formatGaugeBatteryText(int percent, char (&out)[10]) {
  if (percent < 0 || percent > 100) {
    std::snprintf(out, sizeof(out), "BAT --");
  } else {
    std::snprintf(out, sizeof(out), "%d%%", percent);
  }
}

// Canonical 7-entry OpenToy Menu order (CONTROLS.md):
// Return to play, Apps, Personality, Sound, Display, Controls, About.
enum class MenuPage : uint8_t {
  Play = 0,
  Root = 1,
  Apps = 2,
  Personality = 3,
  Sound = 4,
  Display = 5,
  Controls = 6,
  About = 7,
};

inline const char* rootEntryLabel(unsigned idx) {
  static const char* const kLabels[7] = {
      "RETURN TO PLAY",
      "APPS",
      "PERSONALITY",
      "SOUND",
      "DISPLAY",
      "CONTROLS",
      "ABOUT",
  };
  return idx < 7 ? kLabels[idx] : "";
}

}  // namespace opentoy_tiny_ui
