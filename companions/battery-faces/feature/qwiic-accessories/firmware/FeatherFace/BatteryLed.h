// SPDX-License-Identifier: MIT
#pragma once
#include "BatteryDiagnostics.h"
struct BatteryLedColor {uint8_t r,g,b;};
inline BatteryLedColor batteryLed(const BatteryState &b,const BatteryDiagnostics &d,uint32_t now){
 if(!b.valid){uint8_t level=now%2000<200?8:1;return {level,0,level};}
 unsigned phase=now%3000;
 uint8_t level=2+(phase<1500?phase:3000-phase)*10/1500;
 if(d.evidence==ChargeEvidence::Gaining){unsigned p=now%2000;level=p<120||(p>=250&&p<370)?16:2;}
 if(b.percent<20)return {level,0,0};
 if(b.percent<=70)return {level,(uint8_t)(level/2),0};
 return {0,level,0};
}
