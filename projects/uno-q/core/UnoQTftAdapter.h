// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace opentoy {
namespace uno_q {

enum class LogicLevel {
  Unknown,
  Lv3v3,
  ToleranceUnverified5V,
};

enum class ShieldBus {
  Unknown,
  Spi4Wire,
  Parallel8Bit8080,
};

struct ShieldDescriptor {
  std::string controller_ic;
  ShieldBus bus = ShieldBus::Unknown;
  LogicLevel logic_level = LogicLevel::Unknown;
  uint16_t width = 320;
  uint16_t height = 240;
  bool pin_map_verified = false;
  bool touch_qualified = false;
  bool battery_sensor_qualified = false;
};

inline bool isHardwareQualified(const ShieldDescriptor& d) {
  return !d.controller_ic.empty() && d.bus != ShieldBus::Unknown &&
         d.logic_level == LogicLevel::Lv3v3 && d.pin_map_verified &&
         d.width > 0 && d.height > 0;
}

class ITftDisplayDriver {
 public:
  virtual ~ITftDisplayDriver() = default;
  virtual bool isQualified() const = 0;
  virtual bool initPanel() = 0;
  virtual bool presentRgb565(const uint16_t* pixels, uint16_t w, uint16_t h) = 0;
  virtual bool supportsTouch() const = 0;
};

// Safe default adapter while unbranded 2.8-inch TFT shield hardware remains unqualified.
// Never drives UNO Q GPIOs, SPI, or 5V/3.3V pins.
class NullUnqualifiedPanel final : public ITftDisplayDriver {
 public:
  explicit NullUnqualifiedPanel(ShieldDescriptor desc = {}) : desc_(desc) {}

  bool isQualified() const override { return false; }
  bool initPanel() override { return false; }
  bool presentRgb565(const uint16_t*, uint16_t, uint16_t) override { return false; }
  bool supportsTouch() const override { return false; }
  const ShieldDescriptor& descriptor() const { return desc_; }

 private:
  ShieldDescriptor desc_;
};

inline void formatUnoQBattery(int mv, uint32_t ageMs, bool qualified, char (&out)[12]) {
  if (!qualified || mv < 2500 || mv > 4500 || ageMs > 5000u) {
    std::snprintf(out, sizeof(out), "BAT --");
    return;
  }
  std::snprintf(out, sizeof(out), "BAT %d.%02dV", mv / 1000, (mv % 1000) / 10);
}

}  // namespace uno_q
}  // namespace opentoy
