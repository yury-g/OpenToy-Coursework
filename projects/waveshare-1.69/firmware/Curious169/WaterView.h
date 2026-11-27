// SPDX-License-Identifier: MIT
#pragma once
#include "BoardAxes.h"
#include "Metadata.h"
#include "NessoPixels.h"
#include "TinyFont3x5.h"
#include "src/fluid/Gravity.h"
#include "src/fluid/Motion.h"
#include "src/fluid/Particles.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace touch169 {

class WaterView {
 public:
  static constexpr int tankWidth = width;
  static constexpr int tankHeight = height;  // Full native 240x280 display.
  using Pool = gravity_water::Particles<tankWidth, tankHeight, 400>;

  void sampleImu(float ax, float ay, float az, float gxDps, float gyDps, float gzDps,
                 uint32_t nowMs) {
    float dt = 0.01f;
    if (hasImuTime_) {
      uint32_t elapsed = uint32_t(nowMs - lastImuMs_);
      if (elapsed == 0) return;
      dt = std::min(elapsed * 0.001f, 0.05f);
    }
    lastImuMs_ = nowMs;
    hasImuTime_ = true;

    const auto down = board_axes::screenDown(ax, ay);
    gravity_.sample(down.x, down.y, dt);
    const float spin = std::sqrt(gxDps * gxDps + gyDps * gyDps + gzDps * gzDps);
    motion_.sample(down.x, down.y, az, spin, dt);
  }

  void step(bool imuFresh, bool touchActive, int touchX, int touchY, uint32_t nowMs) {
    if (!hasStepTime_) {
      lastStepMs_ = nowMs;
      hasStepTime_ = true;
    }
    // Accumulate 3 units per ms against threshold 50 -> exact 60 Hz steps, capped at 3 steps.
    uint32_t dtMs = std::min<uint32_t>(uint32_t(nowMs - lastStepMs_), 50);
    lastStepMs_ = nowMs;
    accumulator_ += dtMs * 3;

    int gx = 0;
    int gy = 0;
    if (imuFresh) {
      if (motion_.sequence != appliedShake_) {
        appliedShake_ = motion_.sequence;
        pool_.burst(motion_.kickX, motion_.kickY, motion_.power);
      }
      const auto d = gravity_.down();
      const float s = gravity_.strength();
      gx = int(d.x * s * 256);
      gy = int(d.y * s * 256);
    }

    if (touchActive) {
      pool_.repel(touchX, touchY);
    }

    while (accumulator_ >= 50) {
      pool_.step(gx, gy, false, (imuFresh && motion_.moving) || touchActive);
      accumulator_ -= 50;
    }
  }

  void render(uint16_t* b) const {
    nesso_water::drawParticles(b, width, height, 0, pool_);
    // NessoPixels emits display byte-swapped RGB565 for M5GFX; convert to native RGB565
    // to match Adafruit_ST7789::drawRGBBitmap and opentoy_tiny_ui.
    for (int i = 0; i < width * height; ++i) {
      uint16_t c = b[i];
      b[i] = uint16_t((c >> 8) | (c << 8));
    }
  }

  const Pool& pool() const { return pool_; }
  const gravity_water::Gravity& gravity() const { return gravity_; }
  const gravity_water::Motion& motion() const { return motion_; }

 private:
  Pool pool_;
  gravity_water::Gravity gravity_;
  gravity_water::Motion motion_;
  bool hasImuTime_ = false;
  bool hasStepTime_ = false;
  uint32_t lastImuMs_ = 0;
  uint32_t lastStepMs_ = 0;
  uint32_t accumulator_ = 0;
  uint32_t appliedShake_ = 0;
};

}  // namespace touch169
