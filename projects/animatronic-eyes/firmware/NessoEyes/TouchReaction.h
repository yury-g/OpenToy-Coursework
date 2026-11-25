// SPDX-License-Identifier: MIT
#pragma once
#if __has_include("EyePixels.h")
#include "EyePixels.h"
#else
#include "../../shared/opentoy/EyePixels.h"
#endif
#include <algorithm>
namespace nesso_eyes {
constexpr int width=240,height=135,bar=15;
struct Pose {FaceState face;int mood=100;int eyeMood[2]={100,100};};
class Reaction {
 struct Eye {
  bool anger=false,touched=false,flinch=false;
  uint32_t angryAt=0,touchAt=0,flinchAt=0,recovery=6000,reopen=280;
  int strength=0;
  int mood(uint32_t now)const{
   if(!anger)return 100;
   float t=std::min(1.f,uint32_t(now-angryAt)/float(recovery));
   return int(-strength+(100+strength)*t*t*(3-2*t));
  }
 } eye_[2];
 unsigned previousMask_=0,events_=0;bool previousDown_=false;
public:
 Pose update(uint32_t now,bool down,unsigned mask){
  bool event=down&&(!previousDown_||mask!=previousMask_);if(event)++events_;
  Pose p;
  for(unsigned i=0;i<2;i++){
   auto& e=eye_[i];bool direct=down&&(mask&(1U<<i));
   if(down){
    int strength=direct?100:mask?(i?42:50):(i?75:85);
    if(direct||e.mood(now)>=-strength){
     e.anger=true;e.strength=strength;e.angryAt=now;
     e.recovery=direct?(i?7200:6200)+180*(events_%3):(i?3000:3400);
    }
    if(direct){e.touched=true;e.touchAt=now;e.reopen=(i?360:270)+15*(events_%3);}
    if(event&&mask&&!direct){e.flinch=true;e.flinchAt=now;}
   }
   unsigned lid=0;uint32_t age=now-e.touchAt;
   if(e.touched&&age<e.reopen)lid=100*(e.reopen-age)/e.reopen;
   uint32_t flinchAge=now-e.flinchAt;
   if(e.flinch&&flinchAge>=60&&flinchAge<220){unsigned phase=flinchAge-60;lid=std::max(lid,phase<60?phase*30/60:(160-phase)*30/100);}
   if(i)p.face.right=lid;else p.face.left=lid;
   p.eyeMood[i]=e.mood(now);
  }
  previousDown_=down;previousMask_=down?mask:0;
  p.mood=std::min(p.eyeMood[0],p.eyeMood[1]);
  p.face.expression=p.mood<0?ANGRY:p.mood>0?HAPPY:NEUTRAL;
  return p;
 }
};
// Preserve original v1 pixels at angry/neutral/happy endpoints; vary only
// their existing mask heights between endpoints, never bitmap cross-fading.
inline bool pixel(FaceState state,int mood,unsigned eye,int x,int y){
 state.expression=NEUTRAL;
 if(!eyeLayerPixel(state,eye,x,y))return false;
 if((eye?state.right:state.left)==100)return true;
 if(mood<0&&y*28*100<(eye?28-x:x)*12*(-mood))return false;
 if(mood>0&&y*100<16*mood)return false;
 return true;
}
// The happy lid rests at y=8 instead of y=16. Follow an upward gaze only
// far enough to keep the enlarged pupil's top visible.
inline int pupilClearMood(const FaceState& state,int mood){
 if(mood<=0)return mood;
 int pupilTop=16+state.pupilY*7/100-8;
 return mood*std::max(0,std::min(8,pupilTop))/16;
}
// Match Feather Eyes 1: twice the original pupil radius, clipped by the
// unchanged expression and eyelid masks. Keep a fully closed eye as a line.
inline bool largePupilPixel(FaceState state,int mood,unsigned eye,int x,int y){
 bool pupils=state.pupils;state.pupils=false;
 if(!pixel(state,pupilClearMood(state,mood),eye,x,y))return false;
 if(!pupils||(eye?state.right:state.left)==100)return true;
 int dx=x-(14+state.pupilX*6/100),dy=y-(16+state.pupilY*7/100);
 return dx*dx+dy*dy>64;
}

}
