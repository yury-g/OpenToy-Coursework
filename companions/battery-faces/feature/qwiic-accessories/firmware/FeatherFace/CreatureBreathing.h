// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

// Continuous local amplitude envelope; hardware ownership and stopping remain
// with the caller. Seed once from creature identity, never from wall-clock time.
class CreatureBreathing {
public:
  bool active = false;
  uint8_t peak = 0;
  uint32_t began = 0;
  uint32_t inhaleMilliseconds = 0;
  uint32_t exhaleMilliseconds = 0;
  uint32_t restMilliseconds = 0;
  uint32_t cycleMilliseconds = 0;

  void start(uint32_t now, uint32_t seed) {
    randomState = seed ? seed : 0x6d2b79f5U;
    active = true;
    nextCycle(now);
  }

  void stop() { active = false; }

  uint8_t update(uint32_t now) {
    if (!active) return 0;
    const uint32_t elapsed = uint32_t(now - began);
    if (elapsed >= cycleMilliseconds) {
      // Missed cycles do not require catch-up or jump into a nonzero phase.
      // Begin one new cycle at this call, after the preceding quiet rest.
      nextCycle(now);
      return 0;
    }
    if (elapsed < inhaleMilliseconds) return ramp(elapsed, inhaleMilliseconds);
    const uint32_t exhaleEnd = inhaleMilliseconds + exhaleMilliseconds;
    if (elapsed < exhaleEnd) return ramp(exhaleEnd - elapsed, exhaleMilliseconds);
    return 0;
  }

  const char *phase(uint32_t now) const {
    if (!active) return "off";
    const uint32_t elapsed = uint32_t(now - began);
    if (elapsed < inhaleMilliseconds) return "inhale";
    if (elapsed < inhaleMilliseconds + exhaleMilliseconds) return "exhale";
    return "rest";
  }

private:
  uint32_t randomState = 1;

  uint32_t nextRandom() {
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return randomState;
  }

  void nextCycle(uint32_t now) {
    began = now;
    inhaleMilliseconds = 3500 + nextRandom() % 2501;
    exhaleMilliseconds = 4500 + nextRandom() % 2501;
    restMilliseconds = 1500 + nextRandom() % 1501;
    peak = uint8_t(64 + nextRandom() % 33);
    cycleMilliseconds = inhaleMilliseconds + exhaleMilliseconds + restMilliseconds;
  }

  uint8_t ramp(uint32_t position, uint32_t duration) const {
    // Rounded smoothstep peak*(3*t*t - 2*t*t*t), t=position/duration.
    const uint64_t denominator = uint64_t(duration) * duration * duration;
    const uint64_t numerator = uint64_t(peak) * position * position *
                               (3 * duration - 2 * position);
    return uint8_t((numerator + denominator / 2) / denominator);
  }
};
