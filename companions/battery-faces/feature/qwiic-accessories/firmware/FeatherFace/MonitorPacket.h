// SPDX-License-Identifier: MIT
#pragma once
#include "PeerBattery.h"
struct MonitorPacket {uint32_t magic=0x4d4f4e33,version=1,face=0,sourceAgeMs=0;PeerPacket packet;};
static_assert(sizeof(MonitorPacket)==92,"Monitor wire format");
struct MonitorReceiver {
 PeerBattery peers[FACE_COUNT];
 unsigned liveFaces(unsigned *out,uint32_t now)const{unsigned count=0;for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i)&&peers[i].fresh(now)){if(out)out[count]=i;count++;}return count;}
 bool receive(const void *data,size_t size,uint32_t now){
  if(!data || size!=sizeof(MonitorPacket))return false;
  MonitorPacket p;memcpy(&p,data,size);
  if(p.magic!=0x4d4f4e33 || p.version!=1 || !faceEnabled(p.face) || p.sourceAgeMs>86400000)return false;
  return peers[p.face].receive(FACE_MACS[p.face],&p.packet,sizeof(p.packet),now-p.sourceAgeMs,FACE_MACS[p.face]);
 }
};
