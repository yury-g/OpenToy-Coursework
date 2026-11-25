// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "UnoQTftAdapter.h"
#include "Companion.h"

namespace opentoy {
namespace uno_q {

constexpr uint16_t kRgbBg = 0x0000;
constexpr uint16_t kRgbInk = 0xFFFF;
constexpr uint16_t kRgbHeaderBg = 0x10A2;
constexpr uint16_t kRgbHeaderInk = 0x07FF;
constexpr uint16_t kRgbBatteryInk = 0xBDF7;
constexpr char kUnoQAppName[] = "EYES1";
#if defined(OPENTOY_UNOQ_MAR2808)
constexpr char kUnoQVersion[] = "0.1.0";
#else
constexpr char kUnoQVersion[] = "0.1.0-src";
#endif
constexpr char kUnoQDate[] = "2026-11-17";

inline uint16_t blendRgb565(uint16_t bg, uint16_t fg, float cov) {
  cov = std::max(0.0f, std::min(1.0f, cov));
  const float inv = 1.0f - cov;
  const unsigned br = (bg >> 11) & 31, bgc = (bg >> 5) & 63, bb = bg & 31;
  const unsigned fr = (fg >> 11) & 31, fgc = (fg >> 5) & 63, fb = fg & 31;
  const unsigned r = unsigned(br * inv + fr * cov + 0.5f);
  const unsigned g = unsigned(bgc * inv + fgc * cov + 0.5f);
  const unsigned b = unsigned(bb * inv + fb * cov + 0.5f);
  return uint16_t((r << 11) | (g << 5) | b);
}

class UnoQEyesApp {
 public:
  explicit UnoQEyesApp(const ShieldDescriptor& desc = {}, unsigned persona_id = 1)
      : UnoQEyesApp(desc.width, desc.height, persona_id, desc.touch_qualified,
                    desc.battery_sensor_qualified) {}

  UnoQEyesApp(uint16_t width, uint16_t height, unsigned persona_id = 1,
              bool touch_qualified = false, bool battery_sensor_qualified = false)
      : width_(width),
        height_(height),
        touch_qualified_(touch_qualified),
        battery_sensor_qualified_(battery_sensor_qualified),
        persona_(feather_face::personality(persona_id).id),
        companion_(persona_id) {}

  void setPersonality(unsigned id) {
    persona_ = feather_face::personality(id).id;
    companion_ = feather_face::Companion(persona_);
  }

  unsigned personality() const { return persona_; }

  bool performanceMode() const { return performance_; }
  void setPerformanceMode(bool perf) { performance_ = perf; }

  bool headerVisible() const { return !performance_; }

  // Touch input is gated strictly on shield touch qualification.
  bool applyQualifiedTouch(uint16_t x, uint16_t /*y*/, uint32_t now_ms) {
    if (!touch_qualified_) {
      return false;
    }
    synthetic_poke_ = (x < width_ / 2u) ? 1u : 2u;
    poke_until_ms_ = now_ms + 300u;
    return true;
  }

  feather_face::CompanionPose step(uint32_t now_ms) {
    if (int32_t(poke_until_ms_ - now_ms) <= 0) {
      synthetic_poke_ = 0;
    }
    const unsigned mask = performance_ ? 0u : synthetic_poke_;
    return companion_.update(512, 512, 0, false, now_ms, mask);
  }

  void render(const feather_face::CompanionPose& pose,
              uint32_t now_ms,
              std::vector<uint16_t>& fb,
              int bat_mv = -1,
              uint32_t bat_age_ms = 0) const {
    fb.resize(size_t(width_) * height_);
    render(pose, now_ms, fb.data(), bat_mv, bat_age_ms);
  }

  // The target uses a static framebuffer so it does not need a large heap.
  void render(const feather_face::CompanionPose& pose,
              uint32_t now_ms,
              uint16_t* fb,
              int bat_mv = -1,
              uint32_t bat_age_ms = 0) const {
    const int w = width_;
    const int h = height_;
    std::fill_n(fb, size_t(w) * size_t(h), kRgbBg);

    const int hdr_h = headerVisible() ? std::max(24, h / 10) : 0;
    if (hdr_h > 0) {
      for (int y = 0; y < hdr_h; ++y) {
        std::fill_n(fb + size_t(y) * w, w, kRgbHeaderBg);
      }
      char left[48] = {};
      std::snprintf(left, sizeof(left), "%s v%s %s", kUnoQAppName, kUnoQVersion, kUnoQDate);
      drawTinyText(fb, w, h, 8, (hdr_h - 7) / 2, left, kRgbHeaderInk, 1);

      char bat[12] = {};
      formatUnoQBattery(bat_mv, bat_age_ms, battery_sensor_qualified_, bat);
      const int right_x = std::max(8, w - int(std::strlen(bat)) * 6 - 8);
      drawTinyText(fb, w, h, right_x, (hdr_h - 7) / 2, bat, kRgbBatteryInk, 1);
    }

    const int avail_h = h - hdr_h;
    const float scale = std::min(float(w) / 116.0f, float(avail_h) / 48.0f);
    const float cy_base = float(hdr_h) + float(avail_h) * 0.52f;
    const float sep = float(w) * 0.25f;
    const float cx_base = float(w) * 0.5f;

    for (unsigned eye = 0; eye < 2; ++eye) {
      const auto& face = pose.eye[eye];
      const auto& profile = feather_face::personality(pose.personality);
      const auto shake = nesso_eyes::jitter(now_ms, pose.mood[eye], eye);
      const float wx = profile.eyeWidth[eye] / 28.0f;
      const float hy = profile.eyeHeight[eye] / 32.0f;
      const float native = scale * std::min(wx, hy);
      const unsigned lid = eye ? face.right : face.left;
      const float pupilX = 14.0f + face.pupilX * 6.0f / 100.0f;
      const float pupilY = 16.0f + face.pupilY * 7.0f / 100.0f;
      const float upper = 16.0f * nesso_eyes::pupilClearMood(face, pose.mood[eye]) / 100.0f;
      const float slope = 12.0f * std::max(0, -pose.mood[eye]) / 2800.0f;
      const float slopeNorm = std::sqrt(1.0f + slope * slope);

      const float cx = cx_base + (eye ? sep : -sep) + shake.x * (scale * 0.4f);
      const float cy = cy_base + (face.y * 0.10f + shake.y + profile.eyeOffset[eye]) * scale;

      const int half_w = int(std::ceil(18.0f * wx * scale)) + 2;
      const int half_h = int(std::ceil(20.0f * hy * scale)) + 2;
      const int x0 = std::max(0, int(cx) - half_w);
      const int x1 = std::min(w, int(cx) + half_w + 1);
      const int y0 = std::max(hdr_h, int(cy) - half_h);
      const int y1 = std::min(h, int(cy) + half_h + 1);

      for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
          const float lx = (float(px) + 0.5f - cx) / scale + 14.0f;
          const float ly = (float(py) + 0.5f - cy) / scale + 16.0f;
          float d = 10.0f;
          if (lid >= 100) {
            const float dx = std::max(0.0f, std::fabs(float(px) + 0.5f - cx) - (profile.eyeWidth[eye] * scale * 0.45f));
            const float dy = float(py) + 0.5f - cy;
            d = std::sqrt(dx * dx + dy * dy) - std::max(1.5f, scale * 0.45f);
          } else {
            const float sx = (lx - 14.0f) / wx;
            const float sy = (ly - 16.0f) / hy;
            if (pose.originalShape) {
              d = (std::sqrt(sx * sx + sy * sy) - 14.0f) * native;
            } else {
              const float qx = std::fabs(sx) - 6.0f;
              const float qy = std::fabs(sy) - 8.0f;
              const float ox = std::max(qx, 0.0f);
              const float oy = std::max(qy, 0.0f);
              d = (std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.0f) - 8.0f) * native;
            }
            if (lid) {
              const float edge = 32.0f * float(lid) / 100.0f;
              d = std::max(d, (face.bottomLids ? ly - (32.0f - edge) : edge - ly) * scale);
            }
            if (!pose.originalShape) {
              if (pose.mood[eye] > 0) d = std::max(d, (upper - ly) * scale);
              if (pose.mood[eye] < 0) {
                d = std::max(d, ((eye ? 28.0f - lx : lx) * slope - ly) * scale / slopeNorm);
              }
            }
            if (face.pupils) {
              const float pdx = lx - pupilX;
              const float pdy = ly - pupilY;
              d = std::max(d, (8.0f - std::sqrt(pdx * pdx + pdy * pdy)) * scale);
            }
          }
          const float cov = 0.5f - d;
          if (cov > 0.0f) {
            fb[size_t(py) * w + px] = blendRgb565(fb[size_t(py) * w + px], kRgbInk, cov);
          }
        }
      }
    }
  }

 private:
  static const uint8_t* glyph5x7(char c) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.- ";
    static const uint8_t rows[][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},
        {31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},
        {31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},
        {31,1,2,4,8,16,31},{0,0,0,0,0,6,6},{0,0,0,31,0,0,0},{0,0,0,0,0,0,0},
    };
    static const uint8_t blank[7] = {};
    if (c >= 'a' && c <= 'z') c = char(c - 32);
    const char* p = std::strchr(chars, c);
    return (c && p) ? rows[p - chars] : blank;
  }

  static void drawTinyText(uint16_t* fb, int w, int h, int x, int y, const char* s, uint16_t color, int scale) {
    for (size_t idx = 0; s[idx] != '\0'; ++idx) {
      const uint8_t* bits = glyph5x7(s[idx]);
      const int gx = x + int(idx) * 6 * scale;
      for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
          if (bits[row] & (16 >> col)) {
            for (int sy = 0; sy < scale; ++sy) {
              for (int sx = 0; sx < scale; ++sx) {
                const int px = gx + col * scale + sx;
                const int py = y + row * scale + sy;
                if (px >= 0 && px < w && py >= 0 && py < h) {
                  fb[size_t(py) * w + px] = color;
                }
              }
            }
          }
        }
      }
    }
  }

  uint16_t width_;
  uint16_t height_;
  bool touch_qualified_;
  bool battery_sensor_qualified_;
  unsigned persona_ = 1;
  feather_face::Companion companion_;
  bool performance_ = false;
  unsigned synthetic_poke_ = 0;
  uint32_t poke_until_ms_ = 0;
};

}  // namespace uno_q
}  // namespace opentoy
