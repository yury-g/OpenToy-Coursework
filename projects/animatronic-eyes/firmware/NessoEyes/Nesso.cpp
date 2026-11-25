// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <M5Unified.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include "Metadata.h"
#include "TouchReaction.h"
#include "Presentation.h"
#include "Idle.h"
#include "NessoMenu.h"
#include "Telemetry.h"
SET_LOOP_TASK_STACK_SIZE(16*1024);
namespace {
using namespace nesso_eyes;
M5Canvas canvas(&M5.Display),hud(&M5.Display);
feather_water::Telemetry telemetry;
Reaction reaction;Pose pose;Idle idle;IdlePose idlePose;Orientation orientation;Transform transform;
NessoMenu ui;unsigned appliedPersona=0,appliedBrightness=1;uint32_t uiEpoch=0;
uint32_t imuSamples=0,imuAt=0;float ax=0,ay=0,az=0;bool imuReady=false;
bool ready=false,down=false;unsigned eyes=0;int touchX=-1,touchY=-1,lastDownX=-1,lastDownY=-1,battery=-1;
uint32_t frames=0,pokes=0,samples=0,readErrors=0,drawMax=0,pushMax=0,hash=0;
uint32_t touchAt=0,skipUntil=0,diagnosticUntil=0;unsigned diagnosticEye=0;
void syncUI(uint32_t now){
 if(ui.epoch==uiEpoch)return;
 uiEpoch=ui.epoch;diagnosticUntil=0;
 if(ui.persona!=appliedPersona){
  appliedPersona=ui.persona;
  const auto& p=feather_face::personality(appliedPersona);
  IdleSettings s;s.seed=p.seed;s.pace=p.pace;s.gazeRange=p.gazeRange;
  s.firstBlink=p.firstBlink;s.winkPeriod=p.winkPeriod;s.blinkStyle=p.id;
  idle=Idle(s);idle.update(now,true);
 }
 if(ui.brightness!=appliedBrightness){
  appliedBrightness=ui.brightness;M5.Display.setBrightness(appliedBrightness?115:45);
 }
}
// Read FT5x06 data with explicit success checks. Failed I2C is a release,
// never cached contact data. Use M5GFX's rotation transform for hit testing.
void sample(){
 uint32_t now=millis();if(int32_t(skipUntil-now)>0){ui.loseInput();down=false;eyes=0;return;}
 if(imuReady&&(M5.Imu.update()&m5::IMU_Class::sensor_mask_accel)){
  auto data=M5.Imu.getImuData();ax=data.accel.x;ay=data.accel.y;az=data.accel.z;auto g=data.gyro;
  ++imuSamples;imuAt=now;orientation.sample(now,ax,ay,az,std::sqrt(g.x*g.x+g.y*g.y+g.z*g.z));
 }
 transform=Transform(orientation.angle(),ui.header);
 uint8_t data[13]={};bool ok=M5.In_I2C.readRegister(0x38,0x02,data,sizeof data,400000);
 bool touched=false;touchX=touchY=-1;
 if(ok){
  unsigned count=data[0]&15;if(count>2)ok=false;
  else for(unsigned i=0;i<count;i++){
   auto* p=data+1+6*i;unsigned event=p[0]>>6;
   if(event==1||event==3)continue;
   m5gfx::touch_point_t point;point.x=((p[0]&15)<<8)|p[1];point.y=((p[2]&15)<<8)|p[3];
   if(point.x>=135||point.y>=240)continue;
   M5.Display.convertRawXY(&point,1);
   touchX=point.x;touchY=point.y;touched=true;
  }
 }
 if(!ok){++readErrors;ui.loseInput();down=false;eyes=0;touchAt=now;++samples;return;}
 NessoTouch nt;nt.down=touched;
 if(touched){nt.x=touchX;nt.y=touchY;lastDownX=touchX;lastDownY=touchY;}
 else{nt.x=lastDownX;nt.y=lastDownY;}
 ui.update(nt,transform,now,true);syncUI(now);
 if(ui.pokeMask&&!down)++pokes;
 down=touched;eyes=ui.pokeMask;touchAt=now;++samples;
}
void draw(){
 uint32_t start=micros(),now=millis();bool fresh=uint32_t(now-touchAt)<100;
 if(!fresh){ui.loseInput();eyes=0;}
 orientation.advance(now);transform=Transform(orientation.angle(),ui.header);
 if(!ui.playing()){
  renderNessoMenuFrame(ui,battery,transform,static_cast<uint16_t*>(canvas.getBuffer()),true);
  uint32_t prepared=micros();canvas.pushSprite(0,0);uint32_t end=micros();
  drawMax=std::max(drawMax,prepared-start);pushMax=std::max(pushMax,end-prepared);++frames;return;
 }
 bool synthetic=int32_t(diagnosticUntil-now)>0;
 unsigned activeMask=synthetic?diagnosticEye:(fresh?eyes:0);
 bool touching=activeMask!=0;
 pose=reaction.update(now,touching,activeMask);
 idlePose=idle.update(now,touching||!ui.playing());
 const auto& personaCfg=feather_face::personality(ui.persona);
 uint8_t mono[1024]={};
 for(unsigned eye=0;eye<2;eye++){
  auto shake=jitter(now,pose.eyeMood[eye],eye);auto eyeFace=idlePose.apply(pose.face,eye);
  for(int y=0;y<32;y++)for(int x=0;x<28;x++){
   bool lit=ui.persona?feather_face::companionPixel(eyeFace,pose.eyeMood[eye],false,eye,x,y,ui.persona)
                      :largePupilPixel(eyeFace,pose.eyeMood[eye],eye,x,y);
   if(lit){
    int px=(eye?78:22)+x+shake.x,py=16+y+shake.y+personaCfg.eyeOffset[eye];
    if(px>=0&&px<128&&py>=0&&py<64)mono[(py/8)*128+px]|=1<<(py%8);
   }
  }
 }
 hash=2166136261U;for(auto b:mono){hash^=b;hash*=16777619U;}
 int hudWidth=int(transform.viewWidth);
 auto* outBuf=static_cast<uint16_t*>(canvas.getBuffer());
 if(ui.header){
  renderNessoHudStrip(static_cast<uint16_t*>(hud.getBuffer()),hudWidth,battery,ui.pressedId()==int(NessoControlId::MenuBtn),true);
  render(mono,outBuf,transform,static_cast<const uint16_t*>(hud.getBuffer()),hudWidth);
 }else{
  render(mono,outBuf,transform,nullptr,0);
 }
 if(ui.persona){
  const uint16_t tint=encodeRgb565(personaCfg.ink,true);
  for(int i=0;i<240*135;i++)if(outBuf[i]==0xffff&&(!ui.header||!transform.header(i%240+.5f,i/240+.5f)))outBuf[i]=tint;
 }
 uint32_t prepared=micros();canvas.pushSprite(0,0);uint32_t end=micros();
 drawMax=std::max(drawMax,prepared-start);pushMax=std::max(pushMax,end-prepared);++frames;
}
void report(){
 if(telemetry.pending())return;
 const auto* app=esp_ota_get_running_partition();
 telemetry.append("EYES app=%s version=%s created=%s build=nesso-eyes-%s ms=%lu frames=%lu hash=%08lx slot=0x%lx battery=%d header=%d\n",appId,version,created,version,(unsigned long)millis(),(unsigned long)frames,(unsigned long)hash,(unsigned long)app->address,battery,ui.header);
 telemetry.append("TOUCH samples=%lu errors=%lu fresh=%d down=%d xy=%d,%d eyes=%u pokes=%lu\n",(unsigned long)samples,(unsigned long)readErrors,uint32_t(millis()-touchAt)<100,down,touchX,touchY,eyes,(unsigned long)pokes);
 telemetry.append("POSE persona=%u mood=%d left=%u right=%u synthetic=%d draw_max_us=%lu push_max_us=%lu heap=%lu\n",ui.persona,pose.mood,pose.face.left,pose.face.right,int32_t(diagnosticUntil-millis())>0,(unsigned long)drawMax,(unsigned long)pushMax,(unsigned long)ESP.getFreeHeap());
 telemetry.append("ORIENT ready=%d fresh=%d angle=%.2f scale=%.3f samples=%lu accel=%.3f,%.3f,%.3f\n",imuReady,imuSamples&&uint32_t(millis()-imuAt)<250,orientation.angle()*360/tau,transform.scale,(unsigned long)imuSamples,ax,ay,az);
 telemetry.append("MENU page=%u root=%u sub=%u brightness=%u epoch=%lu\n",unsigned(ui.page),ui.rootPage,ui.subPage,ui.brightness,(unsigned long)ui.epoch);
}
}
void setup(){
 Serial.setTxBufferSize(2048);Serial.setTxTimeoutMs(0);Serial.begin(115200);
 auto cfg=M5.config();cfg.internal_imu=true;cfg.internal_spk=false;cfg.internal_mic=false;M5.begin(cfg);
 if(M5.getBoard()!=m5::board_t::board_ArduinoNessoN1){telemetry.append("ERR board\n");return;}
 auto* bus=static_cast<lgfx::Bus_SPI*>(M5.Display.panel()->bus());auto config=bus->config();config.dma_channel=0;bus->config(config);
 M5.Display.setRotation(1);M5.Display.setBrightness(115);
 if(auto* touch=M5.Display.touch()){auto config=touch->config();config.bus_shared=false;touch->config(config);}
 if(M5.Display.width()!=width||M5.Display.height()!=height){telemetry.append("ERR geometry\n");return;}
 canvas.setColorDepth(16);hud.setColorDepth(16);if(!canvas.createSprite(width,height)||!hud.createSprite(width,bar)){telemetry.append("ERR framebuffer\n");return;}
 imuReady=M5.Imu.isEnabled();if(imuReady)M5.Imu.setCalibration(0,0,0);
 uint8_t mac[6];esp_read_mac(mac,ESP_MAC_EFUSE_FACTORY);
 telemetry.append("BOOT nesso-eyes-%s created=%s factory=%02x:%02x:%02x:%02x:%02x:%02x touch=0x38 display=240x135\n",version,created,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);ready=true;
}
void loop(){
 telemetry.pump(Serial);if(!ready){delay(100);return;}
 static uint32_t inputAt=0,frameAt=0,batteryAt=millis()-2000,reportAt=0;
 uint32_t now=millis();if(now-inputAt>=10){inputAt=now;sample();if(!ready)return;}
 if(now-batteryAt>=2000){batteryAt=now;battery=M5.Power.getBatteryLevel();}
 if(now-frameAt>=33){frameAt=now;draw();}
 bool requested=false;
 for(unsigned i=0;i<16&&Serial.available();i++){
  int c=Serial.read();if(c=='?')requested=true;
  if(c=='!'){skipUntil=now+1500;ui.loseInput();requested=true;}
  if(c=='N'){ui.openMenu();syncUI(now);requested=true;}
  if(c=='B'){ui.back();syncUI(now);requested=true;}
  if(c=='>'){ui.nextPage();syncUI(now);requested=true;}
  if(c=='H'){ui.togglePerformance();syncUI(now);requested=true;}
  if(c>='0'&&c<='3'){ui.setPersona(unsigned(c-'0'));syncUI(now);requested=true;}
  if((c=='L'||c=='R')&&ui.playing()){diagnosticEye=c=='L'?1:2;diagnosticUntil=now+300;requested=true;}
 }
 // Suppress periodic serial chatter when Performance mode is active unless explicitly queried.
 if(Serial&&(requested||(ui.header&&now-reportAt>=1000))){reportAt=now;report();}
 delay(1);
}
