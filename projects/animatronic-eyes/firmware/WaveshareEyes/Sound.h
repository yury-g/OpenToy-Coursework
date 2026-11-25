// SPDX-License-Identifier: MIT
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace waveshare_eyes {
// ES8311 datasheet 0x0e bits6/5: analog PGA and ADC modulator power-down.
inline bool codecCaptureOff(uint8_t value){return (value&0x60)==0x60;}
constexpr unsigned sampleRate=24000,soundSamples=1920; // 80 ms, no looping
inline int16_t soundSample(unsigned i,unsigned eye){
 if(i>=soundSamples)return 0;
 float t=float(i)/sampleRate;
 float envelope=std::min(1.f,std::min(i/240.f,(soundSamples-1-i)/480.f));
 float hz=eye==2?540.f:430.f;
 return int16_t(900.f*envelope*std::sin(6.283185307f*(hz*t-650.f*t*t)));
}
class SoundTrigger {
 unsigned previous_=0;uint32_t last_=0;bool played_=false;
public:
 unsigned update(uint32_t now,unsigned eyes,bool enabled){
  unsigned pressed=eyes&~previous_;previous_=eyes;
  if(!enabled||!pressed||(played_&&uint32_t(now-last_)<500))return 0;
  played_=true;last_=now;return pressed&1?1:2;
 }
};
}
