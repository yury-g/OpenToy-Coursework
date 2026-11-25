// SPDX-License-Identifier: MIT
#pragma once
#include "Display.h"
#if __has_include("Companion.h")
#include "Companion.h"
#else
#include "../../../animatronic-eyes/firmware/AnimatronicEyes/Companion.h"
#endif
#include <cmath>
namespace rlcd_eyes {
// A target presentation adapter: consume the unchanged Companion pose, never
// enlarge its tiny bitmap. Evaluate rounded rectangles, circles and lid planes
// at each native pixel center. One bit, no spatial/temporal dithering.
struct NativeEye {
 float cx,cy,shapeX,shapeY,pupilX,pupilY,happyCut,blinkCut,halfLine;
 FaceState face;int mood;unsigned side;bool surprised;
 NativeEye(const feather_face::CompanionPose& pose,unsigned eye,uint32_t now,bool header):
  face(pose.eye[eye]),mood(pose.mood[eye]),side(eye),surprised(pose.originalShape){
  const auto& profile=feather_face::personality(pose.personality);
  auto shake=nesso_eyes::jitter(now,mood,eye);
  cx=200.f+4.f*((eye?92.f:36.f)-64.f+shake.x);
  cy=(header?175.f:150.f)+4.f*(face.y*.14f+shake.y+profile.eyeOffset[eye]);
  shapeX=28.f/profile.eyeWidth[eye];shapeY=32.f/profile.eyeHeight[eye];
  pupilX=14.f+face.pupilX*.06f;pupilY=16.f+face.pupilY*.07f;
  happyCut=std::max(0.f,std::min(8.f,pupilY-8.f))*std::max(mood,0)/100.f;
  blinkCut=32.f*(eye?face.right:face.left)/100.f;
  halfLine=profile.eyeWidth[eye]*2.f-2.f;
 }
 bool white(float px,float py)const{
  if(blinkCut>=32.f){ // Preserve a closed-eye stroke, with native rounded caps.
   float dx=std::max(0.f,std::fabs(px-cx)-halfLine),dy=py-cy;
   return dx*dx+dy*dy<=4.f;
  }
  const float x=(px-cx)*.25f+14.f,y=(py-cy)*.25f+16.f;
  const float nx=(x-14.f)*shapeX,ny=(y-16.f)*shapeY;
  if(surprised){if(nx*nx+ny*ny>196.f)return false;}
  else {
   float dx=std::max(0.f,std::fabs(nx)-6.f),dy=std::max(0.f,std::fabs(ny)-8.f);
   if(dx*dx+dy*dy>64.f)return false;
  }
  if(face.bottomLids?y>=32.f-blinkCut:y<blinkCut)return false;
  if(!surprised){
   if(mood<0&&y<(side?28.f-x:x)*(12.f/28.f)*(-mood/100.f))return false;
   if(mood>0&&y<happyCut)return false;
  }
  float dx=x-pupilX,dy=y-pupilY;
  return !face.pupils||dx*dx+dy*dy>64.f; // radius8 logical =32 native, still2x legacy.
 }
};
inline void renderNative(const feather_face::CompanionPose& pose,uint32_t now,uint8_t* buffer,bool header){
 std::fill(buffer,buffer+frameBytes,0); // Vendor bit0=black; keep proven controller init.
 for(unsigned eye=0;eye<2;eye++){
  NativeEye g(pose,eye,now,header);
  const int left=std::max(0,int(std::floor(g.cx-56.f))),right=std::min(width,int(std::ceil(g.cx+56.f)));
  const int top=std::max(header?headerHeight:0,int(std::floor(g.cy-64.f))),bottom=std::min(height,int(std::ceil(g.cy+64.f)));
  for(int y=top;y<bottom;y++)for(int x=left;x<right;x++)if(g.white(x+.5f,y+.5f))pixel(buffer,x,y,true);
 }
}
}
