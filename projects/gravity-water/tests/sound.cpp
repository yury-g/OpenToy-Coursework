// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstdlib>
#include "../firmware/GravityWaterNesso/src/fluid/PwmEnvelope.h"
#include "../firmware/GravityWaterNesso/src/fluid/TextureSound.h"
int main(){
 gravity_water::PwmEnvelope pwm;
 assert(pwm.step(0,false)==0);assert(pwm.step(0,true)==4);
 for(int i=0;i<31;i++)pwm.step(0,true);
 assert(pwm.step(0,true)==128);
 assert(pwm.step(14000,true)==237 && pwm.step(-14000,true)==19);
 int previous=128;for(int i=0;i<32;i++){int duty=pwm.step(0,false);assert(previous-duty<=4);previous=duty;}
 assert(previous==0);
 gravity_water::TextureSound s;int16_t samples[128];
 s.render(samples,128,0,0,false);for(auto v:samples)assert(v==0);
 long long energy=0;
 for(int n=0;n<500;n++){s.render(samples,128,255,255,true);for(auto v:samples){assert(std::abs(int(v))<=14000);energy+=std::abs(int(v));}}
 assert(energy>1000000);
 for(int n=0;n<8;n++)s.render(samples,128,0,0,false);
 assert(s.quiet());for(auto v:samples)assert(v==0);
}
