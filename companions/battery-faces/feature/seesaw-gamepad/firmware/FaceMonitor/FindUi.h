// SPDX-License-Identifier: MIT
#pragma once
#include "FindController.h"
#include "../FeatherFace/TileLayout.h"
struct FindUi {
 accessory::FindController control;
 template<class Udp>void service(Udp &udp,uint32_t now){
  for(unsigned i=0;i<2;i++){accessory::Command command;if(control.next(i,now,command)){udp.beginPacket(IPAddress(192,168,4,1),MONITOR_PORT+1);udp.write((const uint8_t*)&command,sizeof(command));udp.endPacket();}}
 }
 template<class Udp>bool oneShotLease(Udp &udp,unsigned i,uint32_t now){
  accessory::Command command;bool accepted=control.oneShotLease(i,now,command);
  if(accepted){udp.beginPacket(IPAddress(192,168,4,1),MONITOR_PORT+1);udp.write((const uint8_t*)&command,sizeof(command));udp.endPacket();}
  Serial.printf("FIND_ONCE accepted=%d target=%u lease_ms=6000 target_boot=%lu sequence=%lu session=%lu\n",accepted,i,(unsigned long)command.targetBoot,(unsigned long)command.sequence,(unsigned long)command.session);return accepted;
 }
 static bool stopHit(int x,int y){return x>=18&&x<470&&y>=10&&y<56;}
 bool tap(int x,int y,uint32_t now,bool facesPage,const MonitorReceiver &receiver){
  if(control.busy(now)&&stopHit(x,y)){control.stopAll(now);return true;}
  if(!facesPage||y<72)return false;
  unsigned active[FACE_COUNT],count=receiver.liveFaces(active,now);
  for(unsigned n=0;n<count;n++){
   if(active[n]>1)continue;auto r=tileRect(n,count);float scale=fminf(r.w/616.0f,r.h/304.0f);int w=616*scale,h=304*scale,left=r.x+(r.w-w)/2,top=r.y+(r.h-h)/2;
   if(x>=left&&x<left+w&&y>=top&&y<top+h)return control.toggle(active[n],now);
  }return false;
 }
 struct Activity {bool known=false,buttonReady=false,buttonDown=false,recentPress=false,outline=false,outlineActive=false;unsigned drive=0;const char *mode="unknown";char text[40]="ACCESSORY --";};
 Activity activity(unsigned i,uint32_t now)const{
  Activity out;if(i>1)return out;const auto &p=control.peers[i];
  if(!p.fresh(now)){if(p.seen)snprintf(out.text,sizeof(out.text),"ACCESSORY OFFLINE");return out;}
  uint32_t flags=p.status.flags;if(!accessory::ioKnown(flags))return out;
  out.outline=flags&accessory::OUTLINE_VISIBLE;out.outlineActive=out.outline&&(flags&accessory::OUTLINE_ACTIVE);
  out.known=true;out.mode=accessory::activityMode(flags);out.drive=accessory::ioDrive(flags);
  out.buttonReady=flags&accessory::BUTTON_READY;out.buttonDown=out.buttonReady&&(flags&accessory::BUTTON_DOWN);
  out.recentPress=out.buttonReady&&(flags&accessory::RECENT_PRESS)&&p.age(now)<1500;
  const char *mode=flags&accessory::BREATHING?"BREATH":flags&accessory::HELD?"HOLD":flags&accessory::MOTOR_ACTIVE?"BUZZ":"IDLE";
  const char *button=!out.buttonReady?"--":out.buttonDown?"DOWN":out.recentPress?"PRESS":"UP";
  snprintf(out.text,sizeof(out.text),"%s%u / Q:%s",mode,out.drive,button);return out;
 }
 template<class Canvas>void card(Canvas &c,unsigned i,uint32_t now){
  if(i>1)return;auto &p=control.peers[i];auto phase=control.requests[i].phase;
  bool active=phase!=accessory::FindController::Idle||(p.fresh(now)&&(p.status.flags&accessory::FINDING));
  c.setTextSize(2);c.setCursor(20,48);c.setTextColor(active?0xffe0:p.locatable(now)?0x07e0:0x8410);
  c.print(phase==accessory::FindController::Stopping?"STOPPING BUZZ":active?"FINDING / TAP TO STOP":p.locatable(now)?"TAP FACE TO FIND":"FIND UNAVAILABLE");
  auto io=activity(i,now);if(io.known&&io.outline){uint16_t ink=io.outlineActive?FACE_INKS[i]:0x4208;c.drawRoundRect(16,74,270,134,18,ink);c.drawRoundRect(17,75,268,132,17,ink);}
  c.fillRect(24,212,256,22,0);c.setTextSize(2);c.setCursor(24,215);c.setTextColor(io.known?0xffff:0x8410);c.print(io.text);
  c.fillRect(24,234,256,4,0x2104);if(io.known&&io.drive)c.fillRect(24,234,256*io.drive/127,4,FACE_INKS[i]);
 }
 template<class Canvas>void overlay(Canvas &c,uint32_t now){
  if(!control.busy(now))return;
  c.fillRoundRect(18,10,452,46,8,0xb800);c.setTextSize(3);c.setTextColor(0xffff);c.setCursor(44,22);c.print("STOP BUZZ");
 }
 void report(uint32_t now){
  for(unsigned i=0;i<2;i++){auto &p=control.peers[i];auto &s=p.status;auto io=activity(i,now);Serial.printf("ACCESSORY color=%s seen=%d fresh=%d ready=%d finding=%d pending=%u age_ms=%lu boot=%lu status_seq=%lu ack_seq=%lu session=%lu reason=%lu remaining_ms=%lu io_known=%d mode=%s drive=%u button_down=%d recent_press=%d outline=%d outline_active=%d\n",FACE_NAMES[i],p.seen,p.fresh(now),p.locatable(now),p.fresh(now)&&bool(s.flags&accessory::FINDING),(unsigned)control.requests[i].phase,(unsigned long)(p.seen?p.age(now):0),(unsigned long)s.targetBoot,(unsigned long)s.sequence,(unsigned long)s.ackSequence,(unsigned long)s.session,(unsigned long)s.reason,(unsigned long)s.remainingMs,io.known,io.mode,io.drive,io.buttonDown,io.recentPress,io.outline,io.outlineActive);}
 }
};
