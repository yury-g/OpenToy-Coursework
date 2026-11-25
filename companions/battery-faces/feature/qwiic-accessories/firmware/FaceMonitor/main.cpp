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
#include "../FeatherFace/FaceNavigation.h"
#include "RuntimeStorage.h"
#include "RuntimeView.h"
#include "FindUi.h"
#ifndef ARDUINO_M5STACK_TAB5
#error Select M5Tab5
#endif
const char BUILD[]="BatteryMonitor-v25-accessory-outline";
class TabDisplay:public M5GFX {
 protected:bool init_impl(bool reset,bool clear)override{
  // This board is ROM/owner-confirmed; do not reuse an old NVS board hint.
  _board=autodetect(reset,m5gfx::board_t::board_M5Tab5);
  return _board==m5gfx::board_t::board_M5Tab5 && lgfx::LGFX_Device::init_impl(false,clear);
 }
};
TabDisplay display;M5Canvas canvas(&display),tile(&display);WiFiUDP udp,findUdp;
FindUi findUi;accessory::AccessoryPeer accessoryIngress[2];

MonitorReceiver receiver,ingress;portMUX_TYPE rxMux=portMUX_INITIALIZER_UNLOCKED;
BatteryMicroTrend histories[FACE_COUNT];CachedFace faces[FACE_COUNT];
uint32_t priorSequence[FACE_COUNT]={},priorUptime[FACE_COUNT]={},received=0,ingressCount=0,lastDraw=0,lastReport=0,lastConnect=0;
uint32_t panelDiagMask=0,lastPanelTry=0;
bool panel=false;char command[80]={};unsigned used=0;
RuntimeObserver runtimeLog;RuntimeStorage runtimeStorage;RuntimeView runtimeView;SwipeGesture touchGesture;
void draw();
const char *phaseLabel(){return runtimeLog.state.phase==RuntimeObserver::WAITING?"WAITING":runtimeLog.state.phase==RuntimeObserver::RUNNING?"RUNNING":runtimeLog.state.phase==RuntimeObserver::STOPPED?"STOPPED":"INTERRUPTED";}
void runtimeReport(){
 Serial.printf("RUNTIME phase=%s session=%lu ready=%u elapsed_s=%lu clock_s=%lu page=%s demo=%d storage=%d saved=%d restored=%d writes=%lu snapshot_bytes=%u\n",phaseLabel(),(unsigned long)runtimeLog.state.session,runtimeLog.ready(millis()),(unsigned long)runtimeLog.elapsed(),(unsigned long)runtimeLog.state.clockSeconds,runtimeView.backgroundPage?"log":runtimeView.racePage?"race":"faces",runtimeView.demoPage,runtimeStorage.available,runtimeStorage.saved,runtimeStorage.restored,(unsigned long)runtimeStorage.writes,(unsigned)sizeof(RuntimeSnapshot));
 for(unsigned k=0;k<4;k++){auto &t=runtimeLog.state.tracks[k];auto rate=runtimeLog.rate(k,millis());Serial.printf("RUNNER color=%s fresh=%d pct=%.2f volts=%.3f points=%u bg_points=%u observed_s=%lu run_observed_s=%lu last_contact_s=%lu low_at_s=%lu resets=%lu interrupted=%u rate_valid=%d pph=%.3f vph=%.4f eta10_min=%.1f\n",FACE_NAMES[RuntimeObserver::IDS[k]],runtimeLog.fresh(k,millis()),t.lastPercent,t.lastVolts,t.count,runtimeLog.background.tracks[k].count,(unsigned long)t.observedSeconds,(unsigned long)t.runObservedSeconds,(unsigned long)t.lastContact,(unsigned long)t.lowAt,(unsigned long)t.resets,t.interrupted,rate.valid,rate.pph,rate.voltsPerHour,runtimeLog.etaMinutes(k,millis()));}
}
void exportRuntime(){
 Serial.println("RUNTIME_CSV_BEGIN");Serial.println("# Passive observation; board identity is not battery identity. Gaps break lines. No measured capacity/health.");Serial.printf("# phase=%s,session=%lu,start_clock_s=%lu,elapsed_s=%lu\n",phaseLabel(),(unsigned long)runtimeLog.state.session,(unsigned long)runtimeLog.state.startSeconds,(unsigned long)runtimeLog.elapsed());Serial.println("dataset,color,clock_seconds,charge_percent,volts,gap_before,voltage_soc_estimate");
 for(unsigned dataset=0;dataset<2;dataset++)for(unsigned k=0;k<4;k++){auto &t=(dataset?runtimeLog.background:runtimeLog.state).tracks[k];unsigned i=RuntimeObserver::IDS[k];for(unsigned n=0;n<t.count;n++){auto &p=t.points[n];Serial.printf("%s,%s,%lu,%.2f,%.3f,%d,%d\n",dataset?"background":"comparison",FACE_NAMES[i],(unsigned long)p.seconds,p.centipercent*.01f,RuntimeObserver::volts(p),RuntimeObserver::gap(p),voltageEstimated(i));}}Serial.println("RUNTIME_CSV_END");
}
void copyIngress(){portENTER_CRITICAL(&rxMux);receiver=ingress;received=ingressCount;for(unsigned i=0;i<2;i++)findUi.control.peers[i]=accessoryIngress[i];portEXIT_CRITICAL(&rxMux);}
void receiveTask(void*){
 for(;;){
  int size=udp.parsePacket();
  if(size>0){MonitorPacket packet;uint32_t arrived=millis();
   if(size==(int)sizeof(accessory::Status)&&udp.remoteIP()==IPAddress(192,168,4,1)){
    accessory::Status status;if(udp.read((uint8_t*)&status,sizeof(status))==(int)sizeof(status)&&accessory::valid(status)){
     portENTER_CRITICAL(&rxMux);accessoryIngress[status.target].receive(status,arrived);portEXIT_CRITICAL(&rxMux);
    }
   }else if(size==(int)sizeof(packet)&&udp.remoteIP()==IPAddress(192,168,4,1)&&udp.read((uint8_t*)&packet,sizeof(packet))==(int)sizeof(packet)){
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
 runtimeReport();findUi.report(millis());
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
 tile.setTextColor(0xffff);tile.setTextSize(7);tile.setCursor(x+312,y+60);if(!hasBatteryMeasurement(i))tile.print("USB");else if(valid)tile.printf(voltageEstimated(i)?"~%d%%":"%d%%",(int)lroundf(p.percent));else tile.print("--%");
 tile.setTextSize(3);tile.setCursor(x+312,y+122);if(!hasBatteryMeasurement(i))tile.print("NO CELL");else if(valid)tile.printf("%.3f V",p.volts);else tile.print("--.--- V");
 tile.setTextSize(2);tile.setCursor(x+312,y+162);
 if(!fresh)tile.print(r.seen?"OFFLINE":"WAITING FOR DATA");else if(!hasBatteryMeasurement(i))tile.print("USB POWERED / NO GAUGE");else if(!valid)tile.print("BATTERY UNKNOWN");else if(voltageEstimated(i))tile.print("VOLTAGE ESTIMATE");else if(p.chargerState)tile.print(chargeLabel(p.chargerState));else{BatteryDiagnostics d;d.evidence=(ChargeEvidence)p.evidence;tile.print(d.label());}
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
 findUi.card(tile,i,now);
}
void draw(){
 uint32_t now=millis();
 if(runtimeView.racePage){runtimeView.draw(canvas,runtimeLog,now,runtimeStorage.saved);findUi.overlay(canvas,now);if(panel)canvas.pushSprite(0,0);return;}
 canvas.fillScreen(0);runtimeView.header(canvas);
 unsigned active[FACE_COUNT],count=receiver.liveFaces(active,now);
 if(!count){canvas.setTextColor(0xffff);canvas.setCursor(40,160);canvas.print("Waiting for live battery faces...");}
 for(unsigned n=0;n<count;n++){
  card(active[n],now);auto r=tileRect(n,count);float sx=r.w/616.0f,sy=r.h/304.0f,scale=fminf(sx,sy);
  tile.pushRotateZoom(&canvas,r.x+r.w*.5f,r.y+r.h*.5f,0,scale,scale);
 }
 findUi.overlay(canvas,now);if(panel)canvas.pushSprite(0,0);
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
 runtimeStorage.begin(runtimeLog);
 // M5GFX enables the Tab5's onboard radio power. Preserve charger configuration.
 lgfx::i2c::bitOff(1,0x43,0x05,1,100000); // internal antenna
 WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.config(IPAddress(192,168,4,2),IPAddress(192,168,4,1),IPAddress(255,255,255,0));findUi.control.boot=esp_random();if(!findUi.control.boot)findUi.control.boot=1;WiFi.begin(MONITOR_SSID,MONITOR_PASSWORD);udp.begin(MONITOR_PORT);xTaskCreate(receiveTask,"battery-ingress",4096,nullptr,2,nullptr);lastConnect=millis();draw();
}
void loop(){
 uint32_t now=millis();copyIngress();findUi.service(findUdp,now);runtimeLog.tick(now);
 for(unsigned k=0;k<4;k++)runtimeLog.observe(k,receiver.peers[RuntimeObserver::IDS[k]],now);
 if(uint32_t(now-runtimeStorage.lastSave)>=300000)runtimeStorage.save(runtimeLog,now);
 uint16_t tx=0,ty=0;bool down=panel&&display.getTouch(&tx,&ty);int gesture=touchGesture.update(down,tx,ty,now);
 if(gesture==2){if(!findUi.tap(touchGesture.lastX,touchGesture.lastY,now,!runtimeView.racePage,receiver)&&runtimeView.tap(touchGesture.lastX,touchGesture.lastY,now,runtimeLog))runtimeStorage.save(runtimeLog,now);draw();}
 else if(gesture==1||gesture==-1){runtimeView.racePage=!runtimeView.racePage;runtimeView.demoPage=false;runtimeView.backgroundPage=false;runtimeView.confirmation=0;draw();}
 if(runtimeView.confirmation&&uint32_t(now-runtimeView.confirmAt)>120000)runtimeView.confirmation=0;
 if(!panel && uint32_t(now-lastPanelTry)>=5000)initializePanel();
 for(unsigned i=0;i<FACE_COUNT;i++){
  const auto &r=receiver.peers[i];const auto &p=r.packet;
  if(!r.fresh(millis())){histories[i]=BatteryMicroTrend{};priorSequence[i]=0;}
  else if(priorSequence[i]!=p.seq){BatteryState b;if(p.valid)b.update(p.volts,p.percent,0,millis());if(p.uptime<priorUptime[i])histories[i]=BatteryMicroTrend{};histories[i].update(b,r.receivedAt);priorSequence[i]=p.seq;priorUptime[i]=p.uptime;}
 }
 while(Serial.available()){char c=Serial.read();if(c=='\n'||c=='\r'){command[used]=0;Serial.setTxTimeoutMs(100);if(!strcmp(command,"view faces")||!strcmp(command,"view default")){runtimeView.racePage=false;runtimeView.demoPage=false;runtimeView.backgroundPage=false;runtimeView.confirmation=0;draw();}
 else if(!strcmp(command,"view race")){runtimeView.racePage=true;runtimeView.demoPage=false;runtimeView.backgroundPage=false;runtimeView.confirmation=0;draw();}
 else if(!strcmp(command,"view log")){runtimeView.racePage=true;runtimeView.demoPage=false;runtimeView.backgroundPage=true;runtimeView.confirmation=0;draw();}
 else if(!strcmp(command,"monitor reboot")){Serial.println("MONITOR_REBOOT");Serial.flush();delay(100);ESP.restart();}
 else if(!strcmp(command,"view demo")){runtimeView.startDemo(now);draw();}
 else if(!strcmp(command,"race save")){Serial.printf("CHECKPOINT ok=%d\n",runtimeStorage.save(runtimeLog,now));}
 else if(!strcmp(command,"race export"))exportRuntime();
 else if(!strncmp(command,"ui tap ",7)){int x,y;if(sscanf(command+7,"%d %d",&x,&y)==2){if(!findUi.tap(x,y,now,!runtimeView.racePage,receiver)&&runtimeView.tap(x,y,now,runtimeLog))runtimeStorage.save(runtimeLog,now);draw();}}
 else if(!strcmp(command,"find once blue")||!strcmp(command,"find once green")){findUi.oneShotLease(findUdp,command[10]=='b'?0:1,now);draw();}
 else if(!strcmp(command,"find stop")){findUi.control.stopAll(now);draw();}
 else if(!strcmp(command,"find blue")||!strcmp(command,"find green")){bool ok=findUi.control.toggle(command[5]=='b'?0:1,now);Serial.printf("FIND accepted=%d\n",ok);draw();}
 else if(!strcmp(command,"status"))report();else if(!strcmp(command,"screen"))screenshot();else if(!strcmp(command,"screen full"))screenshot(true);else if(!strncmp(command,"sync ",5)){Serial.printf("SYNC %s\n",command+5);report();}Serial.setTxTimeoutMs(0);used=0;}else if(c>=32&&c<=126&&used<sizeof(command)-1)command[used++]=c;}
 if(uint32_t(now-lastDraw)>=250){lastDraw=now;draw();}
 if(WiFi.status()!=WL_CONNECTED&&uint32_t(now-lastConnect)>=15000){lastConnect=now;WiFi.reconnect();}delay(2);
}
