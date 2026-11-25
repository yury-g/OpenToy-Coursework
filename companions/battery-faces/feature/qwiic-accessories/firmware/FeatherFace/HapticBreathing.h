// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

// Pure amplitude envelope, expressed on the DA7280's 0..127 command scale.
// The hardware owner must send update()'s output and stop the motor on expiry.
class HapticBreathing {
public:
  bool active = false;
  uint8_t peak = 0;
  uint32_t began = 0;
  uint32_t totalMilliseconds = 0;

  bool start(uint32_t now, uint8_t requestedPeak, uint32_t seconds) {
    if (active || requestedPeak < 4 || requestedPeak > 32 ||
        seconds < 10 || seconds > 60 || seconds % 10 != 0) {
      return false;
    }
    peak = requestedPeak;
    began = now;
    totalMilliseconds = seconds * 1000;
    active = true;
    return true;
  }

  uint8_t update(uint32_t now) {
    if (!active) return 0;
    if (elapsed(now) >= totalMilliseconds) {
      stop();
      return 0;
    }
    const uint32_t cycle = elapsed(now) % 10000;
    if (cycle < 4000) return ramp(cycle);
    if (cycle < 8000) return ramp(8000 - cycle);
    return 0;
  }

  void stop() { active = false; }

  uint32_t elapsed(uint32_t now) const {
    const uint32_t passed = uint32_t(now - began);
    return passed < totalMilliseconds ? passed : totalMilliseconds;
  }

  uint32_t remaining(uint32_t now) const {
    return active ? totalMilliseconds - elapsed(now) : 0;
  }

  const char *phase(uint32_t now) const {
    if (!active || elapsed(now) >= totalMilliseconds) return "off";
    const uint32_t cycle = elapsed(now) % 10000;
    if (cycle < 4000) return "inhale";
    if (cycle < 8000) return "exhale";
    return "rest";
  }

private:
  uint8_t ramp(uint32_t position) const {
    // peak * (3*t*t - 2*t*t*t), t=position/4000. Integer arithmetic
    // gives deterministic nearest-integer commands and zero endpoint slopes.
    const uint64_t numerator = uint64_t(peak) * position * position *
                               (12000 - 2 * position);
    return uint8_t((numerator + 32000000000ULL) / 64000000000ULL);
  }
};
