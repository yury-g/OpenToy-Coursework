// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace rlcd_hw {

struct BatteryReading {
  bool present = false;
  uint32_t mv = 0;
  int pct = -1;
};

void batteryBegin();
BatteryReading batteryRead();

}  // namespace rlcd_hw
