// SPDX-License-Identifier: MIT
#pragma once
#include "FindState.h"
// Include after serviceMonitor(). All actuator and UDP work stays in loop context.
// Host application supplies these three functions and calls the four hooks below.
bool accessoryFindPulse(uint32_t now);
void accessoryFindStop();
bool accessoryFindCapable();
uint32_t accessoryIoFlags(uint32_t now);
accessory::FindNode findNode;
accessory::AccessoryPeer findGreen;
struct FindInbox {uint8_t bytes[sizeof(accessory::Status)];uint32_t size=0,arrived=0;};
FindInbox findInbox[8];unsigned findRead=0,findWrite=0,findCount=0;
bool findInitialized=false,findHasStatus=false,findDirty=true,findGreenDirty=false;uint32_t findLastStatus=0,findLastRelay=0,findLastIo=0;
bool findReceiveEspNow(const uint8_t *source,const void *data,size_t size,uint32_t now){
 if(!source||!data)return false;
 bool match=false;
 if(localFace()==0&&size==sizeof(accessory::Status)&&!memcmp(source,GREEN_MAC,6)){
  accessory::Status s;memcpy(&s,data,size);match=accessory::valid(s)&&s.target==1&&s.sourceAgeMs==0;
 }else if(localFace()==1&&size==sizeof(accessory::Command)&&!memcmp(source,BLUE_MAC,6)){
  accessory::Command c;memcpy(&c,data,size);match=accessory::valid(c)&&c.target==1;
 }
 if(!match)return false;
 portENTER_CRITICAL(&peerMux);
 if(findCount<8){auto &in=findInbox[findWrite];memcpy(in.bytes,data,size);in.size=size;in.arrived=now;findWrite=(findWrite+1)%8;findCount++;}
 portEXIT_CRITICAL(&peerMux);return true;
}
void findSendUdp(const accessory::Status &s){
 if(!gatewayReady||!WiFi.softAPgetStationNum())return;
 monitorUdp.beginPacket(IPAddress(192,168,4,2),MONITOR_PORT);monitorUdp.write((const uint8_t*)&s,sizeof(s));monitorUdp.endPacket();
}
bool findActive(){return findInitialized&&findNode.active;}
bool findLocalAcknowledge(){
 if(!findNode.acknowledge())return false;
 accessoryFindStop();findDirty=true;return true;
}
void findApplyLocal(const accessory::Command &c,uint32_t now){
 bool was=findNode.active;bool applied=findNode.apply(c,now,accessoryFindCapable());
 if((was&&!findNode.active)||(applied&&c.op==(uint32_t)accessory::Op::Stop))accessoryFindStop();
 findDirty=true;
}
void findService(uint32_t now){
 if(localFace()>1||isRetiredGreenDevice())return;
 if(!findInitialized){findNode.begin(localFace(),esp_random());findInitialized=true;}
 for(unsigned n=0;n<8;n++){
  FindInbox in;bool have=false;
  portENTER_CRITICAL(&peerMux);if(findCount){in=findInbox[findRead];findRead=(findRead+1)%8;findCount--;have=true;}portEXIT_CRITICAL(&peerMux);
  if(!have)break;
  if(in.size==sizeof(accessory::Command)){accessory::Command c;memcpy(&c,in.bytes,sizeof(c));findApplyLocal(c,now);}
  else{accessory::Status s;memcpy(&s,in.bytes,sizeof(s));if(findGreen.receive(s,in.arrived))findGreenDirty=true;}
 }
 if(localFace()==0&&gatewayReady){
  // Only the dedicated Tab5 private-AP endpoint may route control. This is not cryptographic authentication.
  for(unsigned n=0;n<4;n++){
   int size=monitorUdp.parsePacket();if(size<=0)break;accessory::Command c;
   bool ok=size==(int)sizeof(c)&&monitorUdp.remoteIP()==IPAddress(192,168,4,2)&&monitorUdp.read((uint8_t*)&c,sizeof(c))==(int)sizeof(c)&&accessory::valid(c);
   if(!ok){while(monitorUdp.available())monitorUdp.read();continue;}
   if(c.target==0)findApplyLocal(c,now);
   else if(findGreen.seen&&(findGreen.fresh(now)||c.op==(uint32_t)accessory::Op::Stop)&&c.targetBoot==findGreen.status.targetBoot&&radioReady&&!radioPaused)esp_now_send(GREEN_MAC,(const uint8_t*)&c,sizeof(c));
  }
 }
 if(findNode.tick(now,accessoryFindCapable())){accessoryFindStop();findDirty=true;}
 if(findNode.pulseDue(now)){
  if(accessoryFindPulse(now))findNode.pulsed(now);
  else if(!accessoryFindCapable()){findNode.close(accessory::Reason::NotReady);accessoryFindStop();findDirty=true;}
  // A busy/quiet actuator stays leased and retries; lease and session deadlines still cap it.
 }
 uint32_t io=accessoryIoFlags(now);if(io!=findLastIo){findLastIo=io;findDirty=true;}
 if(!findHasStatus||(findDirty&&uint32_t(now-findLastStatus)>=100)||uint32_t(now-findLastStatus)>=250){
  findHasStatus=true;findLastStatus=now;auto status=findNode.status(now,accessoryFindCapable());
  status.flags|=io;
  if(localFace()==0)findSendUdp(status);
  else if(radioReady&&!radioPaused)esp_now_send(BLUE_MAC,(const uint8_t*)&status,sizeof(status));
  findDirty=false;
 }
 if(localFace()==0&&findGreen.seen&&((findGreenDirty&&uint32_t(now-findLastRelay)>=50)||uint32_t(now-findLastRelay)>=250)){
  findLastRelay=now;findGreenDirty=false;auto status=findGreen.status;status.sourceAgeMs=findGreen.age(now);findSendUdp(status);
 }
}
