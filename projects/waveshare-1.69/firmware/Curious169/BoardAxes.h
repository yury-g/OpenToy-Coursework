// SPDX-License-Identifier: MIT
#pragma once
#include "src/fluid/Gravity.h"

namespace touch169 {
namespace board_axes {

// Waveshare ESP32-S3-Touch-LCD-1.69, portrait rotation 0 (240x280).
// 0.2.0 used {-ay, ax}, inferred from vendor silkscreen/model orientation.
// On 2026-11-27 the owner reported water moving backwards/upside down.
// 0.2.1 reverses both screen gravity axes in response: {ay, -ax}.
// This corrects motion only; display and touch coordinates stay unchanged.
// The four-direction physical recheck remains pending after installation.
inline gravity_water::Point screenDown(float ax, float ay) {
  return {ay, -ax};
}

}  // namespace board_axes
}  // namespace touch169
