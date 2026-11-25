// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstdio>

namespace copy_face {

inline void formatBatteryBadge(int mv, uint32_t ageMs, bool qualified, char (&out)[16]) {
  if (!qualified || mv < 2500 || mv > 4500 || ageMs > 5000u) {
    std::snprintf(out, sizeof(out), "BAT --");
    return;
  }
  std::snprintf(out, sizeof(out), "BAT %d.%02dV", mv / 1000, (mv % 1000) / 10);
}

}  // namespace copy_face
