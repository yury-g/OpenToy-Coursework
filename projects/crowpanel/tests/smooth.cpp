// SPDX-License-Identifier: MIT
#include <cassert>
#include <vector>
#include <cstdio>
#include "../../animatronic-eyes/firmware/AnimatronicEyes/Companion.h"
#include "../firmware/main/Smooth.h"
int main(){
 feather_face::CompanionPose p;p.personality=1;p.mood[0]=p.mood[1]=0;
 for(unsigned e=0;e<2;e++){p.eye[e].pupils=true;p.eye[e].left=p.eye[e].right=0;}
 for(int scale:{7,8}){
  int w=28*scale,h=32*scale;std::vector<uint16_t> pixels(w*h+2,0xabcd);
  crowpanel::renderSmoothEye(p,0,scale,0xffff,pixels.data()+1);
  assert(pixels.front()==0xabcd&&pixels.back()==0xabcd);
  auto at=[&](int x,int y){return pixels[1+y*w+x];};
  assert(at(14*scale,16*scale)==0); // pupil center
  assert(at(14*scale+7*scale,16*scale)==0); // retained doubled radius
  assert(at(14*scale+9*scale,16*scale)==0xffff);
  assert(at(0,0)==0);
  unsigned covered=0;for(int i=1;i<=w*h;i++)if(pixels[i]!=0&&pixels[i]!=0xffff)++covered;
  assert(covered>100); // native edge coverage, not binary enlarged source pixels
 }
 p.eye[0].left=100;std::vector<uint16_t> shut(224*256);
 crowpanel::renderSmoothEye(p,0,8,0xffff,shut.data());
 assert(shut[128*224+112]!=0 && shut[110*224+112]==0);
 p.eye[0].left=0;p.mood[0]=-100;
 std::vector<uint16_t> angry(224*256);crowpanel::renderSmoothEye(p,0,8,0xffff,angry.data());
 assert(angry[4*8*224+24*8]==0); // inward sloping angry lid
 for(unsigned persona:{1U,2U,3U})for(unsigned e:{0U,1U})for(bool bottom:{false,true}){
  p.personality=persona;p.mood[e]=0;p.eye[e].left=p.eye[e].right=50;p.eye[e].bottomLids=bottom;
  std::vector<uint16_t> bounded(224*256+2,0xabcd);crowpanel::renderSmoothEye(p,e,8,0xffff,bounded.data()+1);
  assert(bounded.front()==0xabcd&&bounded.back()==0xabcd);
  assert(bounded[1+12*8*224+5*8]==(bottom?0xffff:0));
  assert(bounded[1+20*8*224+5*8]==(bottom?0:0xffff));
 }
 puts("PASS native smooth geometry, doubled pupils, edge coverage, lids and buffer bounds");
}
