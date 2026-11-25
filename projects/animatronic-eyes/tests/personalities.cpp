#include "../firmware/AnimatronicEyes/Companion.h"
#include <cassert>
#include <cstdio>
#include <set>
using namespace feather_face;
uint32_t digest(const uint8_t* m){uint32_t h=2166136261U;for(int i=0;i<1024;i++)h=(h^m[i])*16777619U;return h;}
int main(){
 std::set<uint32_t> signatures;unsigned looks[3]={};
 for(unsigned id=1;id<=3;id++){
  Companion c(id);CompanionPose p;std::set<uint32_t> frames;uint8_t mono[1024];
  for(uint32_t t=0;t<60000;t+=33){p=c.update(512,512,0,false,t);assert(p.personality==id);assert(!p.pokeMask);if(t>1000)assert(p.idle.active);drawCompanion(p,t,mono);frames.insert(digest(mono));}
  assert(frames.size()>80&&c.idle.blinks[0]>3&&c.idle.blinks[1]>3);looks[id-1]=c.idle.looks;
  p=c.update(512,512,0,false,60000,1);assert(p.pokeMask==1&&p.mood[0]==-100&&!p.idle.active);
  p=c.update(512,512,0,false,60300);assert(p.mood[0]<0); // Synthetic release must not fake controller loss.
  p=c.update(512,512,SELECT|Y,true,70000);assert(p.pokeMask==2&&p.eye[1].right==100);
  p=c.update(0,0,SELECT|Y,false,70033);assert(!p.pokeMask&&p.eye[1].right==0&&p.mood[1]==100);
  p=c.update(512,512,0,false,72000);assert(p.idle.active);
  CompanionPose neutral;neutral.personality=id;drawCompanion(neutral,0,mono);signatures.insert(digest(mono));
  for(unsigned eye=0;eye<2;eye++)for(int y=0;y<32;y++)for(int x=0;x<28;x++){
   FaceState s;int dx=x-14,dy=y-16;
   if(dx*dx+dy*dy<=64)assert(!companionPixel(s,0,false,eye,x,y,id));
  }
  for(int raw:{0,512,1023}){p=c.update(512,raw,SELECT,true,73000);drawCompanion(p,73000,mono);uint16_t out[240*135+2];out[0]=123;out[32401]=456;nesso_eyes::render(mono,out+1,companionTransform(p),nullptr,0);assert(out[0]==123&&out[32401]==456);for(int y=0;y<135;y++)for(int x=0;x<240;x++)if(out[1+y*240+x])assert(x>=2&&x<238&&y>=15&&y<133);}
 }
 assert(signatures.size()==3);assert(looks[0]>looks[2]&&looks[2]>looks[1]);
 Companion classic;auto p=classic.update(512,512,0,false,1000);assert(!p.idle.active&&p.mood[0]==0);
 puts("PASS three distinct looks and tempos, independent seeds, no-gamepad play, poke recovery, optional controller loss, unchanged 2x pupils and screen bounds");
}
