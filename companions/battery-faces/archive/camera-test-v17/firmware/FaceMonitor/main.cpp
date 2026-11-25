// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "../FeatherFace/MonitorPacket.h"
#include "../FeatherFace/FaceLinkConfig.h"
#include "../FeatherFace/BatteryDiagnostics.h"
#include "../FeatherFace/BatteryMicroTrend.h"
#include "../FeatherFace/CachedFace.h"
#include "../FeatherFace/FaceDisplay.h"
#include "../FeatherFace/TileLayout.h"
#include "../FeatherFace/CameraProtocol.h"
#include "../FeatherFace/CameraView.h"
#include "CameraTcpReceiver.h"
#include "TabCameraSource.h"
#include "../FeatherFace/FaceNavigation.h"
#ifndef ARDUINO_M5STACK_TAB5
#error Select M5Tab5
#endif
const char BUILD[]="BatteryMonitor-v17-tab5-camera";
class TabDisplay:public M5GFX {
 protected:bool init_impl(bool reset,bool clear)override{
  // This board is ROM/owner-confirmed; do not reuse an old NVS board hint.
  _board=autodetect(reset,m5gfx::board_t::board_M5Tab5);
  return _board==m5gfx::board_t::board_M5Tab5 && lgfx::LGFX_Device::init_impl(false,clear);
 }
};
TabDisplay display;M5Canvas canvas(&display),tile(&display);WiFiUDP udp;
facecamera::FrameAssembler cameraFrames[2]={facecamera::FrameAssembler(0),facecamera::FrameAssembler(1)};CameraView cameraView;SwipeGesture cameraSwipe;
uint8_t cameraPixels[facecamera::FRAME_BYTES];uint32_t cameraFallbacks=0;

MonitorReceiver receiver,ingress;portMUX_TYPE rxMux=portMUX_INITIALIZER_UNLOCKED;
CameraTcpReceiver cameraReceiver(cameraFrames[0],rxMux);
TabCameraSource tabCamera(cameraFrames[1],rxMux);
unsigned cameraFresh(){portENTER_CRITICAL(&rxMux);uint32_t now=millis();unsigned mask=(cameraFrames[0].fresh(now)?1:0)|(cameraFrames[1].fresh(now)?2:0);portEXIT_CRITICAL(&rxMux);return mask;}
BatteryMicroTrend histories[FACE_COUNT];CachedFace faces[FACE_COUNT];
uint32_t priorSequence[FACE_COUNT]={},priorUptime[FACE_COUNT]={},received=0,ingressCount=0,lastDraw=0,lastReport=0,lastConnect=0;
uint32_t panelDiagMask=0,lastPanelTry=0;
bool panel=false;char command[80]={};unsigned used=0;
void copyIngress(){portENTER_CRITICAL(&rxMux);receiver=ingress;received=ingressCount;portEXIT_CRITICAL(&rxMux);}
void receiveTask(void*){
 for(;;){
  int size=udp.parsePacket();
  if(size>0){MonitorPacket packet;uint32_t arrived=millis();
   if(size==(int)sizeof(packet)&&udp.remoteIP()==IPAddress(192,168,4,1)&&udp.read((uint8_t*)&packet,sizeof(packet))==(int)sizeof(packet)){
    portENTER_CRITICAL(&rxMux);if(ingress.receive(&packet,sizeof(packet),arrived))ingressCount++;portEXIT_CRITICAL(&rxMux);
   }else while(udp.available())udp.read();
  }else vTaskDelay(pdMS_TO_TICKS(2));
 }
}
void initializePanel(){
 lastPanelTry=millis();lgfx::i2c::release(1);
 pinMode(31,INPUT_PULLUP);pinMode(32,INPUT_PULLUP);delay(2);
 if(!digitalRead(31)&&digitalRead(32)){
  pinMode(32,OUTPUT_OPEN_DRAIN);digitalWrite(32,HIGH);
  for(unsigned n=0;n<9;n++){digitalWrite(32,LOW);delayMicroseconds(5);digitalWrite(32,HIGH);delayMicroseconds(5);}
  pinMode(31,OUTPUT_OPEN_DRAIN);digitalWrite(31,LOW);delayMicroseconds(5);digitalWrite(32,HIGH);delayMicroseconds(5);digitalWrite(31,HIGH);
  pinMode(31,INPUT_PULLUP);pinMode(32,INPUT_PULLUP);
 }
 lgfx::i2c::init(1,31,32);panelDiagMask=0;
 if(lgfx::i2c::readRegister8(1,0x43,1).has_value())panelDiagMask|=1;
 if(lgfx::i2c::readRegister8(1,0x44,1).has_value())panelDiagMask|=2;
 const uint8_t regs[]={3,7,13,11,9,17,5};for(unsigned n=0;n<7;n++)if(lgfx::i2c::readRegister8(1,0x44,regs[n],100000).has_value())panelDiagMask|=4<<n;
 panel=display.init()&&display.width()>0&&display.height()>0;
 if(panel){display.setRotation(1);display.setBrightness(100);display.fillScreen(0);}
}
const char *chargeLabel(unsigned s){return s==1?"CHARGING":s==2?"DISCHARGING":s==3?"CHARGER DONE":s==4?"IDLE":"UNKNOWN";}
void report(){
 copyIngress();
 Serial.printf("MONITOR build=%s connected=%d width=%d height=%d packets=%lu uptime_ms=%lu ip=%s panel=%d tiles=%u i2c_mask=%lu sda=%d scl=%d\n",BUILD,WiFi.status()==WL_CONNECTED,display.width(),display.height(),(unsigned long)received,(unsigned long)millis(),WiFi.localIP().toString().c_str(),panel,receiver.liveFaces(nullptr,millis()),(unsigned long)panelDiagMask,digitalRead(31),digitalRead(32));
 for(unsigned source=0;source<2;source++){
  portENTER_CRITICAL(&rxMux);const auto &f=cameraFrames[source];uint32_t now=millis(),count=f.completedFrames(),seq=f.sequence(),age=now-f.completedAt(),rejected=f.rejectedPackets();bool live=f.fresh(now);unsigned fmt=f.format();portEXIT_CRITICAL(&rxMux);
  Serial.printf("CAMERA_SOURCE source=%u fresh=%d active=%d frames=%lu seq=%lu age_ms=%lu rejected=%lu format=%u width=%u height=%u\n",source,live,cameraView.active&&cameraView.source==source,(unsigned long)count,(unsigned long)seq,(unsigned long)age,(unsigned long)rejected,fmt,facecamera::frameWidth(fmt),facecamera::frameHeight(fmt));
 }
 Serial.printf("CAMERA fresh=%d active=%d source=%u fallbacks=%lu\n",cameraFresh()!=0,cameraView.active,cameraView.source,(unsigned long)cameraFallbacks);
 tabCamera.report();
 Serial.print("TAB_POWER");for(unsigned reg:{3u,5u,7u,11u,13u}){auto v=lgfx::i2c::readRegister8(1,0x44,reg,100000);Serial.printf(" r%u=%d",reg,v.has_value()?int(v.value()&0xa0):-1);}Serial.println();
 Serial.printf("CAMERA_TCP received=%lu errors=%lu\n",(unsigned long)cameraReceiver.received,(unsigned long)cameraReceiver.errors);
 for(unsigned i=0;i<FACE_COUNT;i++){if(!faceEnabled(i))continue;const auto &r=receiver.peers[i];const auto &p=r.packet;uint32_t now=millis();Serial.printf("NODE color=%s seen=%d fresh=%d valid=%lu pct=%.2f volts=%.3f age_ms=%lu rx=%lu seq=%lu current_valid=%lu ma=%.1f charger=%s\n",FACE_NAMES[i],r.seen,r.fresh(now),(unsigned long)p.valid,p.percent,p.volts,(unsigned long)(r.seen?uint32_t(now-r.receivedAt):0),(unsigned long)r.count,(unsigned long)p.seq,(unsigned long)p.currentValid,p.currentMilliAmps,chargeLabel(p.chargerState));}
}
void graph(unsigned i,int x,int y,uint16_t ink){
 const auto &m=histories[i];const int w=256,h=28;tile.drawRect(x-1,y-1,w+2,h+2,0x4208);if(!m.count)return;
 float lo=m.at(0).volts,hi=lo;for(unsigned n=1;n<m.count;n++){lo=fminf(lo,m.at(n).volts);hi=fmaxf(hi,m.at(n).volts);}if(hi-lo<.04f){float mid=(lo+hi)*.5f;lo=mid-.02f;hi=mid+.02f;}
 uint32_t newest=m.at(m.count-1).time;bool have=false;int px=0,py=0;
 for(unsigned n=0;n<m.count;n++){auto p=m.at(n);uint32_t age=newest-p.time;if(age>120000)continue;int tx=x+w-1-age*(w-1)/120000U;int ty=y+h-1-(p.volts-lo)/(hi-lo)*(h-1);ty=constrain(ty,y,y+h-1);if(have)tile.drawLine(px,py,tx,ty,ink);px=tx;py=ty;have=true;}
}
void card(unsigned i,uint32_t now){
 const auto &r=receiver.peers[i];const auto &p=r.packet;bool fresh=r.fresh(now),valid=fresh&&p.valid;
 int x=0,y=0;tile.fillScreen(0);uint16_t ink=fresh?FACE_INKS[i]:0x8410;
 tile.drawRoundRect(x,y,616,304,14,0x4208);tile.setTextColor(ink);tile.setTextSize(3);tile.setCursor(x+20,y+16);
 tile.print(faceTitle(i));tile.print(" FACE");
 tile.setTextSize(2);tile.setCursor(x+510,y+22);tile.setTextColor(fresh?0x07e0:0xfd20);tile.print(fresh?"LIVE":r.seen?"OFFLINE":"WAIT");
 tile.setTextColor(0xffff);tile.setTextSize(7);tile.setCursor(x+312,y+60);if(!hasBatteryMeasurement(i))tile.print(i==7?"CAM":"USB");else if(valid)tile.printf(voltageEstimated(i)?"~%d%%":"%d%%",(int)lroundf(p.percent));else tile.print("--%");
 tile.setTextSize(3);tile.setCursor(x+312,y+122);if(!hasBatteryMeasurement(i))tile.print("NO CELL");else if(valid)tile.printf("%.3f V",p.volts);else tile.print("--.--- V");
 tile.setTextSize(2);tile.setCursor(x+312,y+162);
 if(!fresh)tile.print(r.seen?"OFFLINE":"WAITING FOR DATA");else if(!hasBatteryMeasurement(i))tile.print(i==7?"CAMERA / NO GAUGE":"USB POWERED / NO GAUGE");else if(!valid)tile.print("BATTERY UNKNOWN");else if(voltageEstimated(i))tile.print("VOLTAGE ESTIMATE");else if(p.chargerState)tile.print(chargeLabel(p.chargerState));else{BatteryDiagnostics d;d.evidence=(ChargeEvidence)p.evidence;tile.print(d.label());}
 tile.setCursor(x+312,y+190);
 if(valid&&p.currentValid)tile.printf("%+.0f mA into cell",p.currentMilliAmps);
 else if(valid&&p.windowMilliseconds>=300000)tile.printf("5m %+.2f pp",p.delta5Percent);
 else tile.print(!hasBatteryMeasurement(i)?"OTHER BATTERIES: SWIPE":"LEARNING TREND");
 BatteryState b;if(valid)b.update(p.volts,p.percent,0,now);FaceState pose=batteryPose(b);if(!hasBatteryMeasurement(i))pose.expression=HAPPY;const uint8_t *pixels=faces[i].pixels(pose,now%4200>=3950&&now%4200<4090);
 for(int sy=0;sy<64;sy++)for(int sx=0;sx<128;sx++)if((pixels[(sy/8)*128+sx]>>(sy%8))&1)tile.fillRect(x+22+sx*2,y+76+sy*2,2,2,i==4?rainbowInk(sx):ink);
 graph(i,x+24,y+242,ink);
 tile.setTextColor(0xffff);tile.setTextSize(2);tile.setCursor(x+312,y+222);
 if(valid&&p.seconds)tile.printf("%lus: %+.2f pp",(unsigned long)p.seconds,p.deltaPercent);else tile.print(!hasBatteryMeasurement(i)?"NO BATTERY MEASUREMENT":"TREND: --");
 tile.setCursor(x+312,y+246);if(valid&&p.seconds)tile.printf("%+.3f V",p.deltaVolts);
 tile.setTextColor(0x8410);tile.setCursor(x+24,y+280);if(r.seen)tile.printf("Last update %lus ago",(unsigned long)(uint32_t(now-r.receivedAt)/1000));else tile.print("No readings received");
 if(voltageEstimated(i)){tile.setCursor(x+312,y+280);tile.print("SOC EST / AMPS UNKNOWN");}
}
bool drawCamera(uint32_t now){
 portENTER_CRITICAL(&rxMux);unsigned format=cameraFrames[cameraView.source].format();bool valid=cameraFrames[cameraView.source].copyPixels(cameraPixels,millis());portEXIT_CRITICAL(&rxMux);if(!valid)return false;
 canvas.fillScreen(0);canvas.setTextColor(0x05d7);canvas.setTextSize(3);canvas.setCursor(20,20);canvas.print(cameraView.source==1?"TAB5 CAMERA / LIVE STILLS":format==2?"XIAO CAMERA / 160x120 B&W":"XIAO CAMERA / 80x60 COLOR");
 for(int y=0;y<600;y++)for(int x=0;x<800;x++)canvas.drawPixel(240+x,72+y,facecamera::pixel565(cameraPixels,format,x*facecamera::frameWidth(format)/800,y*facecamera::frameHeight(format)/600));
 canvas.setTextColor(0xffff);canvas.setTextSize(2);canvas.setCursor(20,690);canvas.print("Tap/swipe: cameras/faces | Hold XIAO image: color/B&W");if(panel)canvas.pushSprite(0,0);return true;
}
void draw(){
 uint32_t now=millis();if(cameraView.active&&drawCamera(now))return;canvas.fillScreen(0);canvas.setTextSize(3);canvas.setTextColor(0xffff);canvas.setCursor(20,20);canvas.print("BATTERY FACES");
 canvas.setTextSize(2);canvas.setCursor(850,26);canvas.setTextColor(WiFi.status()==WL_CONNECTED?0x07e0:0xfd20);canvas.print(WiFi.status()==WL_CONNECTED?"BLUE LINK CONNECTED":"CONNECTING TO BLUE");
 unsigned active[FACE_COUNT],count=receiver.liveFaces(active,now);
 if(!count){canvas.setTextColor(0xffff);canvas.setCursor(40,160);canvas.print("Waiting for live battery faces...");}
 for(unsigned n=0;n<count;n++){
  card(active[n],now);auto r=tileRect(n,count);float sx=r.w/616.0f,sy=r.h/304.0f,scale=fminf(sx,sy);
  tile.pushRotateZoom(&canvas,r.x+r.w*.5f,r.y+r.h*.5f,0,scale,scale);
 }
 if(panel)canvas.pushSprite(0,0);
}
bool screenWrite(const char *data,size_t length,uint32_t start){
 size_t at=0;while(at<length){if(uint32_t(millis()-start)>20000)return false;size_t n=length-at;if(n>256)n=256;size_t sent=Serial.write((const uint8_t*)data+at,n);at+=sent;if(!sent)delay(1);}return true;
}
void screenshot(bool full=false){
 const unsigned width=full?1280:320,height=full?720:180,step=full?1:4;uint32_t start=millis();
 char header[40];int headerLength=snprintf(header,sizeof(header),"SCREEN_BEGIN %u %u\n",width,height);if(!screenWrite(header,headerLength,start))return;static char row[5122];const char hex[]="0123456789abcdef";
 for(unsigned y=0;y<height;y++){for(unsigned x=0;x<width;x++){uint16_t c=canvas.readPixel(x*step,y*step);for(unsigned n=0;n<4;n++)row[x*4+n]=hex[(c>>(12-n*4))&15];}row[width*4]='\n';if(!screenWrite(row,width*4+1,start))return;delay(1);}screenWrite("SCREEN_END\n",11,start);
}
void setup(){
 Serial.setTxBufferSize(4096);Serial.begin(115200);Serial.setTxTimeoutMs(0);initializePanel();display.setTextSize(3);display.setTextColor(0xffff);display.setCursor(20,20);display.print("BATTERY MONITOR - STARTING");
 canvas.setColorDepth(16);canvas.setPsram(true);if(!canvas.createSprite(1280,720)){Serial.println("ERR canvas allocation");return;}
 tile.setColorDepth(16);tile.setPsram(true);if(!tile.createSprite(616,304)){Serial.println("ERR tile allocation");return;}tile.setPivot(308,152);
 // M5GFX enables the Tab5's onboard radio power. Preserve charger configuration.
 lgfx::i2c::bitOff(1,0x43,0x05,1,100000); // internal antenna
 WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.begin(MONITOR_SSID,MONITOR_PASSWORD);udp.begin(MONITOR_PORT);cameraReceiver.begin();tabCamera.begin();xTaskCreate(receiveTask,"battery-ingress",4096,nullptr,2,nullptr);lastConnect=millis();draw();
}
void loop(){
 uint32_t now=millis();copyIngress();
 unsigned cameraLive=cameraFresh();cameraView.followDefault(cameraLive);
 if(cameraView.expire(cameraLive))cameraFallbacks++;
 uint16_t tx=0,ty=0;bool down=panel&&display.getTouch(&tx,&ty);int action=cameraSwipe.update(down,tx,ty,now);
 if(action==4&&cameraView.active&&cameraView.source==0){portENTER_CRITICAL(&rxMux);unsigned format=cameraFrames[0].format();portEXIT_CRITICAL(&rxMux);cameraReceiver.requestFormat(format==2?1:2);}else if(action){if(action==2&&cameraView.active&&cameraView.defaultWanted)cameraView.leave();else cameraView.cycle(cameraLive,action<0?-1:1);}if(!panel && uint32_t(now-lastPanelTry)>=5000)initializePanel();
 for(unsigned i=0;i<FACE_COUNT;i++){
  const auto &r=receiver.peers[i];const auto &p=r.packet;
  if(!r.fresh(millis())){histories[i]=BatteryMicroTrend{};priorSequence[i]=0;}
  else if(priorSequence[i]!=p.seq){BatteryState b;if(p.valid)b.update(p.volts,p.percent,0,millis());if(p.uptime<priorUptime[i])histories[i]=BatteryMicroTrend{};histories[i].update(b,r.receivedAt);priorSequence[i]=p.seq;priorUptime[i]=p.uptime;}
 }
 while(Serial.available()){char c=Serial.read();if(c=='\n'||c=='\r'){command[used]=0;Serial.setTxTimeoutMs(100);if(!strcmp(command,"view camera")||!strcmp(command,"view camera 0"))cameraView.select(cameraLive,0);else if(!strcmp(command,"view camera 1"))cameraView.select(cameraLive,1);else if(!strcmp(command,"quality color"))cameraReceiver.requestFormat(1);else if(!strcmp(command,"quality gray"))cameraReceiver.requestFormat(2);else if(!strcmp(command,"camera off"))tabCamera.setEnabled(false);else if(!strcmp(command,"camera on"))tabCamera.setEnabled(true);else if(!strcmp(command,"view default"))cameraView.resetDefault(cameraLive);else if(!strcmp(command,"view faces"))cameraView.leave();else if(!strcmp(command,"status"))report();else if(!strcmp(command,"screen"))screenshot();else if(!strcmp(command,"screen full"))screenshot(true);else if(!strncmp(command,"sync ",5)){Serial.printf("SYNC %s\n",command+5);report();}Serial.setTxTimeoutMs(0);used=0;}else if(c>=32&&c<=126&&used<sizeof(command)-1)command[used++]=c;}
 if(uint32_t(now-lastDraw)>=250){lastDraw=now;draw();}
 if(WiFi.status()!=WL_CONNECTED&&uint32_t(now-lastConnect)>=15000){lastConnect=now;WiFi.reconnect();}delay(2);
}
