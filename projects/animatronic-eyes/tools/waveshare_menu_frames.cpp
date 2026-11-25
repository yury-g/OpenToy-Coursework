// SPDX-License-Identifier: MIT
// Source-only menu previews; the battery shown is unavailable, not estimated.
#include "../firmware/WaveshareEyes/MenuView.h"
#include <cstdio>
int main(){
 using namespace waveshare_eyes;
 feather_face::Companion c(1);uint8_t mono[1024];auto pose=c.update(512,512,0,false,1600,0);feather_face::drawCompanion(pose,1600,mono);
 for(Page page:{Page::Play,Page::Root,Page::Sound,Page::Performance}){
  Menu m;m.page=page;
  for(int y=0;y<360;y++){
   uint16_t row[360]={};if(m.playing())renderRow(mono,y,row,0xffff);
   renderStatusRow(y,row,true,-1,1,false);renderMenuRow(m,y,row,-1);
   for(auto p:row){uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};fwrite(rgb,1,3,stdout);}
  }
 }
}
