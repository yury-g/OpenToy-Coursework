// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <esp_ota_ops.h>
#include "Board.h"
#include "Round.h"
#include "Status.h"
#include "Sound.h"
#include "Telemetry.h"
namespace {
using namespace waveshare_eyes;
feather_water::Telemetry telemetry;
feather_face::Companion companion{1};feather_face::CompanionPose pose;
HoldControl hold;SoundTrigger sounds;
bool ready=false,soundReady=false,visible=true,muted=false;
unsigned persona=1,poke=0,synthetic=0;
Touch touch;
uint32_t frames=0,samples=0,errors=0,pokes=0,hash=0,drawMax=0,touchAt=0,skipUntil=0,syntheticUntil=0;
int battery=-1;uint8_t mono[1024];
void personality(unsigned id){persona=id;companion=feather_face::Companion(id);}
void action(Action a){
 if(a==Action::Header)visible=!visible;
 if(a==Action::Mute){muted=!muted;if(muted)stopSound();}
 if(a==Action::Personality)personality(persona%3+1);
}
void sample(uint32_t now){
 if(int32_t(skipUntil-now)>0)return;
 uint8_t bytes[6]={};bool ok=readTouch(bytes);++samples;if(!ok)++errors;
 auto next=decodeTouch(bytes,ok);if(next.down&&!touch.down)++pokes;
 touch=next;touchAt=now;action(hold.update(touch,now));poke=touch.down?eyeAt(touch.x,touch.y):0;
}
void draw(uint32_t now){
 uint32_t start=micros();unsigned mask=uint32_t(now-touchAt)<100?poke:0;
 if(int32_t(syntheticUntil-now)>0)mask=synthetic;
 pose=companion.update(512,512,0,false,now,mask);
 if(auto eye=sounds.update(now,mask,audioAvailable()&&!muted))playSound(eye);
 feather_face::drawCompanion(pose,now,mono);hash=2166136261U;for(auto b:mono){hash^=b;hash*=16777619U;}
 auto* buffer=displayBuffer();if(!buffer){ready=false;stopSound();return;}
 uint16_t color=feather_face::personality(persona).ink;
 for(int y=0;y<360;y+=stripRows){
  for(int r=0;r<stripRows;r++){
   auto* row=buffer+r*360;renderRow(mono,y+r,row,color);renderStatusRow(y+r,row,visible,battery,persona,muted);
   for(int x=0;x<360;x++)row[x]=(row[x]>>8)|(row[x]<<8); // RGB565 big-endian wire order
  }
  if(!displayStrip(y,buffer)){ready=false;stopSound();return;}
 }
 ++frames;drawMax=std::max(drawMax,micros()-start);
}
void report(uint32_t now){
 if(telemetry.pending())return;const auto* app=esp_ota_get_running_partition();
 telemetry.append("EYES app=%s version=%s created=%s revision=%s ms=%lu slot=0x%lx ready=%d\n",appId,version,created,revision,(unsigned long)now,(unsigned long)app->address,ready);
 telemetry.append("DISPLAY frames=%lu transfers=%lu errors=%lu hash=%08lx draw_max_us=%lu header=%d battery_mv=%d heap=%lu\n",(unsigned long)frames,(unsigned long)displayTransfers,(unsigned long)displayErrors,(unsigned long)hash,(unsigned long)drawMax,visible,battery,(unsigned long)ESP.getFreeHeap());
 telemetry.append("TOUCH samples=%lu errors=%lu fresh=%d down=%d xy=%d,%d eye=%u pokes=%lu\n",(unsigned long)samples,(unsigned long)errors,uint32_t(now-touchAt)<100,touch.down,touch.x,touch.y,poke,(unsigned long)pokes);
 telemetry.append("POSE persona=%u mood=%d,%d lids=%u,%u synthetic=%d blinks=%lu,%lu\n",persona,pose.mood[0],pose.mood[1],pose.eye[0].left,pose.eye[1].right,int32_t(syntheticUntil-now)>0,(unsigned long)companion.idle.blinks[0],(unsigned long)companion.idle.blinks[1]);
 telemetry.append("AUDIO ready=%d mute=%d amp=%d plays=%lu errors=%lu mic=off radio=off stage=%d result=%d power=%d raw_mv=%d\n",audioAvailable(),muted,ampEnabled(),(unsigned long)audioPlays,(unsigned long)audioErrors,audioStage,audioResult,codecPower,batteryRaw);
}
}
void setup(){
 Serial.setTxBufferSize(2048);Serial.setTxTimeoutMs(0);Serial.begin(115200);
 ready=boardBegin();if(ready)soundReady=audioBegin();battery=readBattery();
}
void loop(){
 using namespace waveshare_eyes;
 telemetry.pump(Serial);pumpSound();uint32_t now=millis();
 static uint32_t sampleAt=0,frameAt=0,batteryAt=0;bool requested=false;
 for(unsigned i=0;i<16&&Serial.available();i++){
  int c=Serial.read();if(c=='?')requested=true;
  if(c=='H'){action(Action::Header);requested=true;}
  if(c=='M'){action(Action::Mute);requested=true;}
  if(c>='1'&&c<='3'){personality(c-'0');requested=true;}
  if(c=='L'||c=='R'){synthetic=c=='L'?1:2;syntheticUntil=now+300;requested=true;}
  if(c=='!'){skipUntil=now+1500;touch={};poke=0;hold.update({},now);requested=true;}
 }
 if(ready){
  if(uint32_t(now-sampleAt)>=10){sampleAt=now;sample(now);}
  if(uint32_t(now-batteryAt)>=2000){batteryAt=now;battery=readBattery();}
  if(uint32_t(now-frameAt)>=33){frameAt=now;draw(now);}
 }
 if(requested)report(now); // request-only diagnostics; no periodic performance overlays
 delay(1);
}
