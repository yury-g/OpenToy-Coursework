#include "../firmware/AnimatronicEyes/Companion.h"
#include <cassert>
#include <cstdio>
using namespace feather_face;
int main(){
 Companion c;auto p=c.update(512,512,0,true,0);assert(p.mood[0]==100&&!p.idle.active);
 for(unsigned t=20;t<=4000;t+=20)p=c.update(512,512,0,true,t);
 assert(p.idle.active&&c.idle.blinks[0]>0&&c.idle.blinks[1]>0);
 p=c.update(100,512,0,true,4020);assert(!p.idle.active&&p.eye[0].pupilX>0);
 p=c.update(512,512,SELECT|X,true,5000);assert(p.pokeMask==1&&p.mood[0]==-100&&p.mood[1]>-100&&p.eye[0].left==100&&!p.idle.active&&!p.originalShape);
 p=c.update(512,512,0,true,9000);assert(p.mood[0]<100&&p.mood[1]==100&&p.idle.active);
 p=c.update(512,512,SELECT|Y,true,10000);assert(p.pokeMask==2&&p.mood[1]==-100&&p.eye[1].right==100);
 p=c.update(512,512,B,true,11000);assert(p.pokeMask==3&&p.mood[0]==-100&&p.mood[1]==-100);
 p=c.update(512,512,0,true,20000);assert(p.mood[0]==100&&p.mood[1]==100);
 p=c.update(512,512,X,true,21000);assert(p.originalShape&&p.eye[0].expression==SURPRISED);
 p=c.update(512,512,Y,true,22000);assert(!p.originalShape&&p.mood[0]==0);
 p=c.update(512,512,A,true,23000);assert(p.mood[0]==100);
 p=c.update(512,512,SELECT|START,true,24000);assert(p.eye[0].left==100&&p.eye[1].right==100&&!p.pokeMask);
 p=c.update(0,512,START,true,25000);assert(p.eye[1].right==100);
 // Preserve controller-loss neutral and fresh reconnect; no queued anger/idle.
 p=c.update(0,0,SELECT|X,false,26000);assert(!p.idle.active&&!p.pokeMask&&p.mood[0]==0&&p.eye[0].left==0&&p.eye[0].pupilX==0);
 p=c.update(512,512,0,true,27000);assert(p.mood[0]==100&&!p.idle.active);
 // Max-size eyes and full old white-travel controls remain within display.
 for(int raw:{0,512,1023}){
  p=c.update(512,raw,SELECT,true,28000);auto tr=companionTransform(p);int shift=eyeWhiteY(p.eye[0])-16;
  for(float x:{-45.f,45.f})for(float y:{float(shift-18),float(shift+18)}){auto q=tr.screen(x,y);assert(q.x>=2&&q.x<=238&&q.y>=15&&q.y<=133);}
 }
 puts("PASS Feather companion idle, direct controls, targeted pokes, anger recovery, presets, blink/wink layers, disconnect neutral, reconnect and fitted eye travel");
}
