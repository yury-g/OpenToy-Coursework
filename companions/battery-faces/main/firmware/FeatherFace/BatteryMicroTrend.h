// SPDX-License-Identifier: MIT
#pragma once
#include "BatteryState.h"
struct BatteryMicroTrend {
 struct Point {uint32_t time;float percent,volts;};
 Point points[61]={};unsigned count=0,next=0;
 float deltaPercent=0,deltaVolts=0;unsigned seconds=0;
 const Point &at(unsigned i)const{return points[((count==61?next:0)+i)%61];}
 bool available()const{return count>=2 && seconds>0;}
 int direction()const{
  if(!available())return 0;
  if(deltaPercent>=.05f && deltaVolts>=.003f)return 1;
  if(deltaPercent<=-.05f && deltaVolts<=-.003f)return -1;
  return 0;
 }
 void update(const BatteryState &b,uint32_t now){
  if(!b.valid){count=next=seconds=0;deltaPercent=deltaVolts=0;return;}
  points[next]={now,b.percent,b.volts};next=(next+1)%61;if(count<61)count++;
  const Point *old=nullptr;
  for(unsigned i=0;i<count;i++)if(uint32_t(now-at(i).time)<=120000){old=&at(i);break;}
  if(!old){seconds=0;deltaPercent=deltaVolts=0;return;}
  seconds=uint32_t(now-old->time)/1000;
  deltaPercent=b.percent-old->percent;deltaVolts=b.volts-old->volts;
 }
};
