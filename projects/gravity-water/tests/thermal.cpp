// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "../firmware/GravityWaterNesso/src/fluid/Particles.h"
using Pool=gravity_water::Particles<135,227,400>;
int main(){
 Pool p;
 for(int i=0;i<600;i++)p.step(0,256);
 int x[400],y[400];for(int i=0;i<400;i++){x[i]=p.particle(i).x;y[i]=p.particle(i).y;}
 for(int i=0;i<120;i++)p.step(0,256);
 for(int i=0;i<400;i++)assert(x[i]==p.particle(i).x && y[i]==p.particle(i).y);
 assert(p.sleeping());
 p.step(0,256,true);assert(!p.sleeping() && p.heat()>0);
 for(int i=0;i<60;i++)p.step(0,256,true);
 assert(p.heat()==255 && p.meanEnergy()>180);
 for(int i=0;i<20;i++)p.step(0,256,false);
 assert(p.heat()==0);
 for(int i=0;i<120;i++)p.step(0,256);
 assert(p.sleeping() && p.meanEnergy()==0);
 auto old=p.particle(0);
 p.burst(400,-600,220);
 assert(!p.sleeping() && p.particle(0).energy>=220);
 assert(p.particle(0).x!=old.x || p.particle(0).y!=old.y);
 for(int i=0;i<120;i++)p.step(0,256);
 int px=p.particle(200).x/256,py=p.particle(200).y/256;
 old=p.particle(200);p.repel(px,py);
 assert(!p.sleeping());
 assert(p.particle(200).x!=old.x || p.particle(200).y!=old.y);
 for(int i=0;i<120;i++)p.step(0,256);
 p.step(256,0);assert(!p.sleeping());
 // Simultaneous sustained controls: heat, changing gravity, touch and bursts.
 for(int tick=0;tick<3600;tick++) {
  if(tick%3==0)p.burst((tick%1800)-900,900-(tick%1800),255);
  p.repel((tick*7)%135,(tick*11)%227);
  p.step(tick%120<60?256:-256,tick%180<90?256:-256,tick%120<90,true);
  for(int n=0;n<400;n++) {
   const auto &a=p.particle(n);
   assert(a.x>=Pool::radius && a.x<=135*256-Pool::radius);
   assert(a.y>=Pool::radius && a.y<=227*256-Pool::radius);
  }
 }
 puts("Thermal: quiet rest, heating, quick cooling, immediate shake/touch wake and tilt passed");
}
