// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstdio>
#include "../firmware/GravityWaterNesso/src/fluid/Particles.h"
using Pool=gravity_water::Particles<135,240,400>;
static void check(const Pool& p) {
 assert(p.count()==400);
 for(int i=0;i<p.count();i++) {
  const auto& a=p.particle(i);
  assert(a.x>=Pool::radius && a.x<=135*256-Pool::radius);
  assert(a.y>=Pool::radius && a.y<=240*256-Pool::radius);
 }
}
static float centerX(const Pool& p) {long long x=0;for(int i=0;i<p.count();i++)x+=p.particle(i).x;return float(x)/p.count()/256;}
static float centerY(const Pool& p) {long long y=0;for(int i=0;i<p.count();i++)y+=p.particle(i).y;return float(y)/p.count()/256;}
int main() {
 Pool p; check(p);float start=centerX(p);
 for(int i=0;i<30;i++) p.step(256,0);
 assert(centerX(p)>start+3); // grains move soon after a quarter-turn
 for(int i=0;i<240;i++) p.step(256,0);
 check(p); assert(centerX(p)>95);
 for(int i=0;i<300;i++) p.step(-256,0);
 check(p); assert(centerX(p)<40);
 for(int i=0;i<300;i++) p.step(0,-256);
 check(p); assert(centerY(p)<65);
 for(int i=0;i<300;i++) p.step(0,256);
 check(p); assert(centerY(p)>170);
 for(int i=0;i<1200;i++) {p.step((i/100)%2?256:-256,(i/150)%2?256:-256);check(p);}
 puts("Particles: conserved count, bounds, response, four directions and turn stress passed");
}
