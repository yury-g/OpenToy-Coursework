// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Geometry.h"
namespace copy_face {
struct Detection {
 float x=0,y=0,w=0,h=0,confidence=0;
 // A real adapter must leave this false without validated lip geometry.
 bool mouthValid=false;
 float aperture=0,bend=0;
 LipLandmarks lips;
 EyeLandmarks eyes[2];
 bool keypointsValid=false;
 Point keypoints[5]; // Left eye, left mouth corner, nose, right eye, right mouth corner.
};
struct Frame { uint32_t captured=0; unsigned count=0; Detection faces[8]; };
struct Pose { float x=0,y=0,aperture=0,bend=0; bool tracking=false; LipShape lips; };
inline float clamp(float x,float lo,float hi) { return std::max(lo,std::min(hi,x)); }
inline bool valid(const Detection& d) {
 return std::isfinite(d.x)&&std::isfinite(d.y)&&std::isfinite(d.w)&&std::isfinite(d.h)&&
        std::isfinite(d.confidence)&&d.confidence>=0&&d.confidence<=1&&
        d.x>=0&&d.y>=0&&d.w>.02f&&d.h>.02f&&d.x+d.w<=1&&d.y+d.h<=1;
}
inline float distance(const Detection& a,const Detection& b) {
 return std::hypot(a.x+a.w*.5f-b.x-b.w*.5f,a.y+a.h*.5f-b.y-b.h*.5f);
}
inline bool matches(const Detection& a,const Detection& b) {
 const float intersection=std::max(0.f,std::min(a.x+a.w,b.x+b.w)-std::max(a.x,b.x))*
                          std::max(0.f,std::min(a.y+a.h,b.y+b.h)-std::max(a.y,b.y));
 const float ratio=a.w*a.h/(b.w*b.h);
 return ratio>.4f&&ratio<2.5f&&
        (intersection/(a.w*a.h+b.w*b.h-intersection)>.15f||distance(a,b)<.15f);
}
class Tracker {
 Detection primary_,candidate_;
 uint32_t seen_=0,candidateAt_=0,frameAt_=0,stepAt_=0;
 unsigned candidateCount_=0;
 bool active_=false,haveFrame_=false,haveStep_=false;
 Pose pose_;
 void expire(uint32_t now) { if(active_&&uint32_t(now-seen_)>=450)active_=false; }
public:
 bool selected(Detection& out,uint32_t& captured,uint32_t now)const {
  if(!active_||uint32_t(now-seen_)>=450)return false;
  out=primary_;captured=seen_;return true;
 }
 void observe(const Frame& f,uint32_t now) {
  expire(now);
  if(f.count>8||uint32_t(now-f.captured)>250||
     (haveFrame_&&int32_t(f.captured-frameAt_)<=0))return;
  frameAt_=f.captured;haveFrame_=true;
  if(active_) {
   int best=-1;float nearest=2;
   for(unsigned i=0;i<f.count;i++) {
    const auto& d=f.faces[i];
    if(valid(d)&&d.confidence>=.5f&&matches(primary_,d)&&distance(primary_,d)<nearest) {
     best=int(i);nearest=distance(primary_,d);
    }
   }
   if(best>=0) {primary_=f.faces[best];seen_=f.captured;candidateCount_=0;}
   return; // A stronger distant face cannot steal a live spatial track.
  }
  int best=-1;float score=-1;
  if(candidateCount_&&uint32_t(f.captured-candidateAt_)>250)candidateCount_=0;
  // Keep an acquisition candidate despite detector ordering/score changes.
  if(candidateCount_)for(unsigned i=0;i<f.count;i++) {
   const auto& d=f.faces[i];
   if(valid(d)&&d.confidence>=.7f&&matches(candidate_,d)) {
    float s=1-distance(candidate_,d);if(s>score){score=s;best=int(i);}
   }
  }
  if(best<0) {
   candidateCount_=0;
   for(unsigned i=0;i<f.count;i++) {
    const auto& d=f.faces[i];float s=d.confidence*d.w*d.h;
    if(valid(d)&&d.confidence>=.7f&&s>score){score=s;best=int(i);}
   }
  }
  if(best<0){candidateCount_=0;return;}
  candidate_=f.faces[best];candidateAt_=f.captured;
  if(++candidateCount_>=3) {primary_=candidate_;seen_=f.captured;active_=true;candidateCount_=0;}
 }
 Pose step(uint32_t now,bool mirror=false) {
  expire(now);
  float dt=haveStep_?float(uint32_t(now-stepAt_)):0;
  stepAt_=now;haveStep_=true;
  float x=0,y=0,aperture=0,bend=0;
  LipShape target;
  if(active_) {
   x=clamp((primary_.x+primary_.w*.5f)*2-1,-1,1);
   y=clamp((primary_.y+primary_.h*.5f)*2-1,-1,1);
   if(mirror)x=-x;
   target=normalizeLips(primary_.lips);
   if(primary_.mouthValid&&std::isfinite(primary_.aperture)&&std::isfinite(primary_.bend)) {
    aperture=clamp(primary_.aperture,0,1);bend=clamp(primary_.bend,-1,1);
   }
  }
  float gaze=1-std::exp(-dt/90.f),mouth=1-std::exp(-dt/120.f);
  pose_.x+=(x-pose_.x)*gaze;pose_.y+=(y-pose_.y)*gaze;
  pose_.aperture+=(aperture-pose_.aperture)*mouth;pose_.bend+=(bend-pose_.bend)*mouth;
  if(target.valid||pose_.lips.valid) {
   if(!pose_.lips.valid)for(unsigned i=0;i<lipPoints;i++) {
    pose_.lips.upper[i]=pose_.lips.lower[i]={-1+2.f*i/(lipPoints-1),0};
   }
   for(unsigned i=0;i<lipPoints;i++) {
    unsigned src=mirror?lipPoints-1-i:i;
    Point a=target.valid?target.upper[src]:Point{-1+2.f*src/(lipPoints-1),0};
    Point b=target.valid?target.lower[src]:a;
    if(mirror){a.x=-a.x;b.x=-b.x;}
    auto smooth=[&](Point& p,Point value){p.x+=(value.x-p.x)*mouth;p.y+=(value.y-p.y)*mouth;};
    smooth(pose_.lips.upper[i],a);smooth(pose_.lips.lower[i],b);
   }
   pose_.lips.valid=true; // Drawable fallback geometry, not inference availability.
  }
  pose_.tracking=active_;return pose_;
 }
};
}
