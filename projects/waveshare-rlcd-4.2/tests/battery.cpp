// SPDX-License-Identifier: MIT
#include "../firmware/RlcdEyes/Battery.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){
 using rlcd_eyes::batteryVoltage;
 assert(batteryVoltage(4800,4,true)==3600); // Four calibrated1.2V pin samples, divider3:1.
 assert(batteryVoltage(5460,4,true)==4095);
 assert(batteryVoltage(6000,4,true)==4500);
 assert(batteryVoltage(6004,4,true)==-1); // Out-of-range node is not a fabricated battery level.
 assert(batteryVoltage(0,4,true)==-1);
 assert(batteryVoltage(4800,0,true)==-1);
 assert(batteryVoltage(4800,4,false)==-1);
 assert(batteryVoltage(-4,4,true)==-1);
 assert(rlcd_eyes::visibleBattery(3644,5000)==3644);
 assert(rlcd_eyes::visibleBattery(3644,5001)==-1);
 assert(rlcd_eyes::visibleBattery(-1,0)==-1);
 uint32_t sample=0xfffffff0U,now=20;assert(rlcd_eyes::visibleBattery(3644,now-sample)==3644);
 char text[8];rlcd_eyes::formatBattery(3644,text);assert(!std::strcmp(text,"3.64V"));
 rlcd_eyes::formatBattery(-1,text);assert(!std::strcmp(text,"--"));
 rlcd_eyes::formatBattery(5000,text);assert(!std::strcmp(text,"--"));
 char hdr[12];
 rlcd_eyes::formatBatteryHeader(3644,0,hdr);assert(!std::strcmp(hdr,"BAT 3.64V"));
 assert(394-int(std::strlen(hdr)*12)>=280);
 rlcd_eyes::formatBatteryHeader(3644,5001,hdr);assert(!std::strcmp(hdr,"BAT --"));
 rlcd_eyes::formatBatteryHeader(-1,0,hdr);assert(!std::strcmp(hdr,"BAT --"));
 rlcd_eyes::formatBatteryHeader(5000,0,hdr);assert(!std::strcmp(hdr,"BAT --"));
 assert(394-int(std::strlen(hdr)*12)>=280);
 puts("PASS battery divider, calibrated voltage, invalid/unavailable reads, neutral BAT -- header, and no percentage inference");
}
