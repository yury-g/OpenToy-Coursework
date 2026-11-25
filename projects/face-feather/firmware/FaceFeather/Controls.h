// SPDX-License-Identifier: MIT
#pragma once
#if __has_include("FaceProtocol.h")
#include "FaceProtocol.h"
#else
#include "../../../xiao-class/firmware/ToyBLE/FaceProtocol.h"
#endif
#include <algorithm>
#include <cstdlib>
namespace feather_face {
constexpr uint32_t SELECT=1UL<<0,B=1UL<<1,Y=1UL<<2,A=1UL<<5,X=1UL<<6,START=1UL<<16;
constexpr uint32_t buttonsMask=SELECT|B|Y|A|X|START;
inline bool fresh(bool ok,uint32_t at,uint32_t now){return ok&&uint32_t(now-at)<100000;}
class Controls {
 int version;Expression emotion=NEUTRAL;uint32_t previous=0;
public:
 explicit Controls(int v):version(v){}
 static int axis(int raw){int d=std::max(0,std::min(1023,raw))-512;return std::abs(d)<=64?0:std::max(-100,std::min(100,(d>0?d-64:d+64)*100/447));}
 FaceState update(int rawX,int rawY,uint32_t buttons,bool valid){
  FaceState s;s.doodle=version==2;
  if(!valid){emotion=NEUTRAL;previous=0;return s;}
  uint32_t pressed=buttons&~previous;previous=buttons;
  bool select=buttons&SELECT,start=buttons&START;
  if(pressed&A)emotion=version==2&&select?NERVOUS:HAPPY;
  if(pressed&B)emotion=version==2&&select?NEUTRAL:ANGRY;
  if(pressed&X)emotion=SURPRISED;
  if(pressed&Y)emotion=version==2?SORRY:NEUTRAL;
  s.expression=emotion;if(version==2)s.jaw=emotion==HAPPY?25:emotion==SURPRISED?92:0;int x=-axis(rawX),y=axis(rawY);
  if(select&&start){s.left=s.right=100;return s;}
  if(version==1){
   if(start){s.left=std::max(-x,std::abs(y));s.right=std::max(x,std::abs(y));s.bottomLids=y<=0;}
   else if(select)s.y=y;
   else{s.pupilX=x;s.pupilY=y;}
  }else{
   if(start){s.jaw+=((100-s.jaw)*std::max(0,-y))/100;s.left=std::max(0,-x);s.right=std::max(0,x);}
   else if(select){s.mouthBend=-y;s.mouthSkew=x;}
   else{s.eyeBend=-y;s.browBend=x;}
  }
  return s;
 }
};
}
