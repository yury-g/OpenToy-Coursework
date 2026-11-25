// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
namespace gravity_water {
// Smooth the PWM midpoint as well as the AC texture when starting/stopping.
class PwmEnvelope {
 public:
 int step(int sample,bool active) {
  sample=std::max(-14000,std::min(14000,sample));
  bias_+=std::max(-4,std::min(4,(active?128:0)-bias_));
  return bias_+sample*bias_/16384;
 }
 private:int bias_=0;
};
}
