#pragma once

#include <cstdint>

class OrangeShade {
 public:
  static constexpr uint8_t steps = 9;
  static constexpr uint8_t standard = 4;

  // Bits 0–2 represent pressed D0–D2 buttons, after pin polarity is applied.
  void sample(uint32_t now, uint8_t pressed) {
    uint8_t newlyPressed = 0;
    for (uint8_t button = 0; button < 3; ++button) {
      const uint8_t bit = 1 << button;
      if ((pressed ^ raw_) & bit) changedAt_[button] = now;
      if (((pressed ^ stable_) & bit) && now - changedAt_[button] >= 30) {
        stable_ ^= bit;
        newlyPressed |= stable_ & bit;
      }
    }
    raw_ = pressed;
    if (newlyPressed & 0x01) {
      step_ = standard;
    } else {
      const uint8_t direction = newlyPressed & 0x06;
      if (direction == 0x02 && step_ > 0) --step_;
      if (direction == 0x04 && step_ + 1 < steps) ++step_;
    }
  }

  uint8_t step() const { return step_; }
  uint8_t screenGreen() const { return 84 + step_ * 20; }
  uint8_t pixelGreen(uint8_t red) const { return red * (10 + step_ * 5) / 100; }

 private:
  uint32_t changedAt_[3] = {};
  uint8_t raw_ = 0;
  uint8_t stable_ = 0;
  uint8_t step_ = standard;
};
