// SPDX-License-Identifier: MIT
#pragma once
#include <Wire.h>
#include "GamepadFace.h"
// Checked seesaw register transactions follow Adafruit's Mini_I2C_Gamepad_QT
// example and Adafruit_seesaw register protocol. Never reset Wire/the seesaw,
// write EEPROM/address registers or enable the optional wired IRQ.
struct FeatherGamepad {
 GamepadFace controls;
 bool ready=false;uint8_t address=0x50,hardware=0;uint16_t product=0,rawX=512,rawY=512;
 uint32_t version=0,high=GamepadFace::MASK,lastProbe=0,lastPoll=0,lastGood=0,samples=0,errors=0;
 bool read(uint8_t module,uint8_t reg,uint8_t *out,uint8_t count,uint16_t wait=125){
  Wire.beginTransmission(address);Wire.write(module);Wire.write(reg);
  if(Wire.endTransmission()!=0)return false;delayMicroseconds(wait);
  if(Wire.requestFrom(address,count)!=count){while(Wire.available())Wire.read();return false;}
  for(unsigned i=0;i<count;i++)out[i]=Wire.read();return true;
 }
 bool writeMask(uint8_t reg){
  uint32_t mask=GamepadFace::MASK;Wire.beginTransmission(address);Wire.write(1);Wire.write(reg);
  for(int shift=24;shift>=0;shift-=8)Wire.write(uint8_t(mask>>shift));return Wire.endTransmission()==0;
 }
 bool begin(uint32_t now){
  lastProbe=now;ready=false;controls.update(false,0,0,0,now);product=0;hardware=0;
  for(address=0x50;address<=0x53;address++){
   uint8_t data[4];if(!read(0,1,&hardware,1)||hardware<0x84||hardware>0x89||!read(0,2,data,4))continue;
   version=uint32_t(data[0])<<24|uint32_t(data[1])<<16|uint32_t(data[2])<<8|data[3];product=version>>16;
   if(product!=5743)continue; // Do not write to unrelated seesaw devices.
   if(!writeMask(3)||!writeMask(0x0b)||!writeMask(5)){errors++;return false;}
   ready=true;return true;
  }
  address=0x50;return false;
 }
 bool service(uint32_t now){
  if(!ready){if(uint32_t(now-lastProbe)>=1000)begin(now);return false;}
  if(uint32_t(now-lastPoll)<25)return false;lastPoll=now;uint8_t gpio[4],x[2],y[2];
  bool ok=read(1,4,gpio,4)&&read(9,0x07+14,x,2,500)&&read(9,0x07+15,y,2,500);
  if(ok){rawX=uint16_t(x[0])<<8|x[1];rawY=uint16_t(y[0])<<8|y[1];ok=rawX<=1023&&rawY<=1023;}
  if(!ok){ready=false;errors++;lastProbe=now;controls.update(false,0,0,0,now);return false;}
  high=uint32_t(gpio[0])<<24|uint32_t(gpio[1])<<16|uint32_t(gpio[2])<<8|gpio[3];lastGood=now;samples++;
  return controls.update(true,rawX,rawY,high,now);
 }
 void report(){Serial.printf("GAMEPAD ready=%d address=%02x hw=%02x product=%u samples=%lu errors=%lu age_ms=%lu raw_x=%u raw_y=%u x=%d y=%d buttons=%08lx manual=%d mood=%s look_x=%.2f look_y=%.2f presses=%u\n",ready,address,hardware,product,(unsigned long)samples,(unsigned long)errors,(unsigned long)(samples?uint32_t(millis()-lastGood):0),rawX,rawY,controls.axisX,controls.axisY,(unsigned long)(~high&GamepadFace::MASK),controls.manual,expressionName(controls.expression),controls.lookX,controls.lookY,controls.presses);}
};
