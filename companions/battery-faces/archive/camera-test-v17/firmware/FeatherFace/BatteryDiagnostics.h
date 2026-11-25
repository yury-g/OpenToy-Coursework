// SPDX-License-Identifier: MIT
#pragma once
#include "BatteryState.h"
#include "DoodleProtocol.h"
inline FaceState batteryPose(const BatteryState &b) {
 FaceState s=restingDoodle();
 if(!b.valid){s.expression=NERVOUS;return s;}
 if(b.percent>70){s.expression=HAPPY;s.jaw=15;}
 else if(b.percent>=45){s.expression=NEUTRAL;s.mouthBend=25;}
 else if(b.percent>=20){s.expression=SORRY;s.left=s.right=15;}
 else{s.expression=ANGRY;s.left=s.right=40;}
 return s;
}
enum class ChargeEvidence {Unknown,Learning,Gaining,Losing,Holding,Unclear,NearFull};
struct BatteryDiagnostics {
 struct Point {uint32_t time;float percent,volts;};
 Point points[8]={};unsigned count=0,next=0;
 bool started=false,warmReady=false,windowReady=false,hasPoint=false;
 uint32_t first=0,lastPoint=0,windowMilliseconds=0;
 float deltaPercent=0,deltaVolts=0;
 ChargeEvidence evidence=ChargeEvidence::Unknown;
 void update(const BatteryState &b,uint32_t now){
  if(!b.valid){started=warmReady=windowReady=hasPoint=false;count=next=0;evidence=ChargeEvidence::Unknown;return;}
  if(!started){started=true;first=now;}
  warmReady=uint32_t(now-first)>=120000;
  evidence=ChargeEvidence::Learning;windowReady=false;
  if(!warmReady)return;
  if(!hasPoint||uint32_t(now-lastPoint)>=60000){points[next]={now,b.percent,b.volts};next=(next+1)%8;if(count<8)count++;lastPoint=now;hasPoint=true;}
  const Point *old=nullptr;uint32_t best=0xffffffffU;
  for(unsigned i=0;i<count;i++){uint32_t age=now-points[i].time;if(age>=300000 && age<=420000 && age<best){old=&points[i];best=age;}}
  if(!old)return;
  windowReady=true;windowMilliseconds=best;deltaPercent=b.percent-old->percent;deltaVolts=b.volts-old->volts;
  if(b.percent>=95 && b.volts>=4.15f && deltaPercent>=-.5f && deltaPercent<=1 && fabsf(deltaVolts)<.025f)evidence=ChargeEvidence::NearFull;
  else if(deltaPercent>=.7f && deltaVolts>=.01f)evidence=ChargeEvidence::Gaining;
  else if(deltaPercent<=-.7f && deltaVolts<=-.005f)evidence=ChargeEvidence::Losing;
  else if(fabsf(deltaPercent)<.7f && fabsf(deltaVolts)<.02f)evidence=ChargeEvidence::Holding;
  else evidence=ChargeEvidence::Unclear;
 }
 const char *label()const{
  switch(evidence){
   case ChargeEvidence::Learning:return "LEARNING";
   case ChargeEvidence::Gaining:return "GAINING";
   case ChargeEvidence::Losing:return "LOSING";
   case ChargeEvidence::Holding:return "HOLDING";
   case ChargeEvidence::NearFull:return "NEAR FULL";
   case ChargeEvidence::Unclear:return "UNCLEAR";
   default:return "UNKNOWN";
  }
 }
 float hoursLeft(const BatteryState &b)const{return windowReady&&evidence==ChargeEvidence::Losing?b.percent/-deltaPercent*(windowMilliseconds/3600000.f):-1;}
 float hoursToFull(const BatteryState &b)const{return windowReady&&evidence==ChargeEvidence::Gaining?(100-b.percent)/deltaPercent*(windowMilliseconds/3600000.f):-1;}
};
enum class TestKind:uint32_t {Watch=1,Runtime=2};
enum class TestOutcome:uint32_t {Empty,Running,Stopped,LowStop,Interrupted,UsbStop};
struct SessionRecord {
 uint32_t version=1;TestKind kind=TestKind::Watch;TestOutcome outcome=TestOutcome::Empty;
 uint32_t elapsedSeconds=0;float startPercent=0,endPercent=0,startVolts=0,endVolts=0,minVolts=0;
};
struct BatterySession {
 SessionRecord record;uint32_t started=0;
 bool running()const{return record.outcome==TestOutcome::Running;}
 bool start(TestKind kind,const BatteryState &b,uint32_t now){
  if(!b.valid || (kind==TestKind::Runtime && (b.percent<25 || b.volts<3.6f)))return false;
  record=SessionRecord{};record.kind=kind;record.outcome=TestOutcome::Running;record.startPercent=record.endPercent=b.percent;record.startVolts=record.endVolts=record.minVolts=b.volts;started=now;return true;
 }
 void update(const BatteryState &b,uint32_t now,bool usbHost=false){
  if(!running())return;
  record.elapsedSeconds=uint32_t(now-started)/1000;
  if(record.kind==TestKind::Runtime && usbHost){record.outcome=TestOutcome::UsbStop;return;}
  if(!b.valid){record.outcome=TestOutcome::Interrupted;return;}
  record.endPercent=b.percent;record.endVolts=b.volts;record.minVolts=fminf(record.minVolts,b.volts);
  if(record.kind==TestKind::Runtime && (b.percent<=20 || b.volts<=3.5f))record.outcome=TestOutcome::LowStop;
 }
 void stop(){if(running())record.outcome=TestOutcome::Stopped;}
 void restore(const SessionRecord &r){
  if(r.version!=1 || (r.kind!=TestKind::Watch && r.kind!=TestKind::Runtime) || (unsigned)r.outcome>(unsigned)TestOutcome::UsbStop || !isfinite(r.startPercent)||!isfinite(r.endPercent)||!isfinite(r.startVolts)||!isfinite(r.endVolts)||!isfinite(r.minVolts))return;
  record=r;if(running())record.outcome=TestOutcome::Interrupted;
 }
 float usedPercent()const{return record.startPercent-record.endPercent;}
};
