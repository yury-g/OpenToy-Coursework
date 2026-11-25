// SPDX-License-Identifier: MIT
#include "../firmware/NessoFace/Controls.h"
#include <cassert>

int main() {
  assert(touchZone(0, 240) == 0x001);
  assert(touchZone(79, 240) == 0x001);
  assert(touchZone(80, 240) == 0x010);
  assert(touchZone(160, 240) == 0x100);
  assert(touchZone(239, 240) == 0x100);
  assert(touchZone(-1, 240) == 0);
  assert(touchZone(240, 240) == 0);
  assert(touchZone(0, 0) == 0);
  LiveFace control;
  assert(control.apply("doodle 20 -30 50 15 82 99 0 happy") == ACCEPTED);
  assert(control.remote && control.pose.left == 99 && control.pose.eyeBend == 20);
  assert(control.apply("doodle -100 0 0 0 101 0 0 happy") == INVALID);
  assert(control.pose.left == 99 && control.pose.eyeBend == 20);
  assert(control.apply("touch off") == ACCEPTED && !control.touchEnabled);
  assert(control.apply("status") == STATUS_QUERY && control.remote);
  assert(control.apply("local") == ACCEPTED && !control.remote && !control.touchEnabled);
  assert(control.apply("reset") == ACCEPTED && control.touchEnabled && !control.remote);
}
