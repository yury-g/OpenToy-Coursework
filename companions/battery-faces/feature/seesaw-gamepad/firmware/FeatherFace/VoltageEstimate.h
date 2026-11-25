// SPDX-License-Identifier: MIT
#pragma once
#include <math.h>
// Generic single-cell LiPo resting-voltage approximation, not a fuel gauge.
// Charging and load sag change this estimate; never infer charger state from it.
inline float voltagePercent(float v){
 if(!isfinite(v)||v<2.8f||v>4.5f)return NAN;
 constexpr float volts[]={3.3f,3.5f,3.6f,3.7f,3.75f,3.8f,3.85f,3.9f,4.0f,4.1f,4.2f};
 constexpr float pct[]={0,5,10,20,35,50,60,70,80,90,100};
 if(v<=volts[0])return 0;if(v>=volts[10])return 100;
 for(unsigned i=1;i<11;i++)if(v<=volts[i])return pct[i-1]+(v-volts[i-1])*(pct[i]-pct[i-1])/(volts[i]-volts[i-1]);
 return NAN;
}
