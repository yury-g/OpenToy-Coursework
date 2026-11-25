// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
namespace gravity_water {
struct Point {float x,y;};
inline float clamp(float v,float lo,float hi) {return std::max(lo,std::min(hi,v));}
class Gravity {
 public:
 void sample(float gx,float gy,float dt) {
  if(!std::isfinite(gx)||!std::isfinite(gy)||!std::isfinite(dt)||dt<=0) return;
  dt=std::min(dt,.05f);
  const float magnitude=std::hypot(gx,gy);
  const float confidence=clamp((magnitude-.06f)/.18f,0,1);
  strength_+= (clamp(magnitude,0,1)-strength_)*(1-std::exp(-dt/.025f));
  if(confidence>0) {
   const float delta=std::remainder(std::atan2(gx,gy)-angle_,6.2831853f);
   angle_=std::remainder(angle_+delta*(1-std::exp(-dt*confidence/.016f)),6.2831853f);
  }
 }
 Point down() const {return {std::sin(angle_),std::cos(angle_)};}
 float strength() const {return strength_;}
 private:
 float angle_=0,strength_=0;
};
}
