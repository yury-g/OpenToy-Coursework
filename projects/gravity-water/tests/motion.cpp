// SPDX-License-Identifier: MIT
#include <cassert>
#include "../firmware/GravityWaterNesso/src/fluid/Motion.h"
int main(){
 gravity_water::Motion m;m.sample(0,1,0,0,.005f);
 for(int i=0;i<400;i++)m.sample(i%2?.01f:-.01f,1,0,1,.005f);
 assert(!m.moving && m.sequence==0);
 m.sample(1.2f,1,0,0,.005f);
 assert(m.moving && m.sequence==1 && m.power>100 && m.kickX>0);
 m.sample(-1.2f,1,0,0,.005f);assert(m.sequence==1); // bounded event rate
 for(int i=0;i<400;i++)m.sample(0,1,0,0,.005f);
 assert(!m.moving);auto n=m.sequence;
 m.sample(0,1,1.4f,0,.005f);assert(m.sequence==n+1 && m.power>100); // screen-normal shake
}
