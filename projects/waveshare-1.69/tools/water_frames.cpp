// SPDX-License-Identifier: MIT
// Renders firmware-exact 240x280 frames from Scene + WaterView + Interaction for layout inspection.
#include "Scene.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace touch169;

int main() {
  Interaction u;
  u.app = App::Water;
  u.page = Page::Play;
  u.header = true;

  Scene s;
  Diagnostics d;
  d.batteryMv = -1;
  d.imuPresent = true;
  d.imuFresh = true;

  std::vector<uint16_t> b(width * height);

  // Scripted 180 ticks @ 33 ms (~6 s) -> emit every 2nd tick (90 frames, 15 fps):
  // 0..31: upright settling with overlay visible (BAT --)
  // 32..89: tilt upward so beads flow directly under top 2-row overlay (BAT 4.18V)
  // 90..119: tilt down-right + touch repulsion at bottom gap/center
  // 120..159: Performance mode (overlays hidden, full 240x280 water, no physics reset)
  // 160..179: Face app full screen with overlay visible
  uint32_t now = 0;
  for (int tick = 0; tick < 180; ++tick) {
    now += 33;
    float ax = -1.0f, ay = 0.0f;
    bool touch = false;
    int tx = 0, ty = 0;

    if (tick < 32) {
      ax = -1.0f;
      ay = 0.0f;
      d.batteryMv = -1;
    } else if (tick < 90) {
      ax = 1.0f;
      ay = -0.25f;
      d.batteryMv = 4180;
      if (tick >= 50 && tick < 75) {
        touch = true;
        tx = 45 + (tick - 50) * 3;
        ty = 20;
      }
    } else if (tick < 120) {
      ax = -0.8f;
      ay = 0.55f;
      touch = true;
      tx = 120;
      ty = 250;
    } else if (tick < 160) {
      u.header = false;  // Performance mode on Water
      ax = -0.85f;
      ay = 0.35f;
    } else {
      u.header = true;
      u.app = App::Face;
      if (tick == 164) u.update({true, true, 55, 140}, now);
      if (tick == 166) {
        u.update({true, false, 55, 140}, now);
        s.input(u, now);
      }
    }

    s.water().sampleImu(ax, ay, 0.0f, 0.0f, 0.0f, 0.0f, now);
    if (u.app == App::Water) {
      s.water().step(true, touch, tx, ty, now);
    }
    if (tick % 2 == 0) {
      s.render(b.data(), u, d, now);
      std::fwrite(b.data(), 2, b.size(), stdout);
    }
  }
  return 0;
}
