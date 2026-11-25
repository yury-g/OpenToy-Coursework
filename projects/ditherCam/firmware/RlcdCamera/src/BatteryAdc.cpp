// SPDX-License-Identifier: MIT
// Battery ADC on Waveshare ESP32-S3-RLCD-4.2: GPIO4 (ADC1_CH3) with 3x divider.
// Honest absent-battery guard: never report fabricated % when battery is absent/floating.
#include "BatteryAdc.h"
#include <Arduino.h>
#include <algorithm>

namespace rlcd_hw {

namespace {
constexpr int kPinBatAdc = 4;
}

void batteryBegin() {
  pinMode(kPinBatAdc, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(kPinBatAdc, ADC_11db);
}

BatteryReading batteryRead() {
  BatteryReading r{};
  uint32_t sum = 0;
  uint32_t minMv = 0xFFFFFFFFu;
  uint32_t maxMv = 0;
  constexpr int kSamples = 8;

  for (int i = 0; i < kSamples; ++i) {
    const uint32_t mv = static_cast<uint32_t>(analogReadMilliVolts(kPinBatAdc)) * 3u;
    sum += mv;
    minMv = std::min(minMv, mv);
    maxMv = std::max(maxMv, mv);
  }
  r.mv = sum / kSamples;
  const uint32_t spread = maxMv - minMv;

  // Valid 18650 cell range with stable reading (reject open/floating/USB-rail noise)
  if (r.mv >= 2750u && r.mv <= 4230u && spread <= 120u) {
    r.present = true;
    if (r.mv <= 3000u) {
      r.pct = 0;
    } else if (r.mv >= 4120u) {
      r.pct = 100;
    } else {
      r.pct = static_cast<int>(((r.mv - 3000u) * 100u) / 1120u);
    }
  } else {
    r.present = false;
    r.pct = -1;
  }
  return r;
}

}  // namespace rlcd_hw
