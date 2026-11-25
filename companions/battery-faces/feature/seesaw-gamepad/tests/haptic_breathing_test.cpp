#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../firmware/FeatherFace/HapticBreathing.h"

static void assertPhase(const HapticBreathing &breathing, uint32_t now,
                        const char *expected) {
  assert(std::strcmp(breathing.phase(now), expected) == 0);
}

static void rejectsInvalidSettingsWithoutStarting() {
  HapticBreathing breathing;
  assert(!breathing.active);
  assert(breathing.update(1000) == 0);
  assert(breathing.elapsed(1000) == 0);
  assert(breathing.remaining(1000) == 0);
  assertPhase(breathing, 1000, "off");
  for (uint8_t peak : {uint8_t(0), uint8_t(3), uint8_t(33), uint8_t(127), uint8_t(255)}) {
    assert(!breathing.start(1000, peak, 10));
    assert(!breathing.active);
  }
  for (uint32_t seconds : {0u, 9u, 11u, 15u, 59u, 61u, 70u, UINT32_MAX}) {
    assert(!breathing.start(1000, 16, seconds));
    assert(!breathing.active);
  }
}

static void followsSmoothInhaleExhaleAndRest() {
  HapticBreathing breathing;
  assert(breathing.start(100, 32, 20));
  struct Sample { uint32_t offset; uint8_t amplitude; const char *phase; };
  const Sample samples[] = {
    {0, 0, "inhale"}, {40, 0, "inhale"}, {1000, 5, "inhale"},
    {2000, 16, "inhale"}, {3000, 27, "inhale"}, {3960, 32, "inhale"},
    {4000, 32, "exhale"}, {4040, 32, "exhale"}, {5000, 27, "exhale"},
    {6000, 16, "exhale"}, {7000, 5, "exhale"}, {7960, 0, "exhale"},
    {8000, 0, "rest"}, {9000, 0, "rest"}, {9999, 0, "rest"},
    {10000, 0, "inhale"}, {14000, 32, "exhale"}, {18000, 0, "rest"}
  };
  for (const Sample &sample : samples) {
    assert(breathing.update(100 + sample.offset) == sample.amplitude);
    assertPhase(breathing, 100 + sample.offset, sample.phase);
  }
}

static void staysBoundedAndMonotonicAtEverySupportedPeak() {
  for (uint8_t peak = 4; peak <= 32; ++peak) {
    HapticBreathing breathing;
    assert(breathing.start(0, peak, 10));
    uint8_t previous = 0;
    for (uint32_t now = 0; now <= 4000; ++now) {
      const uint8_t amplitude = breathing.update(now);
      assert(amplitude >= previous && amplitude <= peak);
      previous = amplitude;
    }
    assert(previous == peak);
    for (uint32_t now = 4001; now < 8000; ++now) {
      const uint8_t amplitude = breathing.update(now);
      assert(amplitude <= previous && amplitude <= peak);
      previous = amplitude;
    }
    for (uint32_t now = 8000; now < 10000; ++now) {
      assert(breathing.update(now) == 0);
    }
  }
}

static void hasGentleQuantizedEndpointsAtTheUpdateCadence() {
  HapticBreathing breathing;
  assert(breathing.start(0, 32, 10));
  assert(breathing.update(0) == breathing.update(40));
  assert(breathing.update(3960) == breathing.update(4000));
  assert(breathing.update(4000) == breathing.update(4040));
  assert(breathing.update(7960) == breathing.update(8000));
  uint8_t previous = 0;
  for (uint32_t now = 0; now < 10000; now += 40) {
    const uint8_t amplitude = breathing.update(now);
    const int step = int(amplitude) - int(previous);
    assert(step >= -1 && step <= 1);
    previous = amplitude;
  }
}

static void expiresExactlyAndCannotBeExtendedOrAutoplay() {
  for (uint32_t seconds = 10; seconds <= 60; seconds += 10) {
    HapticBreathing breathing;
    assert(breathing.start(123, 16, seconds));
    assert(breathing.totalMilliseconds == seconds * 1000);
    assert(breathing.elapsed(123) == 0);
    assert(breathing.remaining(123) == seconds * 1000);
    assert(!breathing.start(4123, 32, 60));
    assert(breathing.began == 123 && breathing.peak == 16);
    assert(breathing.totalMilliseconds == seconds * 1000);
    const uint32_t end = 123 + seconds * 1000;
    assert(breathing.update(end - 1) == 0);
    assert(breathing.active);
    assert(breathing.remaining(end - 1) == 1);
    assert(breathing.elapsed(end) == seconds * 1000);
    assert(breathing.remaining(end) == 0);
    assertPhase(breathing, end, "off");
    assert(!breathing.start(end, 32, 60));
    assert(breathing.update(end) == 0);
    assert(!breathing.active);
    assert(breathing.update(end + 4000) == 0);
    assert(breathing.update(end + 100000) == 0);
    assert(breathing.elapsed(end + 100000) == seconds * 1000);
    assert(breathing.remaining(end + 100000) == 0);
    assertPhase(breathing, end + 4000, "off");
  }
}

static void explicitStopIsImmediateAndRestartRequiresANewStart() {
  HapticBreathing breathing;
  assert(breathing.start(100, 32, 60));
  assert(breathing.update(4100) == 32);
  breathing.stop();
  assert(!breathing.active);
  assert(breathing.update(4100) == 0);
  assert(breathing.remaining(4100) == 0);
  assertPhase(breathing, 4100, "off");
  breathing.stop();
  assert(breathing.update(14100) == 0);
  assert(breathing.start(14100, 4, 10));
  assert(breathing.update(18100) == 4);
  assert(breathing.update(24100) == 0);
  assert(!breathing.active);
}

static void handlesMillisWrapForEnvelopeAndDeadline() {
  HapticBreathing breathing;
  const uint32_t began = UINT32_MAX - 1999;
  assert(breathing.start(began, 32, 10));
  assert(breathing.update(began + 2000) == 16);
  assert(breathing.elapsed(began + 2000) == 2000);
  assert(breathing.remaining(began + 2000) == 8000);
  assert(breathing.update(began + 4000) == 32);
  assertPhase(breathing, began + 4000, "exhale");
  assert(breathing.update(began + 8000) == 0);
  assertPhase(breathing, began + 8000, "rest");
  assert(breathing.update(began + 9999) == 0 && breathing.active);
  assert(breathing.update(began + 10000) == 0 && !breathing.active);
  assert(breathing.elapsed(began + 12000) == 10000);
  assert(breathing.remaining(began + 12000) == 0);
}

int main() {
  rejectsInvalidSettingsWithoutStarting();
  followsSmoothInhaleExhaleAndRest();
  staysBoundedAndMonotonicAtEverySupportedPeak();
  hasGentleQuantizedEndpointsAtTheUpdateCadence();
  expiresExactlyAndCannotBeExtendedOrAutoplay();
  explicitStopIsImmediateAndRestartRequiresANewStart();
  handlesMillisWrapForEnvelopeAndDeadline();
  puts("haptic breathing: smooth 4/4/2 envelope, bounded peak/duration, stop and wrap PASS");
}
