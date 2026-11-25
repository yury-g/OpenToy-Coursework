// SPDX-License-Identifier: MIT
#include "../shared/SoundWake.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace opentoy_sound;

namespace {

PcmBlockSummary makeToneBlock(float amplitude, size_t n = 256) {
  std::vector<int16_t> pcm(n);
  for (size_t i = 0; i < n; ++i) {
    const float s = amplitude * std::sin(2.0f * 3.14159265f * float(i) / 16.0f);
    const int32_t v = int32_t(std::round(s * 32767.0f));
    pcm[i] = int16_t(std::max(-32767, std::min(32767, v)));
  }
  return analyzePcm16Mono(pcm.data(), pcm.size());
}

}  // namespace

int main() {
  // 1. Stuck/constant I2S lines are rejected as invalid.
  {
    std::vector<int16_t> zeros(128, 0);
    assert(!analyzePcm16Mono(zeros.data(), zeros.size()).valid);
    std::vector<int16_t> rail(128, -1);
    assert(!analyzePcm16Mono(rail.data(), rail.size()).valid);
  }

  // 2. Off by default: loud sound produces zero alertness when Sensitivity::Off.
  {
    SoundWakeMonitor mon;
    const auto loud = makeToneBlock(0.35f);
    assert(loud.valid && !loud.clipped);
    auto expr = mon.update(loud, 100, 1, false);
    assert(expr.suppressed && !expr.awake && expr.alertness == 0.0f);
  }

  // 3. Debounce, wake from quiet idle, distinct Curious vs Dreamy timing, and gradual settle.
  {
    SoundWakeMonitor curious, dreamy;
    curious.setSensitivity(Sensitivity::Normal);
    dreamy.setSensitivity(Sensitivity::Normal);

    const auto quiet = makeToneBlock(0.006f);
    for (uint32_t t = 0; t < 200; t += 20) {
      curious.update(quiet, t, 1, false);
      dreamy.update(quiet, t, 2, false);
    }

    const auto clap = makeToneBlock(0.28f);
    // Single block does not pass 2-block debounce yet.
    auto c1 = curious.update(clap, 220, 1, false);
    auto d1 = dreamy.update(clap, 220, 2, false);
    assert(!c1.awake && !d1.awake);

    // Second consecutive block wakes both; Curious attacks faster than Dreamy.
    auto c2 = curious.update(clap, 240, 1, false);
    auto d2 = dreamy.update(clap, 240, 2, false);
    assert(c2.awake && d2.awake);
    assert(c2.alertness > d2.alertness);
    assert(c2.lid_open_boost > 0 && c2.gaze_stabilize_pct > 0);

    // Return to quiet: stays alert during hold, then settles smoothly (Dreamy lingers longer).
    ListeningExpression cq{}, dq{};
    for (uint32_t t = 260; t <= 1600; t += 40) {
      cq = curious.update(quiet, t, 1, false);
      dq = dreamy.update(quiet, t, 2, false);
    }
    assert(!cq.awake && !dq.awake);
    assert(dq.alertness >= cq.alertness);
  }

  // 4. Speaker self-trigger suppression: own chirp + ringdown cooldown never wakes microphone.
  {
    SoundWakeMonitor mon;
    mon.setSensitivity(Sensitivity::High);
    const auto quiet = makeToneBlock(0.005f);
    mon.update(quiet, 100, 3, false);

    const auto self_chirp = makeToneBlock(0.40f);
    auto during = mon.update(self_chirp, 120, 3, /*speaker_active=*/true);
    assert(during.suppressed && !during.awake);

    // Echo 150 ms after speaker stops (< 280 ms cooldown) is still suppressed!
    auto ringdown = mon.update(self_chirp, 270, 3, /*speaker_active=*/false);
    assert(ringdown.suppressed && !ringdown.awake);

    // External sound after cooldown expires wakes Cheeky with asymmetric perk.
    mon.update(self_chirp, 450, 3, false);
    auto woke = mon.update(self_chirp, 470, 3, false);
    assert(!woke.suppressed && woke.awake && woke.right_perk != woke.left_perk);
  }

  // 5. Priority rules: direct pokes suppress lid boost; Copy-Face camera keeps gaze ownership.
  {
    SoundWakeMonitor mon;
    mon.setSensitivity(Sensitivity::Normal);
    const auto voice = makeToneBlock(0.22f);
    mon.update(voice, 100, 1, false);
    auto poked = mon.update(voice, 120, 1, false, /*direct_poke_active=*/true, false);
    assert(poked.awake && poked.lid_open_boost == 0 && poked.gaze_stabilize_pct == 0);

    auto cam = mon.update(voice, 140, 1, false, false, /*camera_tracking_active=*/true);
    assert(cam.awake && cam.lid_open_boost > 0 && cam.gaze_stabilize_pct == 0);
  }

  std::puts("PASS SoundWake adaptive noise floor, debounce, persona timing, echo suppression, and priority rules");
  return 0;
}
