// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace touch169 {

constexpr const char* version = "0.2.2-dev";
constexpr const char* created = "2026-11-27";
constexpr const char* board = "ESP32-S3-Touch-LCD-1.69";
constexpr int width = 240;
constexpr int height = 280;

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

inline constexpr bool contains(Rect r, int px, int py) {
  return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

// Compact overlaid controls in Play mode (not a full-width reserved footer).
constexpr Rect hardwareButton{10, 236, 106, 38};
constexpr Rect softwareButton{124, 236, 106, 38};
// Panel Back button on Hardware/Software pages.
constexpr Rect panelBackButton{10, 236, 220, 38};
// Top-center safe-edge restore zone while in Performance mode.
constexpr Rect restoreZone{48, 0, 144, 32};

// Ordered installed apps; horizontal swipes cycle through this list.
enum class App : uint8_t {
  Face = 0,
  Water = 1,
};

constexpr unsigned appCount = 2;

inline const char* appShortName(App app) {
  switch (app) {
    case App::Face: return "EYES1";
    case App::Water: return "WATER";
  }
  return "EYES1";
}

}  // namespace touch169
