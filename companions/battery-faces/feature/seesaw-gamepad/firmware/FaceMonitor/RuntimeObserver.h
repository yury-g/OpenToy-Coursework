// SPDX-License-Identifier: MIT
#pragma once
#include "../FeatherFace/PeerBattery.h"
#include <algorithm>
// Passive observer. No load, power, charger or battery-test commands.
struct RuntimePoint {uint32_t seconds=0;uint16_t millivolts=0,centipercent=0;};
struct RuntimeTrack {
 RuntimePoint points[97];
 uint32_t interval=60,lastPlot=0,observedSeconds=0,runObservedSeconds=0,lastContact=0,lowAt=0,resets=0;
 uint16_t count=0;uint8_t interrupted=0,reserved=0;
 float lastPercent=0,lastVolts=0;
};
struct RuntimeSnapshot {
 uint32_t magic=0x52554e31,version=1,clockSeconds=0,phase=0,session=0,startSeconds=0,stopSeconds=0;
 RuntimeTrack tracks[4];
};
static_assert(sizeof(RuntimeSnapshot)<4600,"NVS snapshot stays bounded");
struct RuntimeRate {bool valid=false;float pph=0,voltsPerHour=0;uint32_t span=0;};
struct RuntimeObserver {
 enum Phase:uint32_t {WAITING,RUNNING,STOPPED,INTERRUPTED};
 static constexpr unsigned IDS[4]={0,1,6,8};
 RuntimeSnapshot state,background;
 struct Live {
  RuntimePoint recent[32];uint16_t count=0;
  uint32_t sourceMillis=0,seenMillis=0,seq=0,uptime=0,lowSince=0,lastRecent=0,observedRemainder=0,runRemainder=0,bgLowSince=0;
  bool seen=false,valid=false,gap=true,lowPending=false,bgLowPending=false;
 } live[4];
 uint32_t lastTick=0,remainder=0,runStartedMillis=0;bool ticked=false;
 void tick(uint32_t now){if(!ticked){lastTick=now;ticked=true;return;}uint32_t delta=now-lastTick;lastTick=now;uint64_t sum=uint64_t(remainder)+delta;state.clockSeconds+=sum/1000;background.clockSeconds=state.clockSeconds;remainder=sum%1000;}
 bool fresh(unsigned k,uint32_t now)const{return k<4&&live[k].valid&&uint32_t(now-live[k].sourceMillis)<8000;}
 bool linkLost(uint32_t now)const{return !fresh(0,now);}
 unsigned ready(uint32_t now)const{unsigned n=0;for(unsigned k=0;k<4;k++)if(fresh(k,now)&&state.tracks[k].lastPercent>=90)n++;return n;}
 static RuntimePoint point(uint32_t sec,float pct,float v,bool gap){return {sec,uint16_t(uint16_t(lroundf(v*1000))|uint16_t(gap?0x8000:0)),uint16_t(lroundf(pct*100))};}
 static float volts(const RuntimePoint &p){return (p.millivolts&0x7fff)/1000.0f;}
 static bool gap(const RuntimePoint &p){return p.millivolts&0x8000;}
 static void addPlotTo(RuntimeTrack &s,uint32_t sec,float pct,float v,bool broken){if(s.count&&sec-s.lastPlot<s.interval&&!broken)return;
  if(s.count==97){
   // Keep first, then every second sample; carry gaps over discarded samples.
   for(unsigned n=1;n<=48;n++){bool broken=gap(s.points[n*2-1]);s.points[n]=s.points[n*2];if(broken)s.points[n].millivolts|=0x8000;}
   s.count=49;s.interval*=2;
  }
  s.points[s.count++]=point(sec,pct,v,broken);s.lastPlot=sec;
 }
 void addPlot(unsigned k,uint32_t sec,float pct,float v,bool broken){addPlotTo(state.tracks[k],sec,pct,v,broken);}
 void observe(unsigned k,const PeerBattery &peer,uint32_t now){
  if(k>=4)return;auto &l=live[k];auto &s=state.tracks[k];const auto &p=peer.packet;
  if(!peer.fresh(now)||!p.valid){l.valid=false;l.count=0;l.lowPending=false;l.bgLowPending=false;l.gap=true;return;}
  if(l.seen&&int32_t(peer.receivedAt-l.sourceMillis)<0)return;
  if(l.seen&&p.seq==l.seq&&p.uptime==l.uptime)return;
  uint32_t sec=state.clockSeconds-std::min(state.clockSeconds,uint32_t(now-peer.receivedAt)/1000);
  bool reset=l.seen&&p.uptime<l.uptime;
  uint32_t dt=l.seen?uint32_t(peer.receivedAt-l.sourceMillis):0;
  bool continuous=l.valid&&l.seen&&!reset&&dt<8000;
  bool broken=l.gap||!continuous;
  if(reset){s.resets++;if(state.phase==RUNNING)s.interrupted=1;}
  if(!continuous){l.count=0;l.lowPending=false;l.bgLowPending=false;}
  if(continuous){l.observedRemainder+=dt;s.observedSeconds+=l.observedRemainder/1000;l.observedRemainder%=1000;if(state.phase==RUNNING&&!s.lowAt&&!s.interrupted){uint32_t runDt=dt;if(int32_t(l.sourceMillis-runStartedMillis)<0)runDt=int32_t(peer.receivedAt-runStartedMillis)>0?uint32_t(peer.receivedAt-runStartedMillis):0;l.runRemainder+=runDt;s.runObservedSeconds+=l.runRemainder/1000;l.runRemainder%=1000;}}
  s.lastContact=sec;s.lastPercent=p.percent;s.lastVolts=p.volts;
  auto &bg=background.tracks[k];bg.lastContact=sec;bg.lastPercent=p.percent;bg.lastVolts=p.volts;bg.observedSeconds=s.observedSeconds;bg.resets=s.resets;addPlotTo(bg,sec,p.percent,p.volts,broken);background.clockSeconds=state.clockSeconds;
  if(p.volts<=3.30f){if(!l.bgLowPending){l.bgLowPending=true;l.bgLowSince=sec;}else if(sec-l.bgLowSince>=60)bg.lowAt=sec?sec:1;}else {l.bgLowPending=false;if(p.volts>3.40f)bg.lowAt=0;}
  if(state.phase==WAITING||state.phase==RUNNING){if(!s.lowAt)addPlot(k,sec,p.percent,p.volts,broken);}
  if(!l.count||sec-l.lastRecent>=10){
   if(l.count==32){for(unsigned n=1;n<32;n++)l.recent[n-1]=l.recent[n];l.count--;}
   l.recent[l.count++]=point(sec,p.percent,p.volts,broken);l.lastRecent=sec;
  }
  if(state.phase==RUNNING&&!s.lowAt&&!s.interrupted){
   if(p.volts<=3.30f){if(!l.lowPending){l.lowPending=true;l.lowSince=sec;}else if(sec-l.lowSince>=60)s.lowAt=sec-state.startSeconds;}
   else l.lowPending=false;
  }
  l.sourceMillis=peer.receivedAt;l.seenMillis=now;l.seq=p.seq;l.uptime=p.uptime;l.seen=true;l.valid=true;l.gap=false;
 }
 RuntimeRate rate(unsigned k,uint32_t now)const{
  RuntimeRate out;if(!fresh(k,now))return out;const auto &l=live[k];if(l.count<2)return out;
  // Only the latest continuous five-minute span; least squares reduces ADC/gauge noise.
  uint32_t newest=l.recent[l.count-1].seconds;unsigned first=0;
  for(unsigned n=0;n<l.count;n++){if(newest-l.recent[n].seconds>=300)first=n;if(gap(l.recent[n])&&n>first)first=n;}
  if(first>=l.count-1)return out;out.span=newest-l.recent[first].seconds;if(out.span<300)return out;
  double sx=0,sy=0,sv=0,sxx=0,sxy=0,sxv=0;unsigned count=l.count-first;
  for(unsigned n=first;n<l.count;n++){double x=l.recent[n].seconds-l.recent[first].seconds,y=l.recent[n].centipercent*.01,v=volts(l.recent[n]);sx+=x;sy+=y;sv+=v;sxx+=x*x;sxy+=x*y;sxv+=x*v;}
  double den=count*sxx-sx*sx;if(den<=0)return out;out.valid=true;out.pph=3600*(count*sxy-sx*sy)/den;out.voltsPerHour=3600*(count*sxv-sx*sv)/den;return out;
 }
 float etaMinutes(unsigned k,uint32_t now)const{
  if(k>=4||state.phase!=RUNNING||state.clockSeconds-state.startSeconds<120||state.tracks[k].interrupted||state.tracks[k].lowAt)return -1;
  auto r=rate(k,now);if(!r.valid||r.pph>=-.2f||state.tracks[k].lastPercent<=10)return -1;
  float minutes=(state.tracks[k].lastPercent-10)*60/-r.pph;return minutes<=10080?minutes:-1;
 }
 bool start(uint32_t now,bool chargedAtArm=false){
  if(state.phase==RUNNING)return false;
  if(chargedAtArm){for(unsigned k=0;k<4;k++)if(!fresh(k,now))return false;}else if(ready(now)!=4)return false;
  runStartedMillis=now;state.phase=RUNNING;state.session++;state.startSeconds=state.clockSeconds;state.stopSeconds=0;
  for(unsigned k=0;k<4;k++){auto &s=state.tracks[k];s.count=0;s.interval=60;s.lowAt=0;s.interrupted=0;s.runObservedSeconds=0;live[k].count=0;live[k].runRemainder=0;live[k].lowPending=false;live[k].gap=true;addPlot(k,state.clockSeconds,s.lastPercent,s.lastVolts,true);}
  return true;
 }
 void stop(){if(state.phase==RUNNING){state.phase=STOPPED;state.stopSeconds=state.clockSeconds;}}
 static bool validSnapshot(const RuntimeSnapshot &saved){
  if(saved.magic!=0x52554e31||saved.version!=1||saved.phase>INTERRUPTED||saved.startSeconds>saved.clockSeconds||saved.stopSeconds>saved.clockSeconds)return false;
  for(const auto &s:saved.tracks){
   if(s.count>97||s.interval<60||s.interval>86400*365U||!isfinite(s.lastPercent)||s.lastPercent<0||s.lastPercent>100||!isfinite(s.lastVolts)||s.lastVolts<0||s.lastVolts>4.5f||s.interrupted>1||s.lastContact>saved.clockSeconds||s.lowAt>saved.clockSeconds-saved.startSeconds)return false;
   for(unsigned n=0;n<s.count;n++)if(s.points[n].seconds>saved.clockSeconds||s.points[n].centipercent>10000||(s.points[n].millivolts&0x7fff)>4500||(n&&s.points[n].seconds<s.points[n-1].seconds))return false;
  }
  return true;
 }
 bool restore(const RuntimeSnapshot &saved){
  if(!validSnapshot(saved))return false;state=saved;if(state.phase==RUNNING){state.phase=INTERRUPTED;state.stopSeconds=state.clockSeconds;}
  for(auto &l:live)l=Live{};ticked=false;remainder=0;return true;
 }
 bool restoreBackground(const RuntimeSnapshot &saved){if(!validSnapshot(saved)||saved.phase!=WAITING)return false;background=saved;state.clockSeconds=std::max(state.clockSeconds,saved.clockSeconds);for(unsigned k=0;k<4;k++){state.tracks[k].observedSeconds=std::max(state.tracks[k].observedSeconds,saved.tracks[k].observedSeconds);state.tracks[k].resets=std::max(state.tracks[k].resets,saved.tracks[k].resets);}return true;}
 uint32_t elapsed()const{return state.phase==WAITING?state.clockSeconds:(state.phase==RUNNING?state.clockSeconds:state.stopSeconds)-state.startSeconds;}
};
