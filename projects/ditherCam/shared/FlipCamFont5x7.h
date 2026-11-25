// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>

namespace flipcam {

// Exact 5x7 font table ported from approved-ui/font5x7.js
struct Glyph5x7 {
  char ch;
  uint8_t cols[5];
};

inline const Glyph5x7* lookupGlyph5x7(char rawCh) {
  static const Glyph5x7 kTable[] = {
      {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
      {'!', {0x00, 0x00, 0x5f, 0x00, 0x00}},
      {'"', {0x00, 0x07, 0x00, 0x07, 0x00}},
      {'#', {0x14, 0x7f, 0x14, 0x7f, 0x14}},
      {'%', {0x23, 0x13, 0x08, 0x64, 0x62}},
      {'&', {0x36, 0x49, 0x55, 0x22, 0x50}},
      {'\'', {0x00, 0x05, 0x03, 0x00, 0x00}},
      {'(', {0x00, 0x1c, 0x22, 0x41, 0x00}},
      {')', {0x00, 0x41, 0x22, 0x1c, 0x00}},
      {'*', {0x14, 0x08, 0x3e, 0x08, 0x14}},
      {'+', {0x08, 0x08, 0x3e, 0x08, 0x08}},
      {',', {0x00, 0x50, 0x30, 0x00, 0x00}},
      {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
      {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
      {'/', {0x20, 0x10, 0x08, 0x04, 0x02}},
      {'0', {0x3e, 0x51, 0x49, 0x45, 0x3e}},
      {'1', {0x00, 0x42, 0x7f, 0x40, 0x00}},
      {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
      {'3', {0x21, 0x41, 0x45, 0x4b, 0x31}},
      {'4', {0x18, 0x14, 0x12, 0x7f, 0x10}},
      {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
      {'6', {0x3c, 0x4a, 0x49, 0x49, 0x30}},
      {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
      {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
      {'9', {0x06, 0x49, 0x49, 0x29, 0x1e}},
      {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
      {';', {0x00, 0x56, 0x36, 0x00, 0x00}},
      {'<', {0x08, 0x14, 0x22, 0x41, 0x00}},
      {'=', {0x14, 0x14, 0x14, 0x14, 0x14}},
      {'>', {0x00, 0x41, 0x22, 0x14, 0x08}},
      {'?', {0x02, 0x01, 0x51, 0x09, 0x06}},
      {'@', {0x32, 0x49, 0x79, 0x41, 0x3e}},
      {'A', {0x7e, 0x11, 0x11, 0x11, 0x7e}},
      {'B', {0x7f, 0x49, 0x49, 0x49, 0x36}},
      {'C', {0x3e, 0x41, 0x41, 0x41, 0x22}},
      {'D', {0x7f, 0x41, 0x41, 0x22, 0x1c}},
      {'E', {0x7f, 0x49, 0x49, 0x49, 0x41}},
      {'F', {0x7f, 0x09, 0x09, 0x09, 0x01}},
      {'G', {0x3e, 0x41, 0x49, 0x49, 0x7a}},
      {'H', {0x7f, 0x08, 0x08, 0x08, 0x7f}},
      {'I', {0x00, 0x41, 0x7f, 0x41, 0x00}},
      {'J', {0x20, 0x40, 0x41, 0x3f, 0x01}},
      {'K', {0x7f, 0x08, 0x14, 0x22, 0x41}},
      {'L', {0x7f, 0x40, 0x40, 0x40, 0x40}},
      {'M', {0x7f, 0x02, 0x0c, 0x02, 0x7f}},
      {'N', {0x7f, 0x04, 0x08, 0x10, 0x7f}},
      {'O', {0x3e, 0x41, 0x41, 0x41, 0x3e}},
      {'P', {0x7f, 0x09, 0x09, 0x09, 0x06}},
      {'Q', {0x3e, 0x41, 0x51, 0x21, 0x5e}},
      {'R', {0x7f, 0x09, 0x19, 0x29, 0x46}},
      {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
      {'T', {0x01, 0x01, 0x7f, 0x01, 0x01}},
      {'U', {0x3f, 0x40, 0x40, 0x40, 0x3f}},
      {'V', {0x1f, 0x20, 0x40, 0x20, 0x1f}},
      {'W', {0x3f, 0x40, 0x38, 0x40, 0x3f}},
      {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
      {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
      {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
      {'[', {0x00, 0x7f, 0x41, 0x41, 0x00}},
      {']', {0x00, 0x41, 0x41, 0x7f, 0x00}},
      {'^', {0x04, 0x02, 0x01, 0x02, 0x04}},
      {'_', {0x40, 0x40, 0x40, 0x40, 0x40}},
      {'|', {0x00, 0x00, 0x7f, 0x00, 0x00}},
      {'~', {0x08, 0x04, 0x08, 0x10, 0x08}},
  };
  const char uc = static_cast<char>(std::toupper(static_cast<unsigned char>(rawCh)));
  for (const auto& g : kTable) {
    if (g.ch == uc) return &g;
  }
  return &kTable[0];
}

// Abstract 1-bit pixel surface helper
template <typename SetPxFn>
inline void fillRect1Bit(int w, int h, SetPxFn setPx, int x, int y, int rw, int rh, bool fg) {
  const int x0 = std::max(0, x);
  const int y0 = std::max(0, y);
  const int x1 = std::min(w, x + rw);
  const int y1 = std::min(h, y + rh);
  for (int py = y0; py < y1; ++py) {
    for (int px = x0; px < x1; ++px) {
      setPx(px, py, fg);
    }
  }
}

template <typename SetPxFn>
inline void strokeRect1Bit(int w, int h, SetPxFn setPx, int x, int y, int rw, int rh, bool fg, int t = 1) {
  fillRect1Bit(w, h, setPx, x, y, rw, t, fg);
  fillRect1Bit(w, h, setPx, x, y + rh - t, rw, t, fg);
  fillRect1Bit(w, h, setPx, x, y, t, rh, fg);
  fillRect1Bit(w, h, setPx, x + rw - t, y, t, rh, fg);
}

inline int measureText5x7(const char* text, int scale = 1) {
  if (!text || !text[0]) return 0;
  const int n = static_cast<int>(std::strlen(text));
  return n * (5 * scale + scale) - scale;
}

enum class TextAlign : uint8_t { LEFT = 0, CENTER = 1, RIGHT = 2 };

template <typename SetPxFn>
inline void drawText5x7(int w, int h, SetPxFn setPx, const char* text, int x, int y,
                        int scale = 1, bool fg = true, TextAlign align = TextAlign::LEFT) {
  if (!text) return;
  const int totalW = measureText5x7(text, scale);
  int cursorX = x;
  if (align == TextAlign::CENTER) cursorX = x - totalW / 2;
  else if (align == TextAlign::RIGHT) cursorX = x - totalW;

  for (size_t i = 0; text[i] != '\0'; ++i) {
    const Glyph5x7* g = lookupGlyph5x7(text[i]);
    for (int col = 0; col < 5; ++col) {
      const uint8_t bits = g->cols[col];
      for (int row = 0; row < 7; ++row) {
        if (bits & (1u << row)) {
          if (scale <= 1) {
            const int px = cursorX + col;
            const int py = y + row;
            if (px >= 0 && px < w && py >= 0 && py < h) setPx(px, py, fg);
          } else {
            fillRect1Bit(w, h, setPx, cursorX + col * scale, y + row * scale, scale, scale, fg);
          }
        }
      }
    }
    cursorX += 5 * scale + scale;
  }
}

template <typename SetPxFn>
inline void drawOutlinedText5x7(int w, int h, SetPxFn setPx, const char* text, int x, int y,
                                int scale = 1, bool fg = true, TextAlign align = TextAlign::LEFT) {
  if (!text) return;
  const int totalW = measureText5x7(text, scale);
  int cursorX = x;
  if (align == TextAlign::CENTER) cursorX = x - totalW / 2;
  else if (align == TextAlign::RIGHT) cursorX = x - totalW;

  // Pass 1: 1px contrasting halo around every glyph pixel for readability over 1-bit photos
  const bool outline = !fg;
  int cx = cursorX;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    const Glyph5x7* g = lookupGlyph5x7(text[i]);
    for (int col = 0; col < 5; ++col) {
      const uint8_t bits = g->cols[col];
      for (int row = 0; row < 7; ++row) {
        if (bits & (1u << row)) {
          const int gx = cx + col * scale;
          const int gy = y + row * scale;
          fillRect1Bit(w, h, setPx, gx - 1, gy - 1, scale + 2, scale + 2, outline);
        }
      }
    }
    cx += 5 * scale + scale;
  }

  // Pass 2: crisp foreground glyph pixels
  drawText5x7(w, h, setPx, text, x, y, scale, fg, align);
}

template <typename SetPxFn>
inline void drawPill5x7(int w, int h, SetPxFn setPx, const char* text, int x, int y,
                        TextAlign align = TextAlign::LEFT, int padX = 4, int padY = 3,
                        bool fg = true, bool bg = false) {
  if (!text || !text[0]) return;
  const int tw = measureText5x7(text, 1);
  const int bw = tw + padX * 2;
  const int bh = 7 + padY * 2;
  int bx = x;
  if (align == TextAlign::CENTER) bx = x - bw / 2;
  else if (align == TextAlign::RIGHT) bx = x - bw;
  fillRect1Bit(w, h, setPx, bx, y, bw, bh, bg);
  drawText5x7(w, h, setPx, text, bx + padX, y + padY, 1, fg, TextAlign::LEFT);
}

template <typename SetPxFn>
inline void drawBadge5x7(int w, int h, SetPxFn setPx, const char* text, int x, int y,
                         bool inverted, float progress, TextAlign align = TextAlign::LEFT,
                         int padX = 4, int padY = 4, bool showProgress = true) {
  const int tw = measureText5x7(text, 1);
  const int bw = tw + padX * 2;
  const int bh = 7 + padY * 2;
  int bx = x;
  if (align == TextAlign::CENTER) bx = x - bw / 2;
  else if (align == TextAlign::RIGHT) bx = x - bw;

  const bool bg = inverted;
  const bool fg = !inverted;
  fillRect1Bit(w, h, setPx, bx, y, bw, bh, bg);
  if (inverted) {
    // Pressed pill: light fill with 1px dark outer border so it stays distinct on bright sky
    strokeRect1Bit(w, h, setPx, bx, y, bw, bh, false, 1);
  } else {
    // Unpressed badge: dark backing with 1px light inner border (preserves 1px dark outer halo)
    strokeRect1Bit(w, h, setPx, bx + 1, y + 1, bw - 2, bh - 2, true, 1);
  }
  if (showProgress && progress > 0.0f) {
    const float clamped = std::min(1.0f, std::max(0.0f, progress));
    const int maxBarW = std::max(1, bw - 4);
    int barW = static_cast<int>(maxBarW * clamped + 0.5f);
    if (barW < 1) barW = 1;
    fillRect1Bit(w, h, setPx, bx + 2, y + bh - 4, barW, 2, fg);
  }
  drawText5x7(w, h, setPx, text, bx + padX, y + padY, 1, fg, TextAlign::LEFT);
}

// Battery icon helper for both RLCD (400x300) and OLED (128x64):
// - If batteryPresent == false: draws outline + '?' / diagonal N/A mark (honest unknown/absent)
// - If batteryPresent == true: draws outline + 0..4 proportional fill bars for pct (0..100)
template <typename SetPxFn>
inline void drawBatteryIcon(int w, int h, SetPxFn setPx, int x, int y,
                            bool batteryPresent, int pct, bool fg = true, bool bg = false) {
  // Icon size: 16x9 body + 2x5 nub on right => 18x9 total
  const int bw = 15;
  const int bh = 9;
  fillRect1Bit(w, h, setPx, x, y, bw, bh, bg);
  strokeRect1Bit(w, h, setPx, x, y, bw, bh, fg, 1);
  fillRect1Bit(w, h, setPx, x + bw, y + 2, 2, 5, fg);

  if (!batteryPresent || pct < 0) {
    // Honest unknown / no-battery glyph: tiny '?' inside battery outline (3x5 pixels)
    // Pattern for '?' at (x+6, y+2)
    static const uint8_t q[5] = {0b110, 0b001, 0b010, 0b000, 0b010};
    for (int r = 0; r < 5; ++r) {
      for (int c = 0; c < 3; ++c) {
        if (q[r] & (1u << (2 - c))) {
          const int px = x + 6 + c;
          const int py = y + 2 + r;
          if (px >= 0 && px < w && py >= 0 && py < h) setPx(px, py, fg);
        }
      }
    }
    return;
  }

  const int clamped = std::min(100, std::max(0, pct));
  // Interior is (bw - 4) = 11 px wide, 5 px tall
  const int fillW = (clamped * 11 + 50) / 100;
  if (fillW > 0) {
    fillRect1Bit(w, h, setPx, x + 2, y + 2, fillW, 5, fg);
  }
}

}  // namespace flipcam
