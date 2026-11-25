// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
constexpr uint8_t BLUE_MAC[6]={0xb8,0xf8,0x62,0xd5,0x8b,0x8c};
constexpr uint8_t GREEN_MAC[6]={0xb8,0xf8,0x62,0xd5,0x80,0x14};
constexpr uint8_t ORANGE_MAC[6]={0x48,0xf6,0xee,0xc4,0xd2,0xbc};
constexpr unsigned FACE_COUNT=8;
constexpr uint8_t YELLOW_MAC[6]={0xe8,0x06,0x90,0x93,0xfc,0x50};
constexpr uint8_t RAINBOW_MAC[6]={0xe8,0x06,0x90,0x93,0xfc,0xf8};
// Green is retired after a new board-heat report. Preserve its wire index.
inline bool faceEnabled(unsigned i){return i<FACE_COUNT && i!=1;}
inline bool voltageEstimated(unsigned i){return i==3 || i==4 || i==6;}
constexpr uint8_t PURPLE_MAC[6]={0xd4,0xe9,0xf4,0xf3,0xc5,0xa0};
inline bool hasBatteryMeasurement(unsigned i){return i!=5 && i!=7;}
constexpr uint8_t PINK_MAC[6]={0x14,0xc1,0x9f,0xd5,0x97,0x8c};
constexpr uint8_t TEAL_MAC[6]={0x10,0xb4,0x1d,0xd2,0xa2,0xc0};
constexpr const uint8_t *FACE_MACS[FACE_COUNT]={BLUE_MAC,GREEN_MAC,ORANGE_MAC,YELLOW_MAC,RAINBOW_MAC,PURPLE_MAC,PINK_MAC,TEAL_MAC};
constexpr const char *FACE_NAMES[FACE_COUNT]={"blue","green","orange","yellow","rainbow","purple","pink","teal"};
constexpr uint16_t FACE_INKS[FACE_COUNT]={0x041f,0x07e0,0xfd20,0xffe0,0xf81f,0xa01f,0xfc18,0x05d7};
// Same ESP32 firmware on both ends; fixed-width, naturally aligned wire format.
// MAC filtering selects the expected board; this telemetry is not encrypted/authenticated.
struct PeerPacket {
 uint32_t magic=0x46414345,version=3,seq=0,uptime=0,valid=0;
 float percent=0,volts=0,deltaPercent=0,deltaVolts=0;
 uint32_t seconds=0,warm=0,evidence=0,windowMilliseconds=0;
 float delta5Percent=0,delta5Volts=0;
 uint32_t currentValid=0;float currentMilliAmps=0;
 uint32_t chargerState=0,diagnosticAge=0; // 0 unknown,1 charging,2 discharging,3 charger done,4 idle
};
static_assert(sizeof(PeerPacket)==76 && sizeof(PeerPacket)<250,"ESP-NOW packet size");
struct PeerBattery {
 PeerPacket packet;uint32_t receivedAt=0,count=0;bool seen=false;
 bool receive(const uint8_t *source,const void *data,size_t size,uint32_t now,const uint8_t *expected=BLUE_MAC){
  if(!source || !data || !expected || !memcmp(source,GREEN_MAC,6) || memcmp(source,expected,6) || size!=sizeof(PeerPacket))return false;
  PeerPacket p;memcpy(&p,data,sizeof(p));
  if(p.magic!=0x46414345 || p.version!=3 || p.valid>1 || p.warm>1 || p.seconds>120 || p.evidence>6 || p.windowMilliseconds>420000)return false;
  if(!isfinite(p.percent)||!isfinite(p.volts)||!isfinite(p.deltaPercent)||!isfinite(p.deltaVolts))return false;
  if(p.valid && (p.percent<0 || p.percent>100 || p.volts<2 || p.volts>4.5f))return false;
  if(!isfinite(p.delta5Percent)||!isfinite(p.delta5Volts)||fabsf(p.delta5Percent)>100||fabsf(p.delta5Volts)>2.5f)return false;
  if(p.currentValid>1 || p.chargerState>4 || !isfinite(p.currentMilliAmps) || fabsf(p.currentMilliAmps)>3000)return false;
  if(fabsf(p.deltaPercent)>100 || fabsf(p.deltaVolts)>2.5f)return false;
  packet=p;receivedAt=now;seen=true;count++;return true;
 }
 bool fresh(uint32_t now)const{return seen && uint32_t(now-receivedAt)<8000;}
};

inline const char *faceTitle(unsigned i){constexpr const char *n[]={"BLUE","GREEN","ORANGE","YELLOW","RAINBOW","PURPLE","PINK","TEAL"};return i<FACE_COUNT?n[i]:"UNKNOWN";}
