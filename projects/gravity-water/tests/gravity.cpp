// SPDX-License-Identifier: MIT
#include <cassert>
#include <cmath>
#include <limits>
#include "../firmware/GravityWaterNesso/src/fluid/Gravity.h"
int main() {
 gravity_water::Gravity g;
 g.sample(1,0,.005f);assert(g.down().x>.15f);
 for(int i=0;i<10;i++)g.sample(1,0,.005f);
 assert(g.down().x>.98f);
 for(int i=0;i<1000;i++)g.sample(i%2?.01f:-.01f,.01f,.005f);
 assert(g.down().x>.98f && g.strength()<.02f);
 for(int a=0;a<360;a+=5) {
  float r=a*3.14159265f/180;
  for(int i=0;i<100;i++)g.sample(sinf(r),cosf(r),.005f);
  auto d=g.down();assert(d.x*sinf(r)+d.y*cosf(r)>.999f);
 }
 g.sample(std::numeric_limits<float>::quiet_NaN(),1,.005f);
 g.sample(0,1,20);assert(std::isfinite(g.down().x));
 for(int i=0;i<100;i++)g.sample(0,-1,.005f);
 assert(g.down().y<-.99f);
}
