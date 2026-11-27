// SPDX-License-Identifier: MIT
#pragma once
#include "Companion.h"
#include "Interaction.h"
#include "Telemetry.h"
#include "TinyFont3x5.h"
#include "WaterView.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace touch169 {

class Scene {
  nesso_eyes::Reaction reaction;
  nesso_eyes::Idle idle{feather_face::Companion::settings(1)};
  WaterView water_;
  unsigned appliedPersona = 1;
  uint32_t pokeAt = 0;
  unsigned pokeMask = 0;

  static constexpr uint16_t ink = 0xe7ff;
  static constexpr uint16_t muted = 0xad55;
  static constexpr uint16_t cyan = 0x07ff;
  static constexpr uint16_t warn = 0xfbe4;
  static constexpr uint16_t bg = 0x1082;
  static constexpr uint16_t edge = 0x4a69;

  void text(uint16_t* b, int x, int y, const char* s, uint16_t c = ink, int scale = 2) {
    opentoy_tiny_ui::drawText3x5(b, width, height, x, y, s, c, scale, width);
  }

  void textOverlay(uint16_t* b, int x, int y, const char* s, uint16_t c = ink, int scale = 2) {
    opentoy_tiny_ui::drawText3x5Shadow(b, width, height, x, y, s, c, 0x0000, scale, width);
  }

  void box(uint16_t* b, int x, int y, int w, int h, const char* s) {
    opentoy_tiny_ui::fillRectBuf(b, width, height, x, y, w, h, bg);
    text(b, x + 8, y + (h - 10) / 2, s);
  }

  void overlayBox(uint16_t* b, Rect r, const char* s) {
    const int x0 = std::max(0, r.x), y0 = std::max(0, r.y);
    const int x1 = std::min(width, r.x + r.w), y1 = std::min(height, r.y + r.h);
    for (int py = y0; py < y1; ++py) {
      for (int px = x0; px < x1; ++px) {
        const bool border =
            (px == r.x || px == r.x + r.w - 1 || py == r.y || py == r.y + r.h - 1);
        if (border) {
          b[py * width + px] = edge;
        } else {
          // Darken underlying scene pixels for legible contrast without an opaque footer bar.
          const uint16_t c = b[py * width + px];
          b[py * width + px] = uint16_t((c >> 2) & 0x39e7);
        }
      }
    }
    const int tx = r.x + (r.w - opentoy_tiny_ui::textWidth3x5(s, 2)) / 2;
    const int ty = r.y + (r.h - 10) / 2;
    textOverlay(b, tx, ty, s, ink, 2);
  }

  void drawHeader(uint16_t* b, const Interaction& ui, const Diagnostics& d, bool overScene) {
    char line[48];
    std::snprintf(line, sizeof line, "%s V%s", appShortName(ui.app), version);
    if (overScene) textOverlay(b, 10, 8, line, cyan, 2);
    else text(b, 10, 8, line, cyan, 2);

    if (d.batteryMv >= 0) {
      std::snprintf(line, sizeof line, "BAT %.2fV", d.batteryMv / 1000.0);
    } else {
      std::snprintf(line, sizeof line, "BAT --");
    }
    const int batX = width - 10 - opentoy_tiny_ui::textWidth3x5(line, 2);
    if (overScene) textOverlay(b, batX, 8, line, ink, 2);
    else text(b, batX, 8, line, muted, 2);

    if (overScene) textOverlay(b, 10, 24, created, ink, 2);
    else text(b, 10, 24, created, muted, 2);

    if (ui.app == App::Water && !d.imuFresh) {
      const char* imuNote = d.imuPresent ? "IMU --" : "NO IMU";
      const int imuX = width - 10 - opentoy_tiny_ui::textWidth3x5(imuNote, 2);
      if (overScene) textOverlay(b, imuX, 24, imuNote, warn, 2);
      else text(b, imuX, 24, imuNote, warn, 2);
    }
  }

 public:
  WaterView& water() { return water_; }
  const WaterView& water() const { return water_; }

  void input(const Interaction& ui, uint32_t now) {
    if (ui.poke) {
      pokeMask = ui.poke;
      pokeAt = now;
    }
  }

  void render(uint16_t* b, const Interaction& ui, const Diagnostics& d, uint32_t now) {
    if (appliedPersona != ui.persona) {
      appliedPersona = ui.persona;
      idle = nesso_eyes::Idle(feather_face::Companion::settings(appliedPersona));
    }
    if (ui.page != Page::Play || ui.app != App::Face) pokeMask = 0;
    const bool touched = pokeMask && uint32_t(now - pokeAt) < 100;
    auto pose = reaction.update(now, touched, touched ? pokeMask : 0);
    auto ip = idle.update(now, touched || ui.page != Page::Play || ui.app != App::Face);

    char line[48];
    if (ui.page == Page::Play) {
      if (ui.app == App::Water) {
        water_.render(b);
      } else {
        std::fill(b, b + width * height, uint16_t(0));
        const float scale = 2.3f;
        const float cy = height / 2.0f;
        for (unsigned eye = 0; eye < 2; ++eye) {
          auto face = ip.apply(pose.face, eye);
          auto shake = nesso_eyes::jitter(now, pose.eyeMood[eye], eye);
          for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
              int lx = int(std::floor((x - (eye ? 184.f : 56.f)) / scale + 14)) - shake.x;
              int ly = int(std::floor((y - cy) / scale + 16)) - shake.y;
              if (feather_face::companionPixel(face, pose.eyeMood[eye], false, eye, lx, ly,
                                               ui.persona)) {
                b[y * width + x] = feather_face::personality(ui.persona).ink;
              }
            }
          }
        }
      }
      if (ui.header) {
        drawHeader(b, ui, d, true);
        overlayBox(b, hardwareButton, "HARDWARE");
        overlayBox(b, softwareButton, "SOFTWARE");
      }
      return;
    }

    std::fill(b, b + width * height, uint16_t(0));
    if (ui.header) drawHeader(b, ui, d, false);

    text(b, 10, 54, ui.page == Page::Hardware ? "HARDWARE" : "SOFTWARE", cyan, 2);
    if (ui.page == Page::Hardware) {
      text(b, 10, 76, "ESP32-S3 / 240X280", ink, 2);
      text(b, 10, 94, d.touch ? "TOUCH: OK" : "TOUCH: READ ERR", d.touch ? ink : warn, 2);
      if (d.imuPresent) {
        std::snprintf(line, sizeof line, "IMU: QMI 0X%02X ID:%02X", d.imuAddr, d.imuWho);
        text(b, 10, 112, line, cyan, 2);
        std::snprintf(line, sizeof line, "SMP %lu %s ERR:%lu", (unsigned long)d.imuSamples,
                      d.imuFresh ? "OK" : "STALE", (unsigned long)d.imuErrors);
        text(b, 10, 130, line, d.imuFresh ? ink : warn, 2);
      } else {
        text(b, 10, 112, "IMU: UNAVAILABLE", warn, 2);
        std::snprintf(line, sizeof line, "IMU ERR:%lu RETRY 2S", (unsigned long)d.imuErrors);
        text(b, 10, 130, line, muted, 2);
      }
      if (d.imuFresh) {
        std::snprintf(line, sizeof line, "ACC %.2f %.2f %.2f", d.ax, d.ay, d.az);
        text(b, 10, 148, line, ink, 2);
      } else {
        text(b, 10, 148, "ACC -- -- --", muted, 2);
      }
      std::snprintf(line, sizeof line, "BAT ADC: %d MV", d.rawBatteryMv);
      text(b, 10, 166, line, muted, 2);
      std::snprintf(line, sizeof line, "I2C ERR:%lu FRM:%lu", (unsigned long)d.errors,
                    (unsigned long)d.frames);
      text(b, 10, 184, line, muted, 2);
      text(b, 10, 204, "SWIPE L/R: SWITCH APP", muted, 1);
      text(b, 10, 216, "USB: STATUS  PWR/BOOT/RST", muted, 1);
    } else {
      std::snprintf(line, sizeof line, "APP: %s - SWIPE L/R", appShortName(ui.app));
      text(b, 10, 74, line, muted, 1);
      std::snprintf(line, sizeof line, "FACE: %s", feather_face::personality(ui.persona).name);
      box(b, 10, 88, 220, 40, line);
      std::snprintf(line, sizeof line, "BRIGHTNESS: %u", ui.brightness);
      box(b, 10, 132, 220, 40, line);
      box(b, 10, 176, 220, 40, "PERFORMANCE");
      text(b, 10, 220, "RESTORE: HOLD TOP 0.8S", muted, 1);
    }
    box(b, panelBackButton.x, panelBackButton.y, panelBackButton.w, panelBackButton.h,
        "BACK TO PLAY");
  }
};

}  // namespace touch169
