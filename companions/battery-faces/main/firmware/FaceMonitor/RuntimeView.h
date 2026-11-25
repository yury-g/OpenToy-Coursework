// SPDX-License-Identifier: MIT
#pragma once
#include "RuntimeObserver.h"
#include <stdio.h>
#include <new>
// Presentation never feeds synthetic data into the real observer.
struct RuntimeView {
 RuntimeObserver demo;bool racePage=false,demoPage=false,backgroundPage=false,voltage=false;int confirmation=0;uint32_t confirmAt=0,demoStarted=0;
 void startDemo(uint32_t now){
  demo.~RuntimeObserver();new (&demo) RuntimeObserver;
  for(uint32_t t=0;t<=3*3600000U;t+=2000){demo.tick(t);for(unsigned k=0;k<4;k++){
   PeerBattery p;p.seen=true;p.receivedAt=t;p.packet.valid=1;p.packet.seq=t/2000+1;p.packet.uptime=t;p.packet.percent=fmaxf(0,98-(t/3600000.0f)*(18+k*6));p.packet.volts=4.18f-(t/3600000.0f)*(.16f+k*.025f);demo.observe(k,p,t);
  }if(t==0)demo.start(0);}
  demoPage=true;racePage=true;backgroundPage=false;demoStarted=now;confirmation=0;
 }
 static void duration(char *out,size_t size,uint32_t sec){snprintf(out,size,"%luh %02lum",(unsigned long)(sec/3600),(unsigned long)(sec/60%60));}
 template<class Canvas>void header(Canvas &c){
  c.fillRect(0,0,1280,66,0);c.setTextSize(3);c.setTextColor(0xffff);c.setCursor(18,20);c.print("BATTERY FACES");
  const char *names[]={"FACES","RACE","LOG","DEMO"};for(unsigned n=0;n<4;n++){int x=490+n*185;bool sel=n==0?!racePage:n==1?racePage&&!demoPage&&!backgroundPage:n==2?backgroundPage&&!demoPage:demoPage;c.fillRoundRect(x,10,175,46,8,sel?0x234a:0x1082);c.setCursor(x+22,24);c.setTextSize(2);c.setTextColor(0xffff);c.print(names[n]);}
 }
 template<class Canvas>void draw(Canvas &c,const RuntimeObserver &real,uint32_t now,bool persisted){
  const RuntimeObserver &r=demoPage?demo:real;const RuntimeSnapshot &shown=backgroundPage?r.background:r.state;uint32_t readNow=demoPage?3*3600000U:now;
  c.fillScreen(0);header(c);c.setTextSize(2);c.setCursor(22,78);c.setTextColor(demoPage?0xffe0:0xffff);
  char elapsed[24];duration(elapsed,sizeof(elapsed),r.elapsed());
  if(demoPage)c.printf("DEMO / SIMULATED 3 HOURS - live recording continues separately");
  else if(backgroundPage)c.print("CONTINUOUS BACKGROUND LOG - records on every page");
  else if(r.state.phase==RuntimeObserver::WAITING)c.printf("BACKGROUND LOG / WAITING FOR CHARGE: %u/4 at 90%%+",r.ready(now));
  else c.printf("%s / session %lu / elapsed %s",r.state.phase==RuntimeObserver::RUNNING?"RACE RUNNING":r.state.phase==RuntimeObserver::STOPPED?"RACE STOPPED":"RACE INTERRUPTED - RESTART REQUIRED",(unsigned long)r.state.session,elapsed);
  c.setTextSize(1);c.setCursor(23,103);c.setTextColor(0x9cf3);c.print(demoPage?"Synthetic curves are never saved as measurements.":persisted?"Saved locally every 5 min. USB export available. Boards identify devices, not swapped packs.":"Live RAM log. Local checkpoint not yet saved / unavailable; status has details.");
  // Shared axes: time since first retained sample or race start. No joins across gaps.
  const int gx=74,gy=140,gw=1120,gh=230;uint32_t first=shown.phase==RuntimeObserver::WAITING?0:shown.startSeconds;
  uint32_t end=(shown.phase==RuntimeObserver::STOPPED||shown.phase==RuntimeObserver::INTERRUPTED)?shown.stopSeconds:shown.clockSeconds;uint32_t span=std::max<uint32_t>(60,end-first);
  for(unsigned n=0;n<=4;n++){int y=gy+n*gh/4;c.drawLine(gx,y,gx+gw,y,0x2104);c.setCursor(8,y-5);c.setTextSize(2);c.setTextColor(0x9cf3);if(voltage)c.printf("%.2f",4.3f-n*.3f);else c.printf("%u%%",100-n*25);}
  for(unsigned k=0;k<4;k++){auto &s=shown.tracks[k];int px=0,py=0;bool prior=false;for(unsigned n=0;n<s.count;n++){
   const auto &p=s.points[n];if(p.seconds<first)continue;int x=gx+uint64_t(p.seconds-first)*gw/span;float y=voltage?(4.3f-RuntimeObserver::volts(p))/1.2f:(100-p.centipercent*.01f)/100;int iy=gy+std::max(0,std::min(gh,int(y*gh)));
   if(prior&&!RuntimeObserver::gap(p)){c.drawLine(px,py,x,iy,FACE_INKS[RuntimeObserver::IDS[k]]);c.drawLine(px,py+1,x,iy+1,FACE_INKS[RuntimeObserver::IDS[k]]);}else c.fillCircle(x,iy,3,FACE_INKS[RuntimeObserver::IDS[k]]);px=x;py=iy;prior=true;
  }}
  c.setTextSize(2);c.setTextColor(0x9cf3);c.setCursor(74,383);c.print("0");c.setCursor(480,383);c.print(voltage?"VOLTAGE / SAME AXES":"CHARGE ESTIMATE / SAME AXES");c.setCursor(1095,383);duration(elapsed,sizeof(elapsed),span);c.print(elapsed);
  for(unsigned k=0;k<4;k++){unsigned i=RuntimeObserver::IDS[k];const auto &s=shown.tracks[k];int y=425+k*48;bool fresh=r.fresh(k,readNow);c.setTextSize(2);c.setTextColor(FACE_INKS[i]);c.setCursor(26,y);c.print(faceTitle(i));
   // Colored miniature smile, preserving the larger faces on the Faces tab.
   c.drawCircle(184,y+7,15,FACE_INKS[i]);c.fillCircle(179,y+3,2,FACE_INKS[i]);c.fillCircle(189,y+3,2,FACE_INKS[i]);c.drawLine(179,y+12,189,y+12,FACE_INKS[i]);
   c.setTextColor(0xffff);c.setCursor(218,y);if(lroundf(s.lastVolts*1000)>0)c.printf(voltageEstimated(i)?"~%3.0f%%  %.3fV":" %3.0f%%  %.3fV",s.lastPercent,s.lastVolts);else c.print(" --%  -.---V");
   c.setCursor(435,y);auto rate=r.rate(k,readNow);if(rate.valid)c.printf("%+6.1f pp/h",rate.pph);else c.print("rate: learning");
   c.setCursor(660,y);float eta=backgroundPage?-1:r.etaMinutes(k,readNow);if(eta>=0)c.printf("~%4.0fm to 10%%",eta);else c.print("to 10%: --");
   c.setCursor(875,y);duration(elapsed,sizeof(elapsed),shown.phase==RuntimeObserver::WAITING?s.observedSeconds:s.runObservedSeconds);c.print(elapsed);
   c.setCursor(1055,y);c.setTextColor(!fresh||s.lowAt||s.interrupted?0xfd20:0x9cf3);if(s.lowAt){if(backgroundPage)c.print("LOW / RECHARGE");else{char low[24];duration(low,sizeof(low),s.lowAt);c.printf("LOW %s",low);}}else c.print(!fresh?"LINK LOST":s.interrupted?"RESET / GAP":r.state.phase==RuntimeObserver::WAITING?"OBSERVING":"LIVE");
  }
  c.setTextSize(1);c.setTextColor(0x9cf3);c.setCursor(26,620);c.print("Rate = last continuous 5m. ~ = voltage SOC. Remaining time is tentative, not capacity or battery health.");
  c.setCursor(26,637);c.setTextColor(r.linkLost(readNow)?0xfd20:0x9cf3);c.print(r.linkLost(readNow)?"BLUE GATEWAY LOST: every other radio reading may be unavailable. Silence never proves an empty battery.":"LOW = <=3.30V for 60s. Recharge at LOW; no automatic cutoff. Charging state / USB removal are not sensed here.");
  c.fillRoundRect(22,662,300,46,8,0x234a);c.setTextSize(2);c.setTextColor(0xffff);c.setCursor(38,677);c.print(voltage?"SHOW CHARGE %":"SHOW VOLTAGE");
  c.fillRoundRect(348,662,905,46,8,confirmation?0x8200:0x234a);c.setCursor(365,677);
  if(demoPage)c.print("EXIT DEMO - SHOW LIVE DATA");
  else if(backgroundPage)c.print("SHOW RACE - background log continues separately");
  else if(confirmation==1)c.print("UNPLUG FOUR USBs, THEN TAP TO START (Tab5 on USB)");
  else if(confirmation==2)c.print("TAP AGAIN TO STOP - KEEP RECORDED CURVES");
  else if(r.state.phase==RuntimeObserver::RUNNING)c.print("STOP RACE");
  else c.print(r.ready(now)==4?"START NEW RACE (all four 90%+)":"WAITING FOR ALL FOUR TO REACH 90%+");
 }
 // Same hit test for physical touch release and integration test; no synthetic source injection.
 // Returns1 when an explicit real race state change needs a checkpoint.
 int tap(int x,int y,uint32_t now,RuntimeObserver &r){
  if(y<66){confirmation=0;if(x>=1045){startDemo(now);}else if(x>=860){racePage=true;demoPage=false;backgroundPage=true;}else if(x>=675){racePage=true;demoPage=false;backgroundPage=false;}else if(x>=490){racePage=false;demoPage=false;backgroundPage=false;}return 0;}
  if(!racePage||y<662)return 0;
  if(x<330){voltage=!voltage;return 0;}
  if(x<348)return 0;if(backgroundPage){backgroundPage=false;return 0;}if(demoPage){demoPage=false;confirmation=0;return 0;}
  if(confirmation&&uint32_t(now-confirmAt)>120000)confirmation=0;
  if(confirmation==1){confirmation=0;return r.start(now,true)?1:0;}
  if(confirmation==2){confirmation=0;r.stop();return 1;}
  if(r.state.phase==RuntimeObserver::RUNNING)confirmation=2;else if(r.ready(now)==4)confirmation=1;confirmAt=now;return 0;
 }
};
