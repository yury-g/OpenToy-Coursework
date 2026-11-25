// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <cstdio>

namespace opentoy {
namespace tab5 {

// M5Stack Tab5 power architecture:
// - C145 ships without a battery; K145 ships with a 2S NP-F550 pack (7.4 V nominal, ~8.4 V full).
// - INA226 (0x41 on I2C0) measures bus voltage and shunt current, NOT calibrated fuel-gauge SOC%.
// - Per OpenToy APP-STATUS-BAR.md and FULL-FAMILY-TEST-CHECKLIST.md:
//   * Never render an empty battery icon for unknown or voltage-only readings.
//   * Never fabricate a percentage from voltage.
//   * Report "BAT --" unless a fresh, coherent 2S rail measurement or verified gauge is qualified.
enum class BatteryReadingKind : uint8_t {
  Unavailable = 0,
  MeasuredMillivolts2S = 1,
  VerifiedGaugePercent = 2,
};

struct BatterySample {
  BatteryReadingKind kind = BatteryReadingKind::Unavailable;
  int32_t value = 0;       // millivolts for MeasuredMillivolts2S, 0..100 for VerifiedGaugePercent
  uint32_t sampled_ms = 0;
  bool valid = false;
};

constexpr int32_t kMinValid2SMillivolts = 5500;
constexpr int32_t kMaxValid2SMillivolts = 8650;
constexpr uint32_t kMaxBatterySampleAgeMs = 5000;

inline bool formatBatteryStatus(const BatterySample& sample,
                                uint32_t now_ms,
                                char* out,
                                size_t capacity) {
  if (!out || capacity < 8) return false;
  if (!sample.valid || sample.kind == BatteryReadingKind::Unavailable ||
      uint32_t(now_ms - sample.sampled_ms) > kMaxBatterySampleAgeMs) {
    std::snprintf(out, capacity, "BAT --");
    return false;
  }
  if (sample.kind == BatteryReadingKind::MeasuredMillivolts2S) {
    if (sample.value < kMinValid2SMillivolts || sample.value > kMaxValid2SMillivolts) {
      std::snprintf(out, capacity, "BAT --");
      return false;
    }
    const int whole = int(sample.value / 1000);
    const int hundredths = int((sample.value % 1000 + 5) / 10);
    if (hundredths >= 100) {
      std::snprintf(out, capacity, "BAT %d.00V", whole + 1);
    } else {
      std::snprintf(out, capacity, "BAT %d.%02dV", whole, hundredths);
    }
    return true;
  }
  if (sample.kind == BatteryReadingKind::VerifiedGaugePercent) {
    if (sample.value < 0 || sample.value > 100) {
      std::snprintf(out, capacity, "BAT --");
      return false;
    }
    std::snprintf(out, capacity, "BAT %d%%", int(sample.value));
    return true;
  }
  std::snprintf(out, capacity, "BAT --");
  return false;
}

}  // namespace tab5
}  // namespace opentoy
