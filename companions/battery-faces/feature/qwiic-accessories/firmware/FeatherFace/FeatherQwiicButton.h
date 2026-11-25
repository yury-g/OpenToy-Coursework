// SPDX-License-Identifier: MIT
#pragma once
#include <Wire.h>
#include "QwiicButtonState.h"
#include "QwiicButtonLed.h"
// Register protocol: SparkFun_Qwiic_Button_Arduino_Library/src/registers.h.
// Only status and volatile LED configuration; never program address/NVM.
struct FeatherQwiicButton {
 static constexpr uint8_t ADDRESS=0x6f,ID=0x5d;
 QwiicButtonState state;bool present=false,ready=false,lit=true;
 uint8_t id=0,led=0,fwMajor=0,fwMinor=0;uint32_t lastPoll=0,lastProbe=0,presses=0,errors=0;
 bool read(uint8_t reg,uint8_t *out,uint8_t size=1){Wire.beginTransmission(ADDRESS);Wire.write(reg);if(Wire.endTransmission()!=0||Wire.requestFrom(ADDRESS,size)!=size)return false;for(unsigned n=0;n<size;n++)out[n]=Wire.read();return true;}
 bool light(uint8_t brightness){
  Wire.beginTransmission(ADDRESS);Wire.write(0x19);for(uint8_t v:{brightness,uint8_t(1),uint8_t(0),uint8_t(0),uint8_t(0),uint8_t(0)})Wire.write(v);
  if(Wire.endTransmission()!=0){errors++;return false;}uint8_t check[6];if(!read(0x19,check,6)||check[0]!=brightness||check[1]!=1||check[2]||check[3]||check[4]||check[5]){errors++;return false;}led=brightness;return true;
 }
 bool begin(uint32_t now){lastProbe=now;present=read(0,&id)&&id==ID;ready=false;if(!present){state.update(false,0,now);return false;}uint8_t s;if(!read(1,&fwMinor)||!read(2,&fwMajor)||!read(3,&s))return false;state.update(true,s,now);ready=light(lit?(state.pressed?128:16):0);return ready;}
 bool service(uint32_t now){
  if(!ready){if(uint32_t(now-lastProbe)>=1000)begin(now);return false;}
  if(uint32_t(now-lastPoll)<20)return false;lastPoll=now;uint8_t status;
  if(!read(3,&status)){present=ready=false;state.update(false,0,now);errors++;lastProbe=now;return false;}
  bool old=state.pressed,event=state.update(true,status,now);if(old!=state.pressed)ready=light(lit?(state.pressed?128:16):0);
  if(event)presses++;return event;
 }
 void animate(uint8_t command,bool active,bool breathing=false){if(!ready)return;uint8_t value=qwiicButtonBrightness(lit,state.pressed,command,active,breathing);if(value!=led)ready=light(value);}
 void report(){uint8_t actual=0;bool ok=read(0x19,&actual);Serial.printf("QWIIC_BUTTON present=%d ready=%d address=6f id=%02x firmware=%u.%u pressed=%d led=%u led_readback=%u io=%d presses=%lu errors=%lu\n",present,ready,id,fwMajor,fwMinor,state.pressed,led,actual,ok,(unsigned long)presses,(unsigned long)errors);}
};
