// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <esp_ota_ops.h>
#include "Board.h"
#include "Round.h"
#include "MenuView.h"
#include "Sound.h"
#include "Telemetry.h"
namespace {
using namespace waveshare_eyes;
constexpr uint32_t touchSampleMs=8;
feather_water::Telemetry telemetry;
feather_face::Companion companion{1};feather_face::CompanionPose pose;
Menu ui;SoundTrigger sounds;
bool ready=false;
unsigned persona=1,poke=0,synthetic=0;
uint32_t uiEpoch=0,renderedEpoch=0xffffffffU;int renderedPressed=-2;unsigned appliedBrightness=1;
Touch touch;
uint32_t frames=0,samples=0,errors=0,pokes=0,hash=0,drawMax=0,touchAt=0,sampleAt=0,skipUntil=0,syntheticUntil=0;
uint32_t touchLifts=0,touchZeroLifts=0;uint8_t lastRelFn=0,lastRelEv=0;int lastRelX=-1,lastRelY=-1;
int battery=-1;uint8_t mono[1024];
void syncUI(uint32_t now){
 if(ui.epoch==uiEpoch)return;
 uiEpoch=ui.epoch;poke=0;syntheticUntil=0;synthetic=0;
 sounds.update(now,0,false);stopSound();
 if(ui.persona!=persona){persona=ui.persona;companion=feather_face::Companion(persona);}
 if(ui.brightness!=appliedBrightness){ui.lightReady=setBrightness(ui.brightness);appliedBrightness=ui.brightness;}
}
void sample(uint32_t now){
 if(uint32_t(now-touchAt)>=100){ui.loseInput();poke=0;syntheticUntil=0;stopSound();}
 if(int32_t(skipUntil-now)>0)return;
 uint8_t bytes[6]={};bool ok=readTouch(bytes);++samples;
 bool valid=touchPacketValid(bytes,ok);if(!valid)++errors;
 auto next=decodeTouch(bytes,valid);if(next.down&&!touch.down)++pokes;
 if((touch.down&&(!next.down||!valid))||(valid&&bytes[1]==1&&(bytes[2]>>6)==1)){
  ++touchLifts;if(valid&&next.x==0&&next.y==0)++touchZeroLifts;
  lastRelFn=bytes[1];lastRelEv=bytes[2]>>6;lastRelX=next.x;lastRelY=next.y;
 }
 touch=next;touchAt=now;ui.update(touch,now,valid);syncUI(now);poke=ui.poke;
 if(!valid){syntheticUntil=0;sounds.update(now,0,false);stopSound();}
}
void serviceTouch(uint32_t now){
 if(!ready||uint32_t(now-sampleAt)<touchSampleMs)return;
 sample(now);sampleAt=millis();
}
void diagnosticTap(int x,int y,uint32_t now){
 // Explicit USB diagnostic only; exercises the same release-to-activate path.
 ui.update({},now);ui.update({true,x,y},now);ui.update({false,x,y},now);
}
void draw(uint32_t now){
 uint32_t start=micros();unsigned mask=uint32_t(now-touchAt)<100?poke:0;
 if(ui.playing()&&int32_t(syntheticUntil-now)>0)mask=synthetic;
 if(!ui.playing())mask=0;
 pose=companion.update(512,512,0,false,now,mask);
 if(auto eye=sounds.update(now,mask,ui.playing()&&audioAvailable()&&!ui.muted))playSound(eye,ui.volume);
 feather_face::drawCompanion(pose,now,mono);hash=2166136261U;for(auto b:mono){hash^=b;hash*=16777619U;}
 auto* buffer=displayBuffer();if(!buffer){ready=false;stopSound();return;}
 const FrameSnapshot frame{ui,battery,persona,feather_face::personality(persona).ink};
 renderedEpoch=ui.epoch;renderedPressed=ui.pressedId();
 bool ok=renderFrameStrips(
  frame,mono,stripRows,buffer,true,
  [](int y,const uint16_t* pixels){return displayStrip(y,pixels);},
  [](int){pumpSound();serviceTouch(millis());}
 );
 if(!ok){ready=false;stopSound();return;}
 ++frames;drawMax=std::max(drawMax,micros()-start);
}
void report(uint32_t now){
 if(telemetry.pending())return;const auto* app=esp_ota_get_running_partition();
 telemetry.append("EYES app=%s version=%s created=%s revision=%s ms=%lu slot=0x%lx ready=%d\n",appId,version,created,revision,(unsigned long)now,(unsigned long)app->address,ready);
 telemetry.append("DISPLAY frames=%lu transfers=%lu errors=%lu hash=%08lx draw_max_us=%lu header=%d battery_mv=%d heap=%lu\n",(unsigned long)frames,(unsigned long)displayTransfers,(unsigned long)displayErrors,(unsigned long)hash,(unsigned long)drawMax,ui.header,battery,(unsigned long)ESP.getFreeHeap());
 telemetry.append("TOUCH samples=%lu errors=%lu fresh=%d down=%d xy=%d,%d eye=%u pokes=%lu lifts=%lu lift0=%lu rel=%u:%u:%d,%d\n",(unsigned long)samples,(unsigned long)errors,uint32_t(now-touchAt)<100,touch.down,touch.x,touch.y,poke,(unsigned long)pokes,(unsigned long)touchLifts,(unsigned long)touchZeroLifts,lastRelFn,lastRelEv,lastRelX,lastRelY);
 telemetry.append("POSE persona=%u mood=%d,%d lids=%u,%u synthetic=%d blinks=%lu,%lu\n",persona,pose.mood[0],pose.mood[1],pose.eye[0].left,pose.eye[1].right,int32_t(syntheticUntil-now)>0,(unsigned long)companion.idle.blinks[0],(unsigned long)companion.idle.blinks[1]);
 telemetry.append("AUDIO ready=%d mute=%d amp=%d plays=%lu errors=%lu mic=off radio=off stage=%d result=%d power=%d raw_mv=%d\n",audioAvailable(),ui.muted,ampEnabled(),(unsigned long)audioPlays,(unsigned long)audioErrors,audioStage,audioResult,codecPower,batteryRaw);
 telemetry.append("MENU page=%u root=%u sub=%u volume=%u brightness=%u light=%d epoch=%lu\n",unsigned(ui.page),ui.rootPage,ui.subPage,ui.volume,ui.brightness,ui.lightReady,(unsigned long)ui.epoch);
}
}
void setup(){
 Serial.setTxBufferSize(2048);Serial.setTxTimeoutMs(0);Serial.begin(115200);
 ready=boardBegin();ui.lightReady=ready;ui.audioReady=ready&&audioBegin();battery=readBattery();
}
void loop(){
 using namespace waveshare_eyes;
 telemetry.pump(Serial);pumpSound();uint32_t now=millis();
 static uint32_t frameAt=0,batteryAt=0;bool requested=false;
 for(unsigned i=0;i<16&&Serial.available();i++){
  int c=Serial.read();if(c=='?')requested=true;
  if(c=='H'){ui.togglePerformance();requested=true;}
  if(c=='N'){ui.openMenu();requested=true;}
  if(c=='B'){ui.back();requested=true;}
  if(c=='>'){ui.next();requested=true;}
  if(c>='a'&&c<='b'){const Rect r=itemRect(c-'a');diagnosticTap(r.x+r.w/2,r.y+r.h/2,now);requested=true;}
  if(c=='M'){ui.toggleMute();requested=true;}
  if(c>='1'&&c<='3'){ui.setPersona(c-'0');requested=true;}
  syncUI(now);
  if((c=='L'||c=='R')&&ui.playing()){synthetic=c=='L'?1:2;syntheticUntil=now+300;requested=true;}
  if(c=='!'){skipUntil=now+1500;touch={};poke=0;syntheticUntil=0;ui.loseInput();sounds.update(now,0,false);stopSound();requested=true;}
 }
 ui.audioReady=audioAvailable();
 if(ready){
  serviceTouch(now);
  if(uint32_t(now-batteryAt)>=2000){batteryAt=now;battery=readBattery();}
  const bool dirty=ui.epoch!=renderedEpoch||ui.pressedId()!=renderedPressed;
  if(dirty||uint32_t(now-frameAt)>=33){frameAt=now;draw(now);}
 }
 if(requested)report(now); // request-only diagnostics; no periodic performance overlays
 delay(1);
}
