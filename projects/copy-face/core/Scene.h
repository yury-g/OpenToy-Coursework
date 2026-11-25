// SPDX-License-Identifier: MIT
#pragma once
#include "Tracking.h"
#include "Companion.h"
#include "Smooth.h"
#include "Strokes.h"
namespace copy_face {
constexpr int eyeScale=8,mouthWidth=320,mouthHeight=128;
struct Layout {
 bool header=true;
 bool developer=true;
 explicit Layout(bool shown=true,bool dev=true):header(shown),developer(dev){}
 int top()const{return header?48:0;}
 int eyeX(unsigned eye)const{return developer?(eye?744:400):(eye?612:188);}
 int eyeY()const{return top()+40;}
 int mouthX()const{return developer?524:352;}
 int mouthY()const{return 404;}
};
inline void applyGaze(const Pose& pose,feather_face::CompanionPose& eyes) {
 for(auto& e:eyes.eye) {e.pupilX=int(pose.x*100);e.pupilY=int(pose.y*100);}
}
inline void renderMouth(const Pose& pose,uint16_t* output) {
 if(pose.lips.valid) {
  std::fill_n(output,mouthWidth*mouthHeight,0);
  float extent=.01f;
  for(unsigned i=0;i<lipPoints;i++)extent=std::max(extent,std::max(std::fabs(pose.lips.upper[i].y),std::fabs(pose.lips.lower[i].y)));
  float scale=std::min(132.f,54.f/extent);
  auto point=[&](Point p){return Point{160+p.x*scale,64+p.y*scale};};
  for(unsigned i=1;i<lipPoints;i++) {
   if(contact(pose.lips,i-1)&&contact(pose.lips,i)) {
    auto mid=[&](unsigned k){return point(Point{(pose.lips.upper[k].x+pose.lips.lower[k].x)/2,(pose.lips.upper[k].y+pose.lips.lower[k].y)/2});};
    stroke(output,mouthWidth,mouthHeight,mid(i-1),mid(i),3,0xffff);
   } else {
    stroke(output,mouthWidth,mouthHeight,point(pose.lips.upper[i-1]),point(pose.lips.upper[i]),2.5f,0xffff);
    stroke(output,mouthWidth,mouthHeight,point(pose.lips.lower[i-1]),point(pose.lips.lower[i]),2.5f,0xffff);
   }
  }
  return;
 }
 const float bend=clamp(pose.bend,-1,1),open=clamp(pose.aperture,0,1);
 for(int y=0;y<mouthHeight;y++)for(int x=0;x<mouthWidth;x++) {
  const float dx=x+.5f-mouthWidth*.5f,u=clamp(dx/132.f,-1,1);
  const float curve=64+30*bend*(1-u*u),slope=-60*bend*u/132;
  const float radius=3+open*28*std::sqrt(std::max(0.f,1-u*u));
  const float vertical=(y+.5f-curve)/std::sqrt(1+slope*slope);
  const float beyond=std::max(0.f,std::fabs(dx)-132);
  const float d=std::hypot(beyond,vertical)-radius;
  output[y*mouthWidth+x]=crowpanel::coveredColor(0xffff,.5f-d);
 }
}
// Scripted input only. Never presented as a real detector or landmark model.
inline Frame syntheticFrame(uint32_t now) {
 Frame f;f.captured=now;float t=float(now%12000)/1000;
 if(t>=9)return f;
 f.count=1;auto& d=f.faces[0];
 d.x=.33f+.17f*std::sin(t*.8f);d.y=.1f+.04f*std::sin(t*.55f);
 d.w=.34f;d.h=.75f;d.confidence=.95f;d.mouthValid=true;
 d.bend=t<2?.85f:t<4?-.75f:0;
 d.aperture=t<6?0:.45f+.4f*std::sin(t*3);
 d.lips.valid=true;d.lips.imageAspect=16.f/9;
 for(unsigned i=0;i<lipPoints;i++) {
  float u=float(i)/(lipPoints-1),curve=4*u*(1-u);
  float x=d.x+d.w*(.2f+.6f*u),y=d.y+d.h*.72f+d.bend*.04f*curve;
  float gap=d.aperture*.1f*curve;
  // Slight asymmetry exercises copying actual samples instead of preset curves.
  y+=.012f*std::sin(u*6.283f)*std::sin(t);
  d.lips.upper[i]={x,y-gap/2};d.lips.lower[i]={x,y+gap/2};
 }
 for(unsigned e=0;e<2;e++) {
  auto& eye=d.eyes[e];eye.valid=true;eye.imageAspect=16.f/9;
  float cx=d.x+d.w*(e?.7f:.3f),cy=d.y+d.h*.36f;
  float w=d.w*.18f,h=(uint32_t(now+e*300)%2600<220)?.001f:.018f;
  eye.contour={Point{cx-w/2,cy},Point{cx-w/4,cy-h},Point{cx+w/4,cy-h},Point{cx+w/2,cy},Point{cx+w/4,cy+h},Point{cx-w/4,cy+h}};
 }
 return f;
}
}
