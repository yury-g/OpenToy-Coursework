// SPDX-License-Identifier: MIT
#include "Board.h"
#include "Menu.h"
#include "Battery.h"
#include "NativeEyes.h"
#include "Metadata.h"
#include "Companion.h"
#include "Telemetry.h"
#include <esp_ota_ops.h>
#include <esp_system.h>
namespace {
using namespace rlcd_eyes;
Canvas canvas;Key key;Menu menu;
feather_face::Companion companion{1};feather_face::CompanionPose pose;
feather_water::Telemetry telemetry;
uint8_t last[frameBytes];bool haveLast=false,ready=false;
unsigned persona=1,physical=0,physicalPulse=0,synthetic=0;int battery=-1;
uint32_t until=0,frames=0,skipped=0,hash=0,pokes=0,drawMaxUs=0;
void selectPersonality(unsigned id){persona=id;companion=feather_face::Companion(id);auto settings=feather_face::Companion::settings(id);settings.seed^=esp_random();companion.idle=nesso_eyes::Idle(settings);}
void applyKey(KeyEvent event,uint32_t now,bool injected){
 const Page before=menu.page;auto action=menu.input(event);
 if(before!=menu.page){physicalPulse=synthetic=0;until=0;}
 if(action.personality)selectPersonality(action.personality);
 if(action.poke){++pokes;if(injected)synthetic=action.poke;else physicalPulse=action.poke;until=now+300;}
}
void report(uint32_t now){
 if(telemetry.pending())return;const auto* app=esp_ota_get_running_partition();
 telemetry.append("APP id=%s version=%s created=%s revision=%s target=%s slot=0x%lx ms=%lu\n",appId,version,created,revision,target,(unsigned long)app->address,(unsigned long)now);
 telemetry.append("DISPLAY ready=%d width=400 height=300 mono=1 renderer=native-curves polarity=black header=%d frames=%lu transfers=%lu skipped=%lu hash=%08lx draw_max_us=%lu transfer_max_us=%lu\n",ready,menu.headerVisible,(unsigned long)frames,(unsigned long)transfers,(unsigned long)skipped,(unsigned long)hash,(unsigned long)drawMaxUs,(unsigned long)transferMaxUs);
 telemetry.append("INPUT physical=%u synthetic=%u pokes=%lu persona=%u mood=%d,%d lids=%u,%u blinks=%lu,%lu\n",physical,int32_t(until-now)>0?synthetic:0,(unsigned long)pokes,persona,pose.mood[0],pose.mood[1],pose.eye[0].left,pose.eye[1].right,(unsigned long)companion.idle.blinks[0],(unsigned long)companion.idle.blinks[1]);
 telemetry.append("BOARD battery_mv=%d source=ADC1_CH3x3 charge=unknown i2c_mask=%u heap=%lu psram=%lu reset=%d audio=off radio=off\n",battery,peripheralMask,(unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getPsramSize(),int(esp_reset_reason()));
 char volts[8];const uint32_t age=millis()-batteryUpdatedMs;formatBattery(visibleBattery(battery,age),volts);
 telemetry.append("CONTROLS profile=key-menu menu=%u key_gpio=18 boot=reserved\n",unsigned(menu.page));
 telemetry.append("BATTERY display=%s pin_mv=%d calibrated=%d samples=%lu errors=%lu age_ms=%lu max_read_us=%lu percent=unknown\n",volts,batteryPinMv,batteryCalibrated(),(unsigned long)batterySamples,(unsigned long)batteryErrors,(unsigned long)age,(unsigned long)batteryReadMaxUs);
}
}
void setup(){
 Serial.setTxBufferSize(2048);Serial.setTxTimeoutMs(0);Serial.begin(115200);
 // Per-instance idle randomness keeps several toys from blinking in lockstep.
 selectPersonality(1);
 ready=boardBegin();battery=readBattery();
}
void loop(){
 telemetry.pump(Serial);const uint32_t now=millis();bool requested=false;
 if(int32_t(until-now)<=0)physicalPulse=synthetic=0;
 for(unsigned i=0;i<16&&Serial.available();i++){
  const int c=Serial.read();
  if(c=='?')requested=true;
  if(c=='H'){menu.open(Page::Play);menu.headerVisible=!menu.headerVisible;physicalPulse=synthetic=0;until=0;requested=true;}
  if(c>='1'&&c<='3'){selectPersonality(c-'0');requested=true;}
  if(c=='k'||c=='K'){applyKey(c=='k'?KeyEvent::Short:KeyEvent::Hold,now,true);requested=true;}
  if((c=='L'||c=='R')&&menu.page==Page::Play){synthetic=c=='L'?1:2;until=now+300;requested=true;}
 }
 physical=digitalRead(18)==LOW;applyKey(key.update(physical,now),now,false);
 const unsigned mask=menu.page==Page::Play&&int32_t(until-now)>0?(physicalPulse|synthetic):0;
 // Input/reaction timing advances between display transfers.
 pose=companion.update(512,512,0,false,now,mask);
 static uint32_t frameAt=0,batteryAt=0;
 if(uint32_t(now-batteryAt)>=2000){batteryAt=now;battery=readBattery();}
 if(ready&&uint32_t(now-frameAt)>=50){
  frameAt=now;uint32_t start=micros();
  const int shownBattery=visibleBattery(battery,millis()-batteryUpdatedMs);
  if(menu.page==Page::Play)renderNative(pose,now,canvas.bytes,menu.headerVisible);
  else memset(canvas.bytes,0,frameBytes); // Menu replaces the scene; clear packed bytes once.
  drawHeader(canvas,menu.headerVisible,shownBattery,persona);drawMenu(canvas,menu,shownBattery);
  hash=2166136261U;for(auto b:canvas.bytes){hash^=b;hash*=16777619U;}
  if(!haveLast||memcmp(last,canvas.bytes,frameBytes)){
   present(canvas.bytes);memcpy(last,canvas.bytes,frameBytes);haveLast=true;
  }else ++skipped;
  ++frames;drawMaxUs=std::max(drawMaxUs,micros()-start);
 }
 if(requested)report(now);
 delay(1);
}
