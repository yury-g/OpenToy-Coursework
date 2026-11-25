// SPDX-License-Identifier: MIT
// Same scene/renderer as the target, with synthetic observations only.
#include "../core/Scene.h"
#include "../core/DeveloperView.h"
#include <fstream>
#include <cstdio>
#include <vector>
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 std::ofstream status(argv[1]);
 using namespace copy_face;
 Tracker tracker;feather_face::Companion companion(1);Layout layout;
 std::vector<uint16_t> screen(1024*600),eye(224*256),mouth(mouthWidth*mouthHeight);
 std::vector<uint16_t> raw(previewWidth*previewHeight),tile(previewWidth*previewHeight);
 std::vector<unsigned char> rgb(1024*600*3);
 for(unsigned frame=0;frame<120;frame++) {
  uint32_t now=frame*100;auto observation=syntheticFrame(now);tracker.observe(observation,now);auto pose=tracker.step(now);
  syntheticCamera(observation,raw.data());
  PreviewImage im{raw.data(),previewWidth,previewHeight,previewWidth,raw.size(),now};
  renderDeveloper(im,tracker,now,false,tile.data());
  auto selected=overlaySelection(im,tracker,now);
  auto shape=selected.tracked?normalizeLips(selected.face.lips):LipShape{};
  status<<expressionName(expression(shape))<<"|"<<(selected.tracked?eyeActivity(selected.face.eyes[0]):"--")<<"|"<<(selected.tracked?eyeActivity(selected.face.eyes[1]):"--")<<"|"<<selected.tracked<<"\n";
  auto eyes=companion.update(512,512,0,false,now,0);applyGaze(pose,eyes);
  std::fill(screen.begin(),screen.end(),0);
  for(unsigned e=0;e<2;e++) {
   crowpanel::renderSmoothEye(eyes,e,eyeScale,0xffff,eye.data());
   for(int y=0;y<256;y++)for(int x=0;x<224;x++)screen[(layout.eyeY()+y)*1024+layout.eyeX(e)+x]=eye[y*224+x];
  }
  renderMouth(pose,mouth.data());
  for(int y=0;y<mouthHeight;y++)for(int x=0;x<mouthWidth;x++)screen[(layout.mouthY()+y)*1024+layout.mouthX()+x]=mouth[y*mouthWidth+x];
  for(int y=0;y<previewHeight;y++)for(int x=0;x<previewWidth;x++)screen[(96+y)*1024+16+x]=tile[y*previewWidth+x];
  for(unsigned i=0;i<screen.size();i++) {
   auto c=screen[i];rgb[3*i]=((c>>11)&31)*255/31;rgb[3*i+1]=((c>>5)&63)*255/63;rgb[3*i+2]=(c&31)*255/31;
  }
  if(fwrite(rgb.data(),1,rgb.size(),stdout)!=rgb.size())return 1;
 }
}
