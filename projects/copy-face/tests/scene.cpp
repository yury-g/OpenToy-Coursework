// SPDX-License-Identifier: MIT
#include "../core/Scene.h"
#include <cassert>
#include <vector>
#include <cstdio>
int main() {
 using namespace copy_face;
 for(bool header:{false,true})for(bool developer:{false,true}) {
  Layout l(header,developer);
  assert(l.eyeY()>=l.top() && l.eyeY()+256<l.mouthY());
  assert(l.mouthY()+mouthHeight<=544);
  assert(l.eyeX(0)>=0 && l.eyeX(1)+224<=1024);
  if(developer)assert(l.eyeX(0)>336);
  for(float aperture:{0.f,1.f})for(float bend:{-1.f,0.f,1.f}) {
   std::vector<uint16_t> pixels(mouthWidth*mouthHeight+2,0xdead);
   Pose p;p.aperture=aperture;p.bend=bend;
   renderMouth(p,pixels.data()+1);
   assert(pixels.front()==0xdead && pixels.back()==0xdead);
   unsigned lit=0;
   for(int i=0;i<mouthWidth*mouthHeight;i++)lit+=pixels[i+1]!=0;
   assert(lit>300);
   for(int x=0;x<mouthWidth;x++)assert(!pixels[x+1] && !pixels[1+(mouthHeight-1)*mouthWidth+x]);
  }
 }
 feather_face::CompanionPose eyes;Pose pose;pose.x=1;pose.y=-1;
 applyGaze(pose,eyes);assert(eyes.eye[0].pupilX==100 && eyes.eye[1].pupilY==-100);
 assert(syntheticFrame(9500).count==0);
 auto f=syntheticFrame(1000);Pose seam;seam.lips=normalizeLips(f.faces[0].lips);
 std::vector<uint16_t> pixels(mouthWidth*mouthHeight+2,0xdead);
 renderMouth(seam,pixels.data()+1);assert(pixels.front()==0xdead&&pixels.back()==0xdead);
 for(unsigned i=0;i<lipPoints;i++){
  seam.lips.upper[i]={-1+2.f*i/(lipPoints-1),-.25f};
  seam.lips.lower[i]={-1+2.f*i/(lipPoints-1),.25f};
 }
 renderMouth(seam,pixels.data()+1);assert(pixels[1+64*mouthWidth+160]==0);
 assert(pixels[1+31*mouthWidth+160]!=0);
 puts("scene: header bounds, mouth extrema, buffer guards and gaze passed");
}
