// SPDX-License-Identifier: MIT
#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "PeripheralProtocol.h"
#include "FrameRetry.h"
#include "EyePixels.h"
class Peripherals {
 Adafruit_SH1106G display{128,64,&Wire,-1};
 uint32_t sampledAt=0,drawnAt=0;
 bool dirty=true;uint8_t drawnMask=255;
 FrameRetry retry;
 bool transfer(const uint8_t *b,size_t n){Wire.beginTransmission(0x3c);size_t written=Wire.write(b,n);uint8_t e=Wire.endTransmission();if(e||written!=n){errors++;return false;}return true;}
public:
 PeripheralState state;bool initialized=false,i2cOK=false;uint32_t raw[4]={},hash=0,frames=0,errors=0;uint8_t mask=0;
 bool begin(){
  Wire.begin(5,6);Wire.setClock(400000);Wire.setTimeOut(25);
  Wire.beginTransmission(0x3c);if(Wire.endTransmission()!=0)return false;
  initialized=display.begin(0x3c,true)&&display.getBuffer();Wire.setClock(400000);
  if(initialized){sample();render();}return initialized&&i2cOK;
 }
 void markDirty(){dirty=true;}
 void sample(){mask=0;for(unsigned i=0;i<4;i++){raw[i]=touchRead(i+1);if(state.touch&&raw[i]>state.threshold)mask|=1<<i;}sampledAt=millis();}
 void render(){
  if(!initialized)return;
  display.clearDisplay();
  if(state.screen==TEST_SCREEN){display.drawRect(0,0,128,64,1);display.drawLine(0,0,127,63,1);display.drawCircle(64,32,20,1);}
  if(state.screen==EYES||state.screen==CLOSED){int dx=state.face.x*10/100+(mask&8?6:0)-(mask&1?6:0),dy=(state.face.pupils?eyeWhiteY(state.face)-16:state.face.y*6/100)+(mask&4?4:0)-(mask&2?4:0);
   for(unsigned eye=0;eye<2;eye++){
    int x=(eye?78:22)+dx,y=16+dy;unsigned lid=eye?state.face.right:state.face.left;
    if(state.screen==CLOSED||(!state.face.pupils&&lid==100)){display.drawLine(x,y+16,x+28,y+16,1);continue;}
    if(state.face.pupils){
     for(int py=0;py<32;py++)for(int px=0;px<28;px++)if(eyeLayerPixel(state.face,eye,px,py))display.drawPixel(x+px,y+py,1);
     continue;
    }
    if(state.face.expression==SURPRISED)display.fillCircle(x+14,y+16,14,1);
    else display.fillRoundRect(x,y,28,32,8,1);
    if(state.face.expression==HAPPY)display.fillRect(x,y,28,16,0);
    if(state.face.expression==ANGRY){
     if(eye)display.fillTriangle(x,y,x+28,y,x,y+12,0);
     else display.fillTriangle(x,y,x+28,y,x+28,y+12,0);
    }
    if(lid)display.fillRect(x,y,28,32*lid/100,0);
   }
  }
  uint8_t config[]={0,0x81,(uint8_t)state.contrast,(uint8_t)(state.screen==SCREEN_OFF?0xae:0xaf)};bool ok=transfer(config,sizeof(config));
  uint8_t *buffer=display.getBuffer();hash=2166136261UL;for(unsigned i=0;i<1024;i++){hash^=buffer[i];hash*=16777619UL;}
  // Full SH1106 frame, page column offset 2 as in the pinned Adafruit driver.
  // Check every command/data transmission; the library's display() returns void.
  for(unsigned p=0;p<8&&ok;p++){uint8_t command[]={0,(uint8_t)(0xb0+p),0x10,0x02};ok=transfer(command,4);
   for(unsigned x=0;x<128&&ok;x+=16){uint8_t packet[17];packet[0]=0x40;memcpy(packet+1,buffer+p*128+x,16);ok=transfer(packet,17);}}
  i2cOK=ok;if(ok)frames++;drawnAt=millis();retry.completed(ok,drawnAt);dirty=!ok;if(ok)drawnMask=mask;
 }
 void tick(bool updating){if(updating)return;if(millis()-sampledAt>=100)sample();if(retry.allowed(millis())&&(dirty||(mask!=drawnMask&&millis()-drawnAt>=200)))render();}
};
