// SPDX-License-Identifier: MIT
#include "../firmware/RlcdEyes/NativeEyes.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace rlcd_eyes;
// Decode panel bytes independently of the drawing helper.
bool lit(const uint8_t* b,int x,int y){return b[(x>>1)*75+((299-y)>>2)]&(128>>(((299-y)&3)*2+(x&1)));}
int main(){
 std::array<uint8_t,15002> guarded{};guarded.front()=0xA5;guarded.back()=0x5A;auto* b=guarded.data()+1;
 feather_face::CompanionPose p;p.personality=1;p.mood[0]=p.mood[1]=0;
 for(auto& e:p.eye)e.pupils=false;
 renderNative(p,0,b,true);assert(!lit(b,0,0)&&!lit(b,399,299)&&!lit(b,200,175));
 assert(lit(b,88,175)&&lit(b,312,175));assert(!lit(b,36,111));
 auto solid=guarded;
 for(auto& e:p.eye)e.pupils=true;
 renderNative(p,0,b,true);assert(!lit(b,88,175));assert(!lit(b,119,175)&&lit(b,120,175));
 int removed=0,subpixelRows=0;
 for(int y=0;y<300;y++)for(int x=0;x<200;x++)removed+=lit(solid.data()+1,x,y)&&!lit(b,x,y);
 assert(removed>=3200&&removed<=3240); // Native circle radius32, twice legacy radius16.
 for(int y=111;y<160;y++)for(int x=0;x<180;x++)if(lit(b,x,y)){subpixelRows+=(x%4)!=0;break;}
 assert(subpixelRows>15); // Native contour steps are not locked to four-pixel columns.
 p.mood[0]=100;renderNative(p,0,b,true);assert(!lit(b,50,142)&&lit(b,50,143));
 p.eye[0].left=100;renderNative(p,0,b,true);assert(lit(b,88,175)&&!lit(b,88,170));assert(!lit(b,312,175));
 p.eye[0].left=0;p.mood[0]=p.mood[1]=0;renderNative(p,0,b,false);assert(!lit(b,88,150)&&lit(b,120,150));
 // Exercise all personalities, moods, extreme gaze, both blink directions and viewport modes.
 for(unsigned id=1;id<=3;id++)for(int mood:{-100,0,100})for(int gaze:{-100,100})for(unsigned lid:{0U,35U,99U,100U})for(bool bottom:{false,true})for(bool header:{false,true}){
  p.personality=id;p.mood[0]=p.mood[1]=mood;
  for(auto& e:p.eye){e.pupilX=e.pupilY=gaze;e.left=e.right=lid;e.bottomLids=bottom;}
  renderNative(p,0xfffffff0U,b,header);
  assert(guarded.front()==0xA5&&guarded.back()==0x5A);
  if(header)for(int y=0;y<50;y++)for(int x=0;x<400;x++)assert(!lit(b,x,y));
 }
 puts("PASS native black polarity, analytic contours, 2x pupil area, raised lid, independent closure, viewport and extreme poses");
}
