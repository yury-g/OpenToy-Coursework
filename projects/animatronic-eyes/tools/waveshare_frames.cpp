// SPDX-License-Identifier: MIT
#include "../firmware/WaveshareEyes/Round.h"
#include "../firmware/WaveshareEyes/MenuView.h"
#include <cstdio>
int main(){
 using namespace waveshare_eyes;Menu menu;
 for(unsigned id=1;id<=3;id++){
  feather_face::Companion c(id);uint8_t mono[1024];uint16_t row[360];
  for(uint32_t now=0;now<9000;now+=20){
   unsigned poke=now>=2200&&now<2500?1:now>=4700&&now<5000?2:0;
   auto pose=c.update(512,512,0,false,now,poke);
   if(now%100)continue;
   feather_face::drawCompanion(pose,now,mono);
   for(int y=0;y<360;y++){
    renderRow(mono,y,row,feather_face::personality(id).ink);renderStatusRow(y,row,true,4100,id,false);renderMenuRow(menu,y,row,4100);
    for(auto p:row){uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};fwrite(rgb,1,3,stdout);}
   }
  }
 }
}
