// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
// Consumes the unchanged CompanionPose. Geometry is evaluated at native pixel
// centers; one-pixel signed-distance coverage replaces magnified binary pixels.
namespace crowpanel {
inline uint16_t coveredColor(uint16_t ink,float coverage){
 coverage=std::max(0.f,std::min(1.f,coverage));
 unsigned r=unsigned(((ink>>11)&31)*coverage+.5f),g=unsigned(((ink>>5)&63)*coverage+.5f),b=unsigned((ink&31)*coverage+.5f);
 return uint16_t((r<<11)|(g<<5)|b);
}
inline void renderSmoothEye(const feather_face::CompanionPose& pose,unsigned eye,int scale,uint16_t ink,uint16_t* output){
 const auto& face=pose.eye[eye];const auto& profile=feather_face::personality(pose.personality);
 const float wx=profile.eyeWidth[eye]/28.f,hy=profile.eyeHeight[eye]/32.f;
 const float native=scale*std::min(wx,hy);
 const unsigned lid=eye?face.right:face.left;
 const float pupilX=14+face.pupilX*6.f/100,pupilY=16+face.pupilY*7.f/100;
 const float upper=16*nesso_eyes::pupilClearMood(face,pose.mood[eye])/100.f;
 const float slope=12*std::max(0,-pose.mood[eye])/2800.f;
 const float slopeNorm=std::sqrt(1+slope*slope);
 const int width=28*scale,height=32*scale;
 for(int py=0;py<height;py++)for(int px=0;px<width;px++){
  float x=(px+.5f)/scale,y=(py+.5f)/scale,d;
  if(lid>=100){
   // A thin native stroke, with round ends, preserves the closed-eye gesture.
   float dx=std::max(0.f,std::fabs(px+.5f-14*scale)-(profile.eyeWidth[eye]*scale/2.f-3));
   float dy=py+.5f-16*scale;d=std::sqrt(dx*dx+dy*dy)-1.5f;
  }else{
   float sx=(x-14)/wx,sy=(y-16)/hy;
   if(pose.originalShape)d=(std::sqrt(sx*sx+sy*sy)-14)*native;
   else{
    float qx=std::fabs(sx)-6,qy=std::fabs(sy)-8;
    float ox=std::max(qx,0.f),oy=std::max(qy,0.f);
    d=(std::sqrt(ox*ox+oy*oy)+std::min(std::max(qx,qy),0.f)-8)*native;
   }
   if(lid){float edge=32*lid/100.f;d=std::max(d,(face.bottomLids?y-(32-edge):edge-y)*scale);}
   if(!pose.originalShape){
    if(pose.mood[eye]>0)d=std::max(d,(upper-y)*scale);
    if(pose.mood[eye]<0)d=std::max(d,((eye?28-x:x)*slope-y)*scale/slopeNorm);
   }
   if(face.pupils){float dx=x-pupilX,dy=y-pupilY;d=std::max(d,(8-std::sqrt(dx*dx+dy*dy))*scale);}
  }
  output[py*width+px]=coveredColor(ink,.5f-d);
 }
}
}
