#include "../firmware/NessoEyes/Idle.h"
#include <cassert>
#include <cstdio>
#include <set>
using namespace nesso_eyes;
int main(){
 Idle idle;std::set<int> looks[2];unsigned closed[2]={},different=0;IdlePose previous;
 for(uint32_t t=0;t<60000;t+=20){auto p=idle.update(t,false);if(t<900)assert(!p.active);
  for(unsigned i=0;i<2;i++){
   assert(p.lid[i]<=100&&p.x[i]>=-70&&p.x[i]<=70&&p.y[i]>=0&&p.y[i]<=60);
   if(p.active){looks[i].insert(p.x[i]*100+p.y[i]);if(p.lid[i]==100)closed[i]++;if(previous.active){assert(std::abs(p.x[i]-previous.x[i])<=16);assert(std::abs(p.y[i]-previous.y[i])<=12);}}
  }
  if(p.lid[0]!=p.lid[1])different++;
  previous=p;
 }
 assert(closed[0]>10&&closed[1]>10&&different>30&&looks[0].size()>100&&looks[1].size()>100);
 // Any touch interrupts blinking and gaze, even when it misses both eyes.
 for(uint32_t t=60000;t<62000;t+=20){auto p=idle.update(t,true);assert(!p.active&&p.lid[0]==0&&p.lid[1]==0);}
 assert(!idle.update(62500,false).active);assert(idle.update(63000,false).active);
 // The compositing helper preserves reaction closure and both independent gazes.
 IdlePose p;p.active=true;p.x[0]=-40;p.x[1]=50;p.y[0]=20;p.y[1]=30;p.lid[0]=30;p.lid[1]=80;
 FaceState reaction;reaction.left=100;auto l=p.apply(reaction,0),r=p.apply(reaction,1);
 assert(l.left==100&&l.pupilX==-40&&r.right==80&&r.pupilX==50);
 Idle wrap;wrap.update(0xffffff00,false);assert(wrap.update(1000,false).active);
 for(uint32_t t=1000;t<10000;t+=20){auto p=wrap.update(t,false);assert(p.lid[0]<=100&&p.lid[1]<=100);}
 puts("PASS autonomous gaze and full blinks, independent timing, bounded smooth motion, touch priority, restart delay, compositor and clock wrap");
}
