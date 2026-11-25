// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Tab5Battery.h"
#include "Tab5CameraAdapter.h"

namespace opentoy {
namespace tab5 {

// Logical landscape canvas (640x360 -> 2x pixel-doubled to 1280x720 physical MIPI-DSI DPI).
constexpr int kUiWidth = 640;
constexpr int kUiHeight = 360;
constexpr int kStatusBarHeight = 28;

constexpr char kTab5FirmwareVersion[] = "0.2.0";
constexpr char kTab5VersionCreated[] = "2026-11-17";

enum class Tab5AppMode : uint8_t {
  CameraEye = 0,   // Default on boot: live 1280x720 Camera Eye viewfinder + dual-target reticle
  CopyFace = 1,    // Live SC202CS -> 320x180 HumanFaceDetect gaze-follow + honest mouth gate
  Eyes1 = 2,       // Shared Curious / Dreamy / Cheeky large-pupil eyes + touch pokes
};

enum class Tab5MenuPage : uint8_t {
  Play = 0,
  Root = 1,
  Apps = 2,
  Personality = 3,
  Sound = 4,
  Display = 5,
  Controls = 6,
  About = 7,
};

struct UiRect {
  int x = 0, y = 0, w = 0, h = 0;
  bool contains(int px, int py) const {
    return px >= x && px < (x + w) && py >= y && py < (y + h);
  }
};

struct UiTouchPoint {
  bool down = false;
  int x = -1;  // Logical [0..640)
  int y = -1;  // Logical [0..360)
};

struct FaceBoxOverlay {
  bool valid = false;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float confidence = 0.0f;
  bool keypoints_valid = false;
  float kp_x[5] = {};
  float kp_y[5] = {};
};

struct Tab5LiveTelemetry {
  bool cam_ready = false;
  bool cam_streaming = false;
  bool ae_warmed = false;
  uint16_t sensor_id = 0;
  uint32_t frame_seq = 0;
  uint32_t encode_us = 0;

  bool detector_ready = false;
  bool face_tracking = false;
  unsigned face_count = 0;
  uint32_t infer_ms = 0;
  float gaze_x = 0.0f;  // [-1, 1]
  float gaze_y = 0.0f;  // [-1, 1]
  FaceBoxOverlay primary_face{};

  const char* panel_name = "PROBING";
  bool display_ready = false;
  bool touch_ready = false;
  bool ina226_ok = false;
  int32_t vbus_mv = 0;
  int32_t current_ma = 0;
  bool es8388_ok = false;
  bool es7210_ok = false;
  bool bmi270_ok = false;
  bool mic_listening = false;
  bool sound_awake = false;
  float mic_rms = 0.0f;
};

inline const char* modeShortName(Tab5AppMode m) {
  switch (m) {
    case Tab5AppMode::CameraEye: return "CAMEYE";
    case Tab5AppMode::CopyFace: return "COPYFACE";
    case Tab5AppMode::Eyes1: return "EYES1";
  }
  return "CAMEYE";
}

// Compact 5x7 uppercase/digit/symbol font for crisp 640x360 rendering.
inline const uint8_t* glyph5x7Tab5(char c) {
  static const char kChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.-:%/() ";
  static const uint8_t kRows[][7] = {
      {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},      {14,17,1,2,4,8,31},
      {30,1,1,14,1,1,30},     {2,6,10,18,31,2,2},     {31,16,16,30,1,1,30},
      {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},       {14,17,17,14,17,17,14},
      {14,17,17,15,1,1,14},   {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
      {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31},
      {31,16,16,30,16,16,16}, {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
      {14,4,4,4,4,4,14},      {7,2,2,2,18,18,12},     {17,18,20,24,20,18,17},
      {16,16,16,16,16,16,31}, {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
      {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13},
      {30,17,17,30,20,18,17}, {15,16,16,14,1,1,30},   {31,4,4,4,4,4,4},
      {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},  {17,17,17,21,21,21,10},
      {17,17,10,4,10,17,17},  {17,17,10,4,4,4,4},     {31,1,2,4,8,16,31},
      {0,0,0,0,0,6,6},        {0,0,0,31,0,0,0},       {0,6,6,0,6,6,0},
      {25,25,2,4,8,19,19},    {1,2,4,8,16,0,0},       {2,4,8,8,8,4,2},
      {8,4,2,2,2,4,8},        {0,0,0,0,0,0,0},
  };
  static const uint8_t kBlank[7] = {};
  if (c >= 'a' && c <= 'z') c = char(c - 32);
  const char* p = std::strchr(kChars, c);
  return (c && p) ? kRows[p - kChars] : kBlank;
}

inline void uiFillRect(uint16_t* fb, int x, int y, int w, int h, uint16_t c) {
  if (!fb || w <= 0 || h <= 0) return;
  const int x0 = std::max(0, x);
  const int y0 = std::max(0, y);
  const int x1 = std::min(kUiWidth, x + w);
  const int y1 = std::min(kUiHeight, y + h);
  for (int py = y0; py < y1; ++py) {
    uint16_t* row = fb + py * kUiWidth;
    for (int px = x0; px < x1; ++px) row[px] = c;
  }
}

inline void uiDrawRect(uint16_t* fb, int x, int y, int w, int h, uint16_t c, int thick = 1) {
  for (int t = 0; t < thick; ++t) {
    uiFillRect(fb, x + t, y + t, w - 2 * t, 1, c);
    uiFillRect(fb, x + t, y + h - 1 - t, w - 2 * t, 1, c);
    uiFillRect(fb, x + t, y + t, 1, h - 2 * t, c);
    uiFillRect(fb, x + w - 1 - t, y + t, 1, h - 2 * t, c);
  }
}

inline void uiDrawText(uint16_t* fb,
                       int x,
                       int y,
                       const char* text,
                       uint16_t color,
                       int scale = 2,
                       int max_x = kUiWidth) {
  if (!fb || !text || scale < 1) return;
  int cx = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    const uint8_t* g = glyph5x7Tab5(text[i]);
    for (int r = 0; r < 7; ++r) {
      const uint8_t bits = g[r];
      for (int c = 0; c < 5; ++c) {
        if (bits & (16u >> c)) {
          uiFillRect(fb, cx + c * scale, y + r * scale, scale, scale, color);
        }
      }
    }
    cx += 6 * scale;
    if (cx + 5 * scale > max_x) break;
  }
}

inline int uiTextWidth(const char* text, int scale = 2) {
  if (!text || !text[0]) return 0;
  return int(std::strlen(text)) * 6 * scale - scale;
}

class Tab5UiState {
 public:
  static constexpr UiRect menuBtn() { return UiRect{332, 2, 100, 24}; }
  static constexpr UiRect restoreZone() { return UiRect{220, 0, 200, 44}; }
  static constexpr UiRect backBtn() { return UiRect{40, 300, 140, 44}; }
  static constexpr UiRect nextBtn() { return UiRect{460, 300, 140, 44}; }

  static UiRect rowRect(unsigned idx) {
    return UiRect{40, 72 + int(idx) * 54, 560, 46};
  }

  static UiRect leftEyeRect() { return UiRect{96, 80, 180, 210}; }
  static UiRect rightEyeRect() { return UiRect{364, 80, 180, 210}; }

  Tab5AppMode mode = Tab5AppMode::CameraEye;
  Tab5MenuPage page = Tab5MenuPage::Play;
  unsigned rootPage = 0;        // 0..1 (4 items on page 0, 3 items on page 1 -> 7 canonical entries)
  unsigned persona = 1;         // 1=Curious, 2=Dreamy, 3=Cheeky
  bool header = true;           // Default visible on boot; hidden in Performance mode
  bool mirrorGaze = true;       // Mirror Copy-Face horizontal gaze by default for front-facing feel
  bool flipLandscape = false;   // 180-deg landscape flip to match bench cable edge
  bool dimBacklight = false;
  bool micListening = false;    // Default OFF per privacy rules; opt-in via Menu or MIC 1
  unsigned pokeMask = 0;        // bit 0 = left eye, bit 1 = right eye
  int pressedControl = -1;
  uint32_t epoch = 1;

 private:
  bool holdingRestore_ = false;
  bool waitRelease_ = false;
  uint32_t holdStartMs_ = 0;
  int lastDownX_ = -1;
  int lastDownY_ = -1;

  void transition(bool require_release = false) {
    ++epoch;
    pokeMask = 0;
    pressedControl = -1;
    holdingRestore_ = false;
    waitRelease_ = require_release;
  }

  int hitTest(int x, int y) const {
    if (x < 0 || x >= kUiWidth || y < 0 || y >= kUiHeight) return -1;
    if (page == Tab5MenuPage::Play) {
      if (!header) return restoreZone().contains(x, y) ? 99 : -1;
      return menuBtn().contains(x, y) ? 98 : -1;
    }
    if (backBtn().contains(x, y)) return 90;
    if (page == Tab5MenuPage::Root && nextBtn().contains(x, y)) return 91;
    for (unsigned i = 0; i < 4u; ++i) {
      if (rowRect(i).contains(x, y)) {
        if (page == Tab5MenuPage::Root && (rootPage * 4u + i) >= 7u) return -1;
        return int(i);
      }
    }
    return -1;
  }

  void activateRow(unsigned r) {
    if (page == Tab5MenuPage::Root) {
      const unsigned idx = rootPage * 4u + r;
      switch (idx) {
        case 0: page = Tab5MenuPage::Play; transition(); break;
        case 1: page = Tab5MenuPage::Apps; transition(); break;
        case 2: page = Tab5MenuPage::Personality; transition(); break;
        case 3: page = Tab5MenuPage::Sound; transition(); break;
        case 4: page = Tab5MenuPage::Display; transition(); break;
        case 5: page = Tab5MenuPage::Controls; transition(); break;
        case 6: page = Tab5MenuPage::About; transition(); break;
      }
      return;
    }
    if (page == Tab5MenuPage::Apps) {
      if (r < 3u) {
        mode = Tab5AppMode(r);
        page = Tab5MenuPage::Play;
        transition();
      }
      return;
    }
    if (page == Tab5MenuPage::Personality) {
      if (r < 3u) {
        persona = r + 1u;
        ++epoch;
      }
      return;
    }
    if (page == Tab5MenuPage::Sound) {
      if (r == 0) {
        micListening = !micListening;
        ++epoch;
      }
      return;
    }
    if (page == Tab5MenuPage::Display) {
      if (r == 0) {
        header = false;
        page = Tab5MenuPage::Play;
        transition();
      } else if (r == 1) {
        mirrorGaze = !mirrorGaze;
        ++epoch;
      } else if (r == 2) {
        flipLandscape = !flipLandscape;
        ++epoch;
      } else if (r == 3) {
        dimBacklight = !dimBacklight;
        ++epoch;
      }
      return;
    }
  }

 public:
  bool playing() const { return page == Tab5MenuPage::Play; }

  void openMenu() {
    header = true;
    page = Tab5MenuPage::Root;
    rootPage = 0;
    transition();
  }

  void back() {
    if (page == Tab5MenuPage::Root) {
      page = Tab5MenuPage::Play;
      transition();
    } else if (page != Tab5MenuPage::Play) {
      page = Tab5MenuPage::Root;
      transition();
    }
  }

  void nextRootPage() {
    if (page == Tab5MenuPage::Root) {
      rootPage = (rootPage + 1u) % 2u;
      ++epoch;
    }
  }

  void setMode(Tab5AppMode m) {
    mode = m;
    page = Tab5MenuPage::Play;
    transition();
  }

  void togglePerformance() {
    if (playing() && header) {
      header = false;
      transition();
    } else {
      openMenu();
    }
  }

  void loseInput() {
    pokeMask = 0;
    pressedControl = -1;
    holdingRestore_ = false;
    waitRelease_ = false;
  }

  void update(const UiTouchPoint& raw_t, uint32_t now_ms, bool valid = true) {
    if (!valid) {
      loseInput();
      return;
    }
    UiTouchPoint t = raw_t;
    if (t.down) {
      lastDownX_ = t.x;
      lastDownY_ = t.y;
    } else if (t.x < 0 || t.y < 0) {
      t.x = lastDownX_;
      t.y = lastDownY_;
    }

    if (waitRelease_) {
      if (!t.down) waitRelease_ = false;
      pokeMask = 0;
      return;
    }

    if (playing()) {
      if (!header) {
        if (t.down && restoreZone().contains(t.x, t.y)) {
          if (!holdingRestore_) {
            holdingRestore_ = true;
            holdStartMs_ = now_ms;
          } else if (uint32_t(now_ms - holdStartMs_) >= 800u) {
            header = true;
            page = Tab5MenuPage::Root;
            rootPage = 0;
            transition(true);
          }
          pokeMask = 0;
          return;
        }
        holdingRestore_ = false;
      } else {
        if (pressedControl == 98) {
          pokeMask = 0;
          if (!t.down) {
            if (menuBtn().contains(t.x, t.y)) {
              openMenu();
            } else {
              pressedControl = -1;
            }
          } else if (!menuBtn().contains(t.x, t.y)) {
            pressedControl = -1;
            waitRelease_ = true;
          }
          return;
        }
        if (t.down && menuBtn().contains(t.x, t.y)) {
          pressedControl = 98;
          pokeMask = 0;
          return;
        }
      }

      // Direct Play interactions (eye pokes in Eyes1 / CopyFace).
      pokeMask = 0;
      if (t.down && mode != Tab5AppMode::CameraEye && (!header || t.y >= kStatusBarHeight)) {
        if (leftEyeRect().contains(t.x, t.y)) pokeMask = 1u;
        else if (rightEyeRect().contains(t.x, t.y)) pokeMask = 2u;
      }
      return;
    }

    // Menu navigation: release inside same target activates; drag-out cancels.
    pokeMask = 0;
    if (t.down) {
      const int hit = hitTest(t.x, t.y);
      if (pressedControl < 0) {
        pressedControl = hit;
        if (pressedControl < 0) waitRelease_ = true;
      } else if (hit != pressedControl) {
        pressedControl = -1;
        waitRelease_ = true;
      }
      return;
    }

    if (pressedControl >= 0) {
      const int act = pressedControl;
      pressedControl = -1;
      if (hitTest(t.x, t.y) != act) return;
      if (act == 90) back();
      else if (act == 91) nextRootPage();
      else if (act >= 0 && act < 4) activateRow(unsigned(act));
    }
  }
};

// Draws smooth rounded large-pupil eyes into 640x360 canvas.
inline void renderSmoothEyes(uint16_t* fb,
                             float gaze_x,
                             float gaze_y,
                             unsigned persona_id,
                             unsigned poke_mask,
                             bool sound_awake,
                             uint32_t now_ms,
                             int y_center = 186) {
  const uint16_t eye_ink =
      (persona_id == 2) ? 0xff7b : (persona_id == 3) ? 0xcff9 : 0x07ff;

  // Periodic natural blink when not poked.
  const uint32_t period = (persona_id == 1) ? 2800u : (persona_id == 2) ? 4200u : 3300u;
  const uint32_t phase = now_ms % period;
  float auto_lid = 0.0f;
  if (!poke_mask && phase < 160u) {
    auto_lid = (phase < 70u) ? (float(phase) / 70.0f) : (float(160u - phase) / 90.0f);
  }

  for (unsigned e = 0; e < 2u; ++e) {
    const bool poked = (poke_mask & (1u << e)) != 0;
    const int cx = (e == 0) ? 196 : 444;
    const int cy = y_center + ((persona_id == 3) ? (e == 0 ? -4 : 4) : 0);
    const int hw = (persona_id == 1) ? 68 : 74;
    const int hh = (persona_id == 2) ? 68 : (sound_awake ? 86 : 78);
    const int rad = 22;

    if (poked) {
      // Closed angry squinting bar with deterministic tremble.
      const int jx = int((now_ms / 29u + e * 3u) % 5u) - 2;
      uiFillRect(fb, cx - hw + jx, cy - 6, hw * 2, 12, 0xf800);
      continue;
    }

    const int lid_cut = int(std::lround(auto_lid * float(hh * 2)));
    const int px_off = int(std::lround(std::max(-1.0f, std::min(1.0f, gaze_x)) * 26.0f));
    const int py_off = int(std::lround(std::max(-1.0f, std::min(1.0f, gaze_y)) * 22.0f));
    const int pup_x = cx + px_off;
    const int pup_y = cy + py_off;
    const int pup_r2 = 34 * 34;

    for (int dy = -hh; dy <= hh; ++dy) {
      if (dy + hh < lid_cut) continue;
      const int py = cy + dy;
      if (py < 0 || py >= kUiHeight) continue;
      for (int dx = -hw; dx <= hw; ++dx) {
        const int px = cx + dx;
        if (px < 0 || px >= kUiWidth) continue;
        // Rounded-rectangle SDF check
        const int qx = std::max(0, std::abs(dx) - (hw - rad));
        const int qy = std::max(0, std::abs(dy) - (hh - rad));
        if (qx * qx + qy * qy > rad * rad) continue;

        // Happy raised lower lid & clean pupil cutout
        const int dpx = px - pup_x;
        const int dpy = py - pup_y;
        if (dpx * dpx + dpy * dpy <= pup_r2) {
          fb[py * kUiWidth + px] = 0x0000;
        } else {
          fb[py * kUiWidth + px] = eye_ink;
        }
      }
    }
  }
}

inline void renderTab5StatusBar(uint16_t* fb,
                                const Tab5UiState& ui,
                                const BatterySample& bat,
                                uint32_t now_ms) {
  uiFillRect(fb, 0, 0, kUiWidth, kStatusBarHeight, 0x1082);
  uiFillRect(fb, 0, kStatusBarHeight - 1, kUiWidth, 1, 0x3186);

  char left_str[48] = {};
  std::snprintf(left_str,
                sizeof(left_str),
                "%s V%s %s",
                modeShortName(ui.mode),
                kTab5FirmwareVersion,
                kTab5VersionCreated);
  uiDrawText(fb, 10, 7, left_str, 0x07ff, 2, 328);

  // Center-right [MENU] button
  const bool mp = (ui.pressedControl == 98);
  const UiRect mb = Tab5UiState::menuBtn();
  uiFillRect(fb, mb.x, mb.y, mb.w, mb.h, mp ? 0x07ff : 0x2104);
  uiDrawRect(fb, mb.x, mb.y, mb.w, mb.h, 0x07ff, 1);
  uiDrawText(fb, mb.x + 26, 7, "MENU", mp ? 0x0000 : 0xffff, 2);

  // Right-aligned truthful battery text (never an empty-battery symbol when unavailable).
  char bat_txt[24] = {};
  (void)formatBatteryStatus(bat, now_ms, bat_txt, sizeof(bat_txt));
  const int bw = uiTextWidth(bat_txt, 2);
  uiDrawText(fb, kUiWidth - bw - 10, 7, bat_txt, 0xbefb, 2);
}

// Renders complete 640x360 logical RGB565 frame. preview320x180 may be nullptr if camera unavailable.
inline void renderTab5Frame(uint16_t* fb640x360,
                            const Tab5UiState& ui,
                            const BatterySample& bat,
                            const Tab5LiveTelemetry& tel,
                            const uint16_t* preview320x180,
                            uint32_t now_ms) {
  if (!fb640x360) return;
  std::fill(fb640x360, fb640x360 + kUiWidth * kUiHeight, uint16_t(0x0000));

  if (ui.header || !ui.playing()) {
    renderTab5StatusBar(fb640x360, ui, bat, now_ms);
  }

  if (!ui.playing()) {
    auto drawBtn = [&](const UiRect& r, const char* label, bool pressed, bool active = false) {
      uiFillRect(fb640x360, r.x, r.y, r.w, r.h, pressed ? 0x07ff : 0x18c3);
      uiDrawRect(fb640x360, r.x, r.y, r.w, r.h, (pressed || active) ? 0xffff : 0x6b4d, 2);
      int tx = r.x + 16;
      if (active) {
        uiFillRect(fb640x360, r.x + 8, r.y + 15, 4, 16, pressed ? 0x0000 : 0x07ff);
        tx = r.x + 20;
      }
      uiDrawText(fb640x360, tx, r.y + 16, label, pressed ? 0x0000 : 0xffff, 2, r.x + r.w - 10);
    };

    static const char* const kRootLabels[7] = {
        "RETURN TO PLAY",
        "APPS (CAMEYE / COPYFACE / EYES1)",
        "PERSONALITY",
        "SOUND AND MICROPHONE",
        "DISPLAY AND PERFORMANCE",
        "CONTROLS",
        "ABOUT HARDWARE",
    };

    if (ui.page == Tab5MenuPage::Root) {
      char title[32] = {};
      std::snprintf(title, sizeof(title), "MENU %u OF 2", ui.rootPage + 1u);
      uiDrawText(fb640x360, 40, 42, title, 0x07ff, 2);
      for (unsigned i = 0; i < 4u; ++i) {
        const unsigned idx = ui.rootPage * 4u + i;
        if (idx < 7u) {
          drawBtn(Tab5UiState::rowRect(i), kRootLabels[idx], ui.pressedControl == int(i), idx == 0);
        }
      }
      drawBtn(Tab5UiState::nextBtn(), "NEXT", ui.pressedControl == 91, false);
    } else if (ui.page == Tab5MenuPage::Apps) {
      uiDrawText(fb640x360, 40, 42, "APPS ON THIS TAB5 BUILD", 0x07ff, 2);
      drawBtn(Tab5UiState::rowRect(0),
              "VIBE CODING CAMERA EYE (USB + LIVE VIEW)",
              ui.pressedControl == 0,
              ui.mode == Tab5AppMode::CameraEye);
      drawBtn(Tab5UiState::rowRect(1),
              "COPY-FACE (SC202CS 5-PT GAZE FOLLOW)",
              ui.pressedControl == 1,
              ui.mode == Tab5AppMode::CopyFace);
      drawBtn(Tab5UiState::rowRect(2),
              "ANIMATRONIC EYES 1 (TOUCH + PERSONAS)",
              ui.pressedControl == 2,
              ui.mode == Tab5AppMode::Eyes1);
      uiDrawText(fb640x360, 40, 290 - 40, "OTHER CATALOG APPS REQUIRE SEPARATE INSTALL", 0xbefb, 1);
    } else if (ui.page == Tab5MenuPage::Personality) {
      uiDrawText(fb640x360, 40, 42, "PERSONALITY", 0x07ff, 2);
      drawBtn(Tab5UiState::rowRect(0), "CURIOUS", ui.pressedControl == 0, ui.persona == 1);
      drawBtn(Tab5UiState::rowRect(1), "DREAMY", ui.pressedControl == 1, ui.persona == 2);
      drawBtn(Tab5UiState::rowRect(2), "CHEEKY", ui.pressedControl == 2, ui.persona == 3);
    } else if (ui.page == Tab5MenuPage::Sound) {
      uiDrawText(fb640x360, 40, 42, "SOUND AND LISTENING", 0x07ff, 2);
      drawBtn(Tab5UiState::rowRect(0),
              ui.micListening ? "ES7210 MIC WAKE: ON (VOLATILE RAM)" : "ES7210 MIC WAKE: OFF (DEFAULT)",
              ui.pressedControl == 0,
              ui.micListening);
      char codec_line[64] = {};
      std::snprintf(codec_line,
                    sizeof(codec_line),
                    "CODECS: ES7210=%s ES8388=%s RMS=%.3f",
                    tel.es7210_ok ? "OK" : "NO",
                    tel.es8388_ok ? "OK" : "NO",
                    double(tel.mic_rms));
      uiDrawText(fb640x360, 40, 140, codec_line, 0xbefb, 2);
      uiDrawText(fb640x360, 40, 170, "NO AUDIO RECORDING OR UPLOAD. SPEAKER AMP OFF IDLE.", 0xbefb, 1);
    } else if (ui.page == Tab5MenuPage::Display) {
      uiDrawText(fb640x360, 40, 42, "DISPLAY (HOLD TOP-CENTER 0.8S TO EXIT PERF)", 0x07ff, 2);
      drawBtn(Tab5UiState::rowRect(0), "ENTER PERFORMANCE MODE", ui.pressedControl == 0, false);
      drawBtn(Tab5UiState::rowRect(1),
              ui.mirrorGaze ? "COPY-FACE MIRROR: ON" : "COPY-FACE MIRROR: OFF",
              ui.pressedControl == 1,
              ui.mirrorGaze);
      drawBtn(Tab5UiState::rowRect(2),
              ui.flipLandscape ? "LANDSCAPE ORIENTATION: FLIPPED" : "LANDSCAPE ORIENTATION: STANDARD",
              ui.pressedControl == 2,
              ui.flipLandscape);
      drawBtn(Tab5UiState::rowRect(3),
              ui.dimBacklight ? "BACKLIGHT: DIM (25%)" : "BACKLIGHT: NORMAL (75%)",
              ui.pressedControl == 3,
              !ui.dimBacklight);
    } else if (ui.page == Tab5MenuPage::Controls) {
      uiDrawText(fb640x360, 40, 42, "CONTROLS", 0x07ff, 2);
      uiDrawText(fb640x360, 40, 90, "TAP MENU IN TOP BAR TO SWITCH MODES AND SETTINGS", 0xffff, 2);
      uiDrawText(fb640x360, 40, 125, "POKE LEFT OR RIGHT EYE IN EYES1 / COPY-FACE", 0xbefb, 2);
      uiDrawText(fb640x360, 40, 160, "HOLD TOP-CENTER 0.8S TO RESTORE FROM PERFORMANCE", 0xbefb, 2);
      uiDrawText(fb640x360, 40, 195, "USB OTCAM1: STATUS SNAP BURST HW FACE MIC MODE", 0x07ff, 2);
    } else if (ui.page == Tab5MenuPage::About) {
      uiDrawText(fb640x360, 40, 42, "ABOUT M5STACK TAB5 HARDWARE", 0x07ff, 2);
      char l1[64] = {}, l2[64] = {}, l3[64] = {};
      std::snprintf(l1,
                    sizeof(l1),
                    "PANEL: %s  CAM: 0X%04X SEQ:%lu",
                    tel.panel_name ? tel.panel_name : "NONE",
                    unsigned(tel.sensor_id),
                    (unsigned long)tel.frame_seq);
      std::snprintf(l2,
                    sizeof(l2),
                    "INA226: %s VBUS:%ldMV  IMU:%s",
                    tel.ina226_ok ? "OK" : "NO",
                    (long)tel.vbus_mv,
                    tel.bmi270_ok ? "BMI270" : "NO");
      std::snprintf(l3,
                    sizeof(l3),
                    "FACE DET: %s INFER:%luMS",
                    tel.detector_ready ? "MSRMNP-0.5.2" : "OFF",
                    (unsigned long)tel.infer_ms);
      uiDrawText(fb640x360, 40, 90, l1, 0xffff, 2);
      uiDrawText(fb640x360, 40, 125, l2, 0xbefb, 2);
      uiDrawText(fb640x360, 40, 160, l3, 0xbefb, 2);
    }

    drawBtn(Tab5UiState::backBtn(), "BACK", ui.pressedControl == 90, false);
    return;
  }

  // === PLAY MODES ===
  if (ui.mode == Tab5AppMode::CameraEye) {
    // Live viewfinder centered at (80, 44, 480x270 = 1.5x of 320x180)
    const int vx0 = 80;
    const int vy0 = ui.header ? 42 : 24;
    const int vw = 480;
    const int vh = 270;
    if (preview320x180 && tel.cam_streaming) {
      for (int y = 0; y < vh; ++y) {
        const int sy = (y * 180) / vh;
        const uint16_t* srow = preview320x180 + sy * 320;
        uint16_t* drow = fb640x360 + (vy0 + y) * kUiWidth + vx0;
        for (int x = 0; x < vw; ++x) {
          drow[x] = srow[(x * 320) / vw];
        }
      }
      // Framing reticle for dual-target bench inspection
      uiDrawRect(fb640x360, vx0, vy0, vw, vh, 0x07ff, 2);
      uiFillRect(fb640x360, vx0 + vw / 2 - 12, vy0 + vh / 2, 25, 1, 0x07e0);
      uiFillRect(fb640x360, vx0 + vw / 2, vy0 + vh / 2 - 12, 1, 25, 0x07e0);
      if (tel.primary_face.valid) {
        const int fx = vx0 + int(tel.primary_face.x * float(vw));
        const int fy = vy0 + int(tel.primary_face.y * float(vh));
        const int fw = std::max(8, int(tel.primary_face.w * float(vw)));
        const int fh = std::max(8, int(tel.primary_face.h * float(vh)));
        uiDrawRect(fb640x360, fx, fy, fw, fh, 0x07e0, 2);
      }
    } else {
      uiDrawRect(fb640x360, vx0, vy0, vw, vh, 0xf800, 2);
      uiDrawText(fb640x360, vx0 + 90, vy0 + 125, "CAMERA STREAM UNAVAILABLE", 0xf800, 2);
    }

    if (ui.header) {
      char foot[72] = {};
      std::snprintf(foot,
                    sizeof(foot),
                    "OTCAM1 READY  SEQ:%lu AE:%u ENC:%luUS FACES:%u",
                    (unsigned long)tel.frame_seq,
                    tel.ae_warmed ? 1u : 0u,
                    (unsigned long)tel.encode_us,
                    tel.face_count);
      uiDrawText(fb640x360, 80, 326, foot, 0xbefb, 2);
    }
    return;
  }

  if (ui.mode == Tab5AppMode::CopyFace) {
    renderSmoothEyes(fb640x360,
                     tel.face_tracking ? tel.gaze_x : 0.0f,
                     tel.face_tracking ? tel.gaze_y : 0.0f,
                     ui.persona,
                     ui.pokeMask,
                     tel.sound_awake,
                     now_ms,
                     ui.header ? 172 : 165);

    // Honest neutral mouth bar: 5-point detector drives gaze only; dense lip model unqualified.
    uiFillRect(fb640x360, 260, 286, 120, 6, tel.face_tracking ? 0x07ff : 0x528a);

    if (ui.header) {
      // Developer live camera inset at bottom-right (160x90)
      const int ix0 = 468, iy0 = 262, iw = 160, ih = 90;
      if (preview320x180 && tel.cam_streaming) {
        for (int y = 0; y < ih; ++y) {
          const uint16_t* srow = preview320x180 + (y * 2) * 320;
          uint16_t* drow = fb640x360 + (iy0 + y) * kUiWidth + ix0;
          for (int x = 0; x < iw; ++x) {
            const int sx = ui.mirrorGaze ? (319 - x * 2) : (x * 2);
            drow[x] = srow[sx];
          }
        }
        if (tel.primary_face.valid) {
          const float bx = ui.mirrorGaze
              ? (1.0f - tel.primary_face.x - tel.primary_face.w)
              : tel.primary_face.x;
          uiDrawRect(fb640x360,
                     ix0 + int(bx * float(iw)),
                     iy0 + int(tel.primary_face.y * float(ih)),
                     std::max(6, int(tel.primary_face.w * float(iw))),
                     std::max(6, int(tel.primary_face.h * float(ih))),
                     0x07e0,
                     1);
        }
      }
      uiDrawRect(fb640x360, ix0, iy0, iw, ih, tel.face_tracking ? 0x07e0 : 0x6b4d, 1);

      char st1[64] = {};
      std::snprintf(st1,
                    sizeof(st1),
                    "%s  INFER:%luMS",
                    tel.face_tracking ? "TRACKING FACE GAZE" : "NO FACE (NEUTRAL FALLBACK)",
                    (unsigned long)tel.infer_ms);
      uiDrawText(fb640x360, 14, 316, st1, tel.face_tracking ? 0x07e0 : 0xbefb, 1);
      uiDrawText(fb640x360, 14, 332, "MOUTH: 5-PT GAZE ONLY (LIP MODEL UNQUALIFIED)", 0xfd20, 1);
    }
    return;
  }

  // Eyes1 mode: autonomous gentle gaze + blinks + touch pokes + optional SoundWake alertness.
  const float idle_gx = 0.45f * std::sin(float(now_ms % 6283u) * 0.001f);
  const float idle_gy = 0.22f * std::cos(float(now_ms % 4712u) * 0.00133f);
  renderSmoothEyes(fb640x360,
                   tel.sound_awake ? 0.0f : idle_gx,
                   tel.sound_awake ? 0.0f : idle_gy,
                   ui.persona,
                   ui.pokeMask,
                   tel.sound_awake,
                   now_ms,
                   ui.header ? 190 : 180);
}

// Blits 640x360 logical landscape RGB565 frame into Tab5's native 720x1280 portrait DPI buffer.
inline void blitLandscape640x360ToPortrait720x1280(const uint16_t* src640x360,
                                                   uint16_t* dst720x1280,
                                                   bool flip_180) {
  if (!src640x360 || !dst720x1280) return;
  if (!flip_180) {
    // Standard landscape: logical (lx, ly) -> physical portrait px = 719 - 2*ly, py = 2*lx.
    for (int ly = 0; ly < kUiHeight; ++ly) {
      const uint16_t* srow = src640x360 + ly * kUiWidth;
      const int px0 = 718 - 2 * ly;
      for (int lx = 0; lx < kUiWidth; ++lx) {
        const uint16_t c = srow[lx];
        const int py0 = 2 * lx;
        uint16_t* d0 = dst720x1280 + py0 * 720 + px0;
        uint16_t* d1 = d0 + 720;
        d0[0] = c;
        d0[1] = c;
        d1[0] = c;
        d1[1] = c;
      }
    }
  } else {
    // Flipped 180-deg landscape: px = 2*ly, py = 1278 - 2*lx.
    for (int ly = 0; ly < kUiHeight; ++ly) {
      const uint16_t* srow = src640x360 + ly * kUiWidth;
      const int px0 = 2 * ly;
      for (int lx = 0; lx < kUiWidth; ++lx) {
        const uint16_t c = srow[lx];
        const int py0 = 1278 - 2 * lx;
        uint16_t* d0 = dst720x1280 + py0 * 720 + px0;
        uint16_t* d1 = d0 + 720;
        d0[0] = c;
        d0[1] = c;
        d1[0] = c;
        d1[1] = c;
      }
    }
  }
}

// Maps raw portrait touch (tx in 0..719, ty in 0..1279) into logical 640x360 coordinates,
// matching blitLandscape640x360ToPortrait720x1280() identically!
inline UiTouchPoint mapPortraitTouchToLandscape(bool down, int tx_720, int ty_1280, bool flip_180) {
  UiTouchPoint out;
  out.down = down;
  if (tx_720 < 0 || tx_720 >= 720 || ty_1280 < 0 || ty_1280 >= 1280) {
    return out;
  }
  if (!flip_180) {
    out.x = ty_1280 / 2;
    out.y = (719 - tx_720) / 2;
  } else {
    out.x = (1279 - ty_1280) / 2;
    out.y = tx_720 / 2;
  }
  return out;
}

}  // namespace tab5
}  // namespace opentoy
