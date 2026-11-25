// SPDX-License-Identifier: MIT
// Scripted inputs through the actual Nesso behavior and Feather renderers.
#include "../firmware/NessoEyes/Idle.h"
#include "../firmware/NessoEyes/Presentation.h"
#include "../firmware/AnimatronicEyes/Companion.h"
#include "../shared/opentoy/DoodlePixels.h"
#include <cstdio>
#include <cstring>
using namespace nesso_eyes;
int main(){
 Reaction reaction;Idle idle;Orientation orientation;feather_face::Companion v1;feather_face::Controls v2(2);DoodleAnimator doodle;
 puts("FAMILY2 480 50");
 for(uint32_t t=0;t<24000;t+=50){
  bool left=t>=6000&&t<6450,right=t>=14500&&t<14950;bool down=left||right;
  auto pose=reaction.update(t,down,left?1:right?2:0);auto rest=idle.update(t,down);
  float target=t<9000?0:t<13500?(t-9000)*tau/4500:tau;
  orientation.sample(t,-std::sin(target),-std::cos(target),0,80);orientation.advance(t);Transform tr(orientation.angle());
  uint8_t mono[1024]={};uint16_t rendered[240*135];uint8_t nesso[240*135];
  for(unsigned eye=0;eye<2;eye++){
   auto s=rest.apply(pose.face,eye);auto shake=jitter(t,pose.eyeMood[eye],eye);
   for(int y=0;y<32;y++)for(int x=0;x<28;x++)if(largePupilPixel(s,pose.eyeMood[eye],eye,x,y)){int px=(eye?78:22)+x+shake.x,py=16+y+shake.y;mono[(py/8)*128+px]|=1<<(py%8);}
  }
  render(mono,rendered,tr,nullptr,0);for(unsigned i=0;i<sizeof nesso;i++)nesso[i]=rendered[i]?255:0;
  int x=512+int(380*std::sin(t*.0013)),y=512+int(300*std::sin(t*.001));uint32_t buttons=0;
  unsigned phase=(t/3000)%8;
  if(phase==1)buttons=feather_face::A;else if(phase==2)buttons=feather_face::B;
  else if(phase==3)buttons=feather_face::X;else if(phase==4)buttons=feather_face::START;
  else if(phase==5)buttons=feather_face::SELECT;else if(phase==6)buttons=feather_face::SELECT|feather_face::START;else buttons=feather_face::Y;
  bool manual=t>=9000&&t<13500;
  uint32_t pokeButtons=left?feather_face::SELECT|feather_face::X:right?feather_face::SELECT|feather_face::Y:0;
  auto a=v1.update(manual?x:512,manual?y:512,pokeButtons,true,t);
  auto b=v2.update(x,y,buttons,true);uint8_t featherMono[1024],feather1[240*135],feather2[1024];
  feather_face::drawCompanion(a,t,featherMono);render(featherMono,rendered,feather_face::companionTransform(a),nullptr,0);
  for(unsigned i=0;i<sizeof feather1;i++)feather1[i]=rendered[i]?255:0;
  doodle.tick(b,t,true);drawDoodle(doodle.current,b.expression,feather2,doodle.wiggle(0,t),doodle.wiggle(1,t),doodle.wiggle(2,t));
  auto touch=tr.screen(left?-28:28,0);
  printf("%u %d %.3f %d %d %u %u %u %d %d\n",t,left?1:right?2:0,orientation.angle(),pose.eyeMood[0],pose.eyeMood[1],rest.lid[0],rest.lid[1],phase,int(touch.x),int(touch.y));
  fwrite(nesso,1,sizeof nesso,stdout);fwrite(feather1,1,sizeof feather1,stdout);fwrite(feather2,1,1024,stdout);
 }
 return ferror(stdout)?1:0;
}
