// SPDX-License-Identifier: MIT
#include "../firmware/AnimatronicEyes/Companion.h"
#include <cstdio>
using namespace feather_face;
int main(){
 Companion actors[3]={Companion(1),Companion(2),Companion(3)};
 puts("PERSONALITIES1 480 50");
 for(uint32_t t=0;t<24000;t+=50){
  printf("%u\n",t);
  for(unsigned i=0;i<3;i++){
   auto pose=actors[i].update(512,512,0,false,t);uint8_t mono[1024],gray[240*135];uint16_t pixels[240*135];
   drawCompanion(pose,t,mono);nesso_eyes::render(mono,pixels,companionTransform(pose),nullptr,0);
   for(unsigned p=0;p<sizeof gray;p++)gray[p]=pixels[p]?255:0;
   printf("%u %u %u %u %u %u\n",i+1,personality(i+1).ink,actors[i].idle.blinks[0],actors[i].idle.blinks[1],actors[i].idle.looks,pose.idle.active);
   fwrite(gray,1,sizeof gray,stdout);
  }
 }
 return ferror(stdout)?1:0;
}
