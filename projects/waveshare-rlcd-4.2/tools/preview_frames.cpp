// SPDX-License-Identifier: MIT
#include "../firmware/RlcdEyes/NativeEyes.h"
#include <cstdio>
#include <string>
void save(const std::string& path,const uint8_t* b){
 FILE* f=std::fopen(path.c_str(),"wb");if(!f)std::exit(1);std::fprintf(f,"P5\n400 300\n255\n");
 for(int y=0;y<300;y++)for(int x=0;x<400;x++)std::fputc((b[(x/2)*75+(299-y)/4]&(1U<<(7-((299-y)%4*2+x%2))))?255:0,f);
 std::fclose(f);
}
int main(int argc,char** argv){
 if(argc!=2)return 2;std::string dir=argv[1];uint8_t mono[1024],frame[15000];
 feather_face::CompanionPose pose;pose.personality=1;
 for(int mood:{100,0,-100}){
  pose.mood[0]=pose.mood[1]=mood;
  feather_face::drawCompanion(pose,0,mono);rlcd_eyes::compose(mono,frame,false);save(dir+"/old-"+std::to_string(mood)+".pgm",frame);
  rlcd_eyes::renderNative(pose,0,frame,false);save(dir+"/new-"+std::to_string(mood)+".pgm",frame);
 }
 feather_face::Companion companion{1};
 for(unsigned i=0;i<240;i++){
  uint32_t now=i*50;unsigned poke=now>=3000&&now<3300?1:now>=7000&&now<7300?2:0;
  auto live=companion.update(512,512,0,false,now,poke);
  rlcd_eyes::renderNative(live,now,frame,false);save(dir+"/frame-"+std::to_string(i)+".pgm",frame);
 }
}
