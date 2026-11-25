// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstdio>
namespace rlcd_eyes {
// Calibrated ADC pin millivolts, summed over checked samples. Exact board
// divider is3:1. This is voltage, never a charge percentage/presence detector.
inline int batteryVoltage(int sumPinMv,unsigned samples,bool calibrated){
 if(!calibrated||!samples||sumPinMv<0)return -1;
 const int64_t mv=int64_t(sumPinMv)*3/samples;
 return mv>=2500&&mv<=4500?int(mv):-1;
}
inline int visibleBattery(int mv,uint32_t ageMs){return mv>=2500&&mv<=4500&&ageMs<=5000?mv:-1;}
inline void formatBattery(int mv,char (&out)[8]){
 if(visibleBattery(mv,0)<0)snprintf(out,sizeof out,"--");
 else snprintf(out,sizeof out,"%d.%02dV",mv/1000,mv%1000/10);
}
inline void formatBatteryHeader(int mv,uint32_t ageMs,char (&out)[12]){
 if(visibleBattery(mv,ageMs)<0)snprintf(out,sizeof out,"BAT --");
 else snprintf(out,sizeof out,"BAT %d.%02dV",mv/1000,mv%1000/10);
}
}
