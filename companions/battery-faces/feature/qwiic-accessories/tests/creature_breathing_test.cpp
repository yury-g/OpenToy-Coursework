#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../firmware/FeatherFace/CreatureBreathing.h"

static void assertPhase(const CreatureBreathing &breathing, uint32_t now,
                        const char *expected) {
  assert(std::strcmp(breathing.phase(now), expected) == 0);
}

static void assertCycleBounds(const CreatureBreathing &breathing) {
  assert(breathing.inhaleMilliseconds >= 3500 && breathing.inhaleMilliseconds <= 6000);
  assert(breathing.exhaleMilliseconds >= 4500 && breathing.exhaleMilliseconds <= 7000);
  assert(breathing.restMilliseconds >= 1500 && breathing.restMilliseconds <= 3000);
  assert(breathing.peak >= 64 && breathing.peak <= 96);
  assert(breathing.cycleMilliseconds == breathing.inhaleMilliseconds +
         breathing.exhaleMilliseconds + breathing.restMilliseconds);
}

static void startsOnlyOnRequestAndStopsImmediately() {
  CreatureBreathing breathing;
  assert(!breathing.active && breathing.update(4000) == 0);
  assertPhase(breathing, 4000, "off");
  breathing.start(100, 123);
  assert(breathing.active && breathing.began == 100);
  assertCycleBounds(breathing);
  assert(breathing.update(100) == 0);
  assertPhase(breathing, 100, "inhale");
  assert(breathing.update(100 + breathing.inhaleMilliseconds) == breathing.peak);
  breathing.stop();
  assert(!breathing.active && breathing.update(60000) == 0);
  assertPhase(breathing, 60000, "off");
  breathing.stop();
  assert(breathing.update(UINT32_MAX) == 0 && !breathing.active);
}

static void followsSmoothMonotonicRampsAndSilentRest() {
  for (uint32_t seed = 1; seed <= 40; ++seed) {
    CreatureBreathing breathing;
    breathing.start(0, seed);
    assertCycleBounds(breathing);
    const uint32_t inhale = breathing.inhaleMilliseconds;
    const uint32_t exhaleEnd = inhale + breathing.exhaleMilliseconds;
    // Smoothstep at one quarter is 5/32; the descending quarter is 27/32.
    // The tolerance covers integer amplitude rounding and sub-ms placement.
    const int risingError = int(breathing.update(inhale / 4)) * 32 - 5 * breathing.peak;
    const int fallingError = int(breathing.update(inhale + breathing.exhaleMilliseconds / 4)) *
                             32 - 27 * breathing.peak;
    assert(risingError >= -17 && risingError <= 17);
    assert(fallingError >= -17 && fallingError <= 17);
    uint8_t previous = 0;
    for (uint32_t now = 0; now <= inhale; ++now) {
      const uint8_t amplitude = breathing.update(now);
      assert(amplitude >= previous && amplitude <= breathing.peak);
      previous = amplitude;
    }
    assert(previous == breathing.peak);
    assertPhase(breathing, inhale - 1, "inhale");
    assertPhase(breathing, inhale, "exhale");
    for (uint32_t now = inhale + 1; now <= exhaleEnd; ++now) {
      const uint8_t amplitude = breathing.update(now);
      assert(amplitude <= previous && amplitude <= breathing.peak);
      previous = amplitude;
    }
    assert(previous == 0);
    assertPhase(breathing, exhaleEnd, "rest");
    for (uint32_t now = exhaleEnd; now < breathing.cycleMilliseconds; ++now) {
      assert(breathing.update(now) == 0);
    }
  }
}

static void hasFlatEndpointsAndSmallStepsAt40msCadence() {
  CreatureBreathing breathing;
  breathing.start(0, 456);
  const uint32_t inhale = breathing.inhaleMilliseconds;
  const uint32_t endExhale = inhale + breathing.exhaleMilliseconds;
  assert(breathing.update(0) == 0 && breathing.update(40) == 0);
  assert(breathing.update(inhale - 40) == breathing.peak);
  assert(breathing.update(inhale) == breathing.peak);
  assert(breathing.update(inhale + 40) == breathing.peak);
  assert(breathing.update(endExhale - 40) == 0);
  assert(breathing.update(endExhale) == 0);
  uint8_t previous = 0;
  for (uint32_t now = 0; now < breathing.cycleMilliseconds; now += 40) {
    const uint8_t amplitude = breathing.update(now);
    const int step = int(amplitude) - int(previous);
    assert(step >= -2 && step <= 2);
    previous = amplitude;
  }
}

static void seedAndRestartReproduceTheSameCycleTrace() {
  CreatureBreathing first, second;
  first.start(0, 0x12345678);
  second.start(0, 0x12345678);
  for (uint32_t now = 0; now < 120000; now += 137) {
    assert(first.update(now) == second.update(now));
    assert(first.peak == second.peak);
    assert(first.inhaleMilliseconds == second.inhaleMilliseconds);
    assert(first.exhaleMilliseconds == second.exhaleMilliseconds);
    assert(first.restMilliseconds == second.restMilliseconds);
  }
  first.start(0, 0x12345678);
  second = CreatureBreathing{};
  second.start(0, 0x12345678);
  for (uint32_t now = 0; now < 120000; now += 137) {
    assert(first.update(now) == second.update(now));
    assert(first.cycleMilliseconds == second.cycleMilliseconds);
  }
  CreatureBreathing different;
  different.start(0, 0x87654321);
  first.start(0, 0x12345678);
  assert(first.inhaleMilliseconds != different.inhaleMilliseconds ||
         first.exhaleMilliseconds != different.exhaleMilliseconds ||
         first.restMilliseconds != different.restMilliseconds || first.peak != different.peak);
  bool differentOutput = false;
  for (uint32_t now = 0; now < 120000; now += 137) {
    if (first.update(now) != different.update(now)) differentOutput = true;
  }
  assert(differentOutput);
}

static void continuesWithVariedCyclesUntilExplicitStop() {
  CreatureBreathing breathing;
  breathing.start(0, 0);
  const uint32_t firstCycle = breathing.cycleMilliseconds;
  const uint8_t firstPeak = breathing.peak;
  bool varied = false;
  for (unsigned cycle = 0; cycle < 1000; ++cycle) {
    assertCycleBounds(breathing);
    const uint32_t began = breathing.began;
    const uint32_t end = began + breathing.cycleMilliseconds;
    assert(breathing.update(began + breathing.inhaleMilliseconds) == breathing.peak);
    assert(breathing.update(end - 1) == 0);
    assert(breathing.update(end) == 0 && breathing.active);
    assert(breathing.began == end);
    assertPhase(breathing, end, "inhale");
    if (breathing.cycleMilliseconds != firstCycle || breathing.peak != firstPeak) varied = true;
  }
  assert(varied);
}

static void missedCyclesRestartAtZeroWithoutCatchup() {
  CreatureBreathing breathing;
  breathing.start(100, 1);
  CreatureBreathing onTime;
  onTime.start(100, 1);
  onTime.update(100 + onTime.cycleMilliseconds);
  assert(breathing.update(100 + breathing.inhaleMilliseconds) == breathing.peak);
  const uint32_t muchLater = 100 + 24 * 60 * 60 * 1000u;
  assertPhase(breathing, muchLater, "rest");
  assert(breathing.update(muchLater) == 0);
  assert(breathing.active && breathing.began == muchLater);
  assert(breathing.peak == onTime.peak);
  assert(breathing.inhaleMilliseconds == onTime.inhaleMilliseconds);
  assert(breathing.exhaleMilliseconds == onTime.exhaleMilliseconds);
  assert(breathing.restMilliseconds == onTime.restMilliseconds);
  assert(breathing.update(muchLater + 40) == 0);
  assertCycleBounds(breathing);
}

static void handlesMillisWrapThroughPeakRestAndNextCycle() {
  CreatureBreathing breathing;
  const uint32_t began = UINT32_MAX - 1999;
  breathing.start(began, 99);
  const uint32_t inhale = breathing.inhaleMilliseconds;
  const uint32_t endExhale = inhale + breathing.exhaleMilliseconds;
  const uint32_t end = began + breathing.cycleMilliseconds;
  assert(breathing.update(began + inhale) == breathing.peak);
  assertPhase(breathing, began + inhale, "exhale");
  assert(breathing.update(began + endExhale) == 0);
  assertPhase(breathing, began + endExhale, "rest");
  assert(breathing.update(end) == 0 && breathing.active && breathing.began == end);
  assertPhase(breathing, end, "inhale");
}

int main() {
  startsOnlyOnRequestAndStopsImmediately();
  followsSmoothMonotonicRampsAndSilentRest();
  hasFlatEndpointsAndSmallStepsAt40msCadence();
  seedAndRestartReproduceTheSameCycleTrace();
  continuesWithVariedCyclesUntilExplicitStop();
  missedCyclesRestartAtZeroWithoutCatchup();
  handlesMillisWrapThroughPeakRestAndNextCycle();
  puts("creature breathing: continuous seeded slow envelopes, quiet rest, stop and wrap PASS");
}
