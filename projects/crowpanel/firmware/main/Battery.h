// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstdlib>
namespace crowpanel {
// Candidate vendor STC8 register format. Accept only coherent readings matching
// V1.0's 39k/100k divider; do not convert voltage into an invented percentage.
inline int batteryMillivolts(const uint8_t* data,bool ok){
 if(!ok)return -1;
 auto word=[&](unsigned at){return uint32_t(data[at])|(uint32_t(data[at+1])<<8)|(uint32_t(data[at+2])<<16)|(uint32_t(data[at+3])<<24);};
 uint32_t adc=word(0),bat=word(4);
 if(adc<1500||adc>3300||bat<2500||bat>4500||data[8]>100||data[9]>4||data[10]>4)return -1;
 if(std::abs(int(bat*100)-int(adc*139))>10000)return -1;
 return int(bat);
}
}
