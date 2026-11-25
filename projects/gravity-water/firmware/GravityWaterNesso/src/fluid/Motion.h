// SPDX-License-Identifier: MIT
#pragma once
#include "Gravity.h"
#include <cstdint>
namespace gravity_water {
// Fast artistic shake response. In-plane inputs use the same axes as gravity.
class Motion {
 public:
 uint32_t sequence=0;
 int kickX=0,kickY=0,power=0;
 bool moving=false;
 void sample(float x,float y,float z,float spin,float dt) {
  if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(spin)||!std::isfinite(dt)||dt<=0)return;
  dt=std::min(dt,.05f);
  if(!initialized_){bx_=x;by_=y;bz_=z;initialized_=true;return;}
  float dx=x-bx_,dy=y-by_,dz=z-bz_;
  float difference=std::sqrt(dx*dx+dy*dy+dz*dz);
  moving=difference>.075f || spin>10;
  cooldown_=std::max(0.f,cooldown_-dt);
  if(cooldown_==0 && (difference>.35f || spin>180)) {
   power=int(clamp(std::max(difference*150,spin*.5f),70,255));
   kickX=int(clamp(dx*650,-850,850));kickY=int(clamp(dy*650,-850,850));
   ++sequence;cooldown_=.04f;
  }
  const float alpha=dt/(.08f+dt);
  bx_+=alpha*dx;by_+=alpha*dy;bz_+=alpha*dz;
 }
 private:
 float bx_=0,by_=0,bz_=0,cooldown_=0;
 bool initialized_=false;
};
}
