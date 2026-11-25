// SPDX-License-Identifier: MIT
// Actual renderer, scripted time/input. Not a camera capture or speed test.
#include <cstdio>
#include <vector>
#include "../../animatronic-eyes/firmware/AnimatronicEyes/Companion.h"
#include "../firmware/main/Presentation.h"
#include "../firmware/main/Smooth.h"
int main(){
 feather_face::Companion model(1);crowpanel::Presentation view;
 std::vector<uint16_t> screen(1024*600),eye(224*256);
 std::vector<unsigned char> rgb(1024*600*3);
 for(unsigned frame=0;frame<120;frame++){
  uint32_t now=frame*100;unsigned poke=now>=2000&&now<2400?1:0;
  auto pose=model.update(512,512,0,false,now,poke);std::fill(screen.begin(),screen.end(),0);
  for(unsigned e=0;e<2;e++){
   auto shake=nesso_eyes::jitter(now,pose.mood[e],e);int scale=view.scale(),w=28*scale,h=32*scale;
   int left=view.left()+((e?78:22)+shake.x)*scale;
   int top=view.top()+(eyeWhiteY(pose.eye[e])+shake.y+feather_face::personality(1).eyeOffset[e])*scale;
   crowpanel::renderSmoothEye(pose,e,scale,feather_face::personality(1).ink,eye.data());
   for(int y=0;y<h;y++)for(int x=0;x<w;x++)if(left+x>=0&&left+x<1024&&top+y>=0&&top+y<600)screen[(top+y)*1024+left+x]=eye[y*w+x];
  }
  for(unsigned i=0;i<screen.size();i++){auto c=screen[i];rgb[3*i]=((c>>11)&31)*255/31;rgb[3*i+1]=((c>>5)&63)*255/63;rgb[3*i+2]=(c&31)*255/31;}
  if(fwrite(rgb.data(),1,rgb.size(),stdout)!=rgb.size())return 1;
 }
}
