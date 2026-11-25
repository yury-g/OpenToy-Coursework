// SPDX-License-Identifier: MIT
// Export a deterministic animation through the actual pure firmware renderer.
#include "../projects/xiao-class/firmware/ToyBLE/DoodlePixels.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 std::ifstream input(argv[1]);std::vector<FaceState> poses;std::string line;
 while(std::getline(input,line)){
  FaceState s;if(line.rfind("doodle ",0)!=0||!parseDoodle(line.c_str()+7,s))return 3;poses.push_back(s);
 }
 if(!input.eof()||poses.empty())return 4;
 const unsigned frames=38,dt=50;uint32_t now=0;DoodleAnimator animation;animation.reset(restingDoodle(),now);
 printf("OPENTOYANIM1 %zu %u %u\n",poses.size(),frames,dt);
 for(unsigned i=0;i<poses.size();i++)for(unsigned j=0;j<frames;j++){
  animation.tick(poses[i],now,true);uint8_t buffer[1024];
  drawDoodle(animation.current,poses[i].expression,buffer,animation.wiggle(0,now),animation.wiggle(1,now),animation.wiggle(2,now));
  uint32_t hash=2166136261;for(uint8_t b:buffer)hash=(hash^b)*16777619U;
  printf("%u %u %d %08x\n",i,now,animation.settled(),hash);fwrite(buffer,1,1024,stdout);now+=dt;
 }
 return ferror(stdout)?5:0;
}
