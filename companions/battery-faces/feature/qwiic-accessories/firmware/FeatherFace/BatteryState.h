// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <math.h>
enum class BatteryTrend {Unknown,Settling,Charging,Discharging,Steady};
struct BatteryState {
 bool valid=false,started=false;
 float volts=0,percent=0,rate=0,hoursLeft=-1;
 uint32_t first=0;
 BatteryTrend trend=BatteryTrend::Unknown;
 void update(float v,float p,float r,uint32_t now) {
  valid=isfinite(v)&&isfinite(p)&&v>=2&&v<=4.5f&&p>=0&&p<=110;
  hoursLeft=-1;trend=BatteryTrend::Unknown;
  if(!valid){started=false;return;}
  volts=v;percent=fminf(100,p);rate=r;
  if(!started){first=now;started=true;}
  if(!isfinite(r))return;
  if(uint32_t(now-first)<30000){trend=BatteryTrend::Settling;return;}
  if(r>.5f)trend=BatteryTrend::Charging;
  else if(r<-.5f){trend=BatteryTrend::Discharging;hoursLeft=percent/-r;}
  else trend=BatteryTrend::Steady;
 }
 const char *label()const{
  switch(trend){
   case BatteryTrend::Settling:return "CHECKING TREND";
   case BatteryTrend::Charging:return "CHARGING";
   case BatteryTrend::Discharging:return "DISCHARGING";
   case BatteryTrend::Steady:return "STEADY";
   default:return valid?"TREND UNKNOWN":"BATTERY UNKNOWN";
  }
 }
};
