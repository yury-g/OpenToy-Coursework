// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
namespace gravity_water {
// Bounded 8kHz mono texture: filtered noise + a soft triangle undertone.
// All effects mix into one PCM stream; no files, heap allocation or trig.
class TextureSound {
 public:
 bool quiet() const {return envelope_==0;}
 void render(int16_t* out,size_t count,int heat,int motion,bool touch) {
  heat=std::max(0,std::min(255,heat));motion=std::max(0,std::min(255,motion));
  int target=std::min(255,heat/2+motion/2+(touch?55:0));
  for(size_t i=0;i<count;i++) {
   random_^=random_<<13;random_^=random_>>17;random_^=random_<<5;
   int noise=int(random_&65535)-32768;
   low_+=(noise-low_)/8;deep_+=(low_-deep_)/16;
   if(envelope_<target)++envelope_;else if(envelope_>target)--envelope_;
   phase_=(phase_+uint32_t(420+heat*2))&65535;
   int triangle=int(phase_<32768?phase_:65535-phase_)-16384;
   int texture=deep_*2+(low_-deep_)/2+triangle/5;
   int sample=texture*envelope_/512;
   out[i]=int16_t(std::max(-14000,std::min(14000,sample)));
  }
 }
 private:
 uint32_t random_=0xbb67ae85,phase_=0;
 int32_t low_=0,deep_=0;
 int envelope_=0;
};
}
