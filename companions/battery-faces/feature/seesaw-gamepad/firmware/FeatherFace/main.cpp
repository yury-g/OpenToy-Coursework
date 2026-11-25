// SPDX-License-Identifier: MIT
// Feather display/USB adapter for the preserved OpenToy Face v2 renderer.
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <WiFiUdp.h>
#include "MonitorPacket.h"
#include "FaceLinkConfig.h"
#include <Preferences.h>
#include "PeerBattery.h"
#include "FaceNavigation.h"
#include "FaceHardware.h"
#include "BatteryLed.h"
#include "BatteryState.h"
#include "BatteryDiagnostics.h"
#include "BatteryMicroTrend.h"
#include "DoodlePixels.h"
#include "FaceDisplay.h"
#include "CachedFace.h"
#if !FACE_TOUCH
#include "FeatherHaptic.h"
#include "FeatherQwiicButton.h"
#include "FeatherGamepad.h"
FeatherHaptic haptic;FeatherQwiicButton qwiicButton;FeatherGamepad gamepad;
bool creatureEnabled=false,qEverPressed=false;uint32_t creatureSeed=0,lastQPress=0;
#endif
#if !FACE_SILVER && !FACE_STICK && !FACE_NESSO && !FACE_YELLOW && !FACE_CYD && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S3_REVTFT)
#error Select the confirmed Feather Reverse TFT or Arduino Nesso N1
#endif
const char *deviceColor(){return FACE_NAMES[localFace()];}
uint16_t faceInk(){return FACE_INKS[localFace()];}
FaceState sourcePose(const BatteryState &b,unsigned which){auto p=batteryPose(b);if(!hasBatteryMeasurement(which))p.expression=HAPPY;return p;}
#if !FACE_TOUCH
const char BUILD[]="BatteryFaces-v27-seesaw-gamepad";
#else
const char BUILD[]="BatteryFaces-v18-green-silver-faces";
#endif
#if FACE_CYD
// On this classic ESP32, early global malloc fails although setup has >110KB
// contiguous free heap. Allocate after Arduino's heap initialization.
class DeferredCanvas:public GFXcanvas16 {
 public:DeferredCanvas():GFXcanvas16(DISPLAY_WIDTH,DISPLAY_HEIGHT,false){}
 bool begin(){buffer=(uint16_t*)malloc(DISPLAY_WIDTH*DISPLAY_HEIGHT*2);buffer_owned=true;if(buffer)memset(buffer,0,DISPLAY_WIDTH*DISPLAY_HEIGHT*2);return buffer!=nullptr;}
};
DeferredCanvas canvas;
#else
GFXcanvas16 canvas(DISPLAY_WIDTH,DISPLAY_HEIGHT);
#endif
BatteryLedColor led={0,0,0};
BatteryState battery;
BatteryDiagnostics diagnostics;
BatteryMicroTrend micro;
BatterySession test;
SessionRecord lastWatch,lastRun;
Preferences storage;
bool storageReady=false,batteryFace=true;
unsigned page=0,selectedFace=0;
BatteryMicroTrend remoteMicro[FACE_COUNT];
uint32_t remoteLastCount[FACE_COUNT]={},remoteLastUptime[FACE_COUNT]={};
DoodleAnimator remoteAnimation[FACE_COUNT];
PeerBattery peers[FACE_COUNT];
HardwareBattery hardwareBattery;
SwipeGesture swipe;
uint32_t gestures=0;
const char *chargerLabel(uint32_t state){return state==1?"CHARGING":state==2?"DISCHARGE":state==3?"CHG DONE":state==4?"IDLE":"UNKNOWN";}
void selectFace(unsigned which){selectedFace=which;page=0;if(page>1 && selectedFace!=localFace())page=1;}
void advanceFace(int direction){selectFace(nextFace(selectedFace,direction));}

portMUX_TYPE peerMux=portMUX_INITIALIZER_UNLOCKED;
bool radioReady=false,radioPaused=false;
constexpr uint8_t BROADCAST_MAC[6]={255,255,255,255,255,255};
uint32_t broadcastCompleted=0;
uint32_t sent=0,sendErrors=0,ackOk[FACE_COUNT]={},ackLost[FACE_COUNT]={};
void sentPeer(const esp_now_send_info_t *info,esp_now_send_status_t status){
 if(!info || !info->des_addr)return;
 if(!memcmp(info->des_addr,BROADCAST_MAC,6)){portENTER_CRITICAL(&peerMux);if(status==ESP_NOW_SEND_SUCCESS)broadcastCompleted++;portEXIT_CRITICAL(&peerMux);return;}
 portENTER_CRITICAL(&peerMux);for(unsigned i=0;i<FACE_COUNT;i++)if(!memcmp(info->des_addr,FACE_MACS[i],6)){if(status==ESP_NOW_SEND_SUCCESS)ackOk[i]++;else ackLost[i]++;}portEXIT_CRITICAL(&peerMux);
}
WiFiUDP monitorUdp;bool gatewayReady=false;uint32_t gatewaySent=0,lastGateway=0,localSampleAt=0;PeerPacket localPacket;
PeerBattery peerSnapshot(unsigned which){
 portENTER_CRITICAL(&peerMux);PeerBattery copy=peers[which];portEXIT_CRITICAL(&peerMux);return copy;
}
void updateRemoteHistories(){
 for(unsigned which=0;which<FACE_COUNT;which++)if(faceEnabled(which) && which!=localFace()){
  PeerBattery remote=peerSnapshot(which);uint32_t now=millis();const auto &p=remote.packet;
  if(!remote.fresh(now)){remoteMicro[which]=BatteryMicroTrend{};remoteLastCount[which]=0;continue;}
  if(remoteLastCount[which]!=remote.count){
   BatteryState b;if(p.valid)b.update(p.volts,p.percent,0,now);
   if(p.uptime<remoteLastUptime[which])remoteMicro[which]=BatteryMicroTrend{};
   remoteMicro[which].update(b,now);remoteLastCount[which]=remote.count;remoteLastUptime[which]=p.uptime;
  }
 }
}
#if !FACE_TOUCH
bool findReceiveEspNow(const uint8_t*,const void*,size_t,uint32_t);
#endif
void receivePeer(const esp_now_recv_info_t *info,const uint8_t *data,int len){
 if(!info || len<0)return;
 uint32_t now=millis();
#if !FACE_TOUCH
 if(findReceiveEspNow(info->src_addr,data,(size_t)len,now))return;
#endif
 portENTER_CRITICAL(&peerMux);
 for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i) && i!=localFace())peers[i].receive(info->src_addr,data,(size_t)len,now,FACE_MACS[i]);
 portEXIT_CRITICAL(&peerMux);
}
void startRadio(){
 WiFi.persistent(false);WiFi.setAutoReconnect(false);
 if(!WiFi.mode(localFace()==0?WIFI_AP_STA:WIFI_STA))return;
 uint32_t start=millis();while(!WiFi.STA.started() && uint32_t(millis()-start)<2000)delay(10);
 if(!WiFi.STA.started())return;
 WiFi.disconnect(false,false); // stop any saved-AP association/scan, without erasing its settings
 WiFi.setSleep(false); // ESP-NOW must remain available while the UI/USB is idle
 if(WiFi.setChannel(1)!=ESP_OK || esp_now_init()!=ESP_OK)return;
#if FACE_YELLOW
 esp_wifi_set_max_tx_power(34); // reduce peak demand on the watch board's supply
#endif
 if(localFace()==0){gatewayReady=WiFi.softAP(MONITOR_SSID,MONITOR_PASSWORD,1,false,1);if(gatewayReady)monitorUdp.begin(MONITOR_PORT+1);}
 radioReady=esp_now_register_recv_cb(receivePeer)==ESP_OK && esp_now_register_send_cb(sentPeer)==ESP_OK;
 for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i) && i!=localFace()){
  esp_now_peer_info_t dest={};memcpy(dest.peer_addr,FACE_MACS[i],6);dest.channel=1;dest.ifidx=WIFI_IF_STA;dest.encrypt=false;
  if(esp_now_add_peer(&dest)!=ESP_OK)radioReady=false;
 }
 esp_now_peer_info_t broadcast={};memcpy(broadcast.peer_addr,BROADCAST_MAC,6);broadcast.channel=1;broadcast.ifidx=WIFI_IF_STA;
 if(esp_now_add_peer(&broadcast)!=ESP_OK)radioReady=false;
}
void sendBattery(uint32_t now){
 if(!radioReady || radioPaused)return;
 PeerPacket p;p.seq=++sent;p.uptime=now;p.valid=battery.valid;
 if(battery.valid){p.percent=battery.percent;p.volts=battery.volts;}
 p.deltaPercent=micro.deltaPercent;p.deltaVolts=micro.deltaVolts;p.seconds=micro.seconds;
 p.warm=diagnostics.warmReady;p.evidence=(uint32_t)diagnostics.evidence;
 p.windowMilliseconds=diagnostics.windowReady?diagnostics.windowMilliseconds:0;p.delta5Percent=diagnostics.deltaPercent;p.delta5Volts=diagnostics.deltaVolts;
 p.currentValid=hardwareBattery.currentValid;p.currentMilliAmps=hardwareBattery.current;p.chargerState=hardwareBattery.chargerState;
 p.diagnosticAge=diagnostics.started?uint32_t(now-diagnostics.first):0;
 localPacket=p;localSampleAt=now;
 // One broadcast per battery sample keeps traffic independent of family size.
 // Delivery is verified at receivers; ESP-NOW broadcast completion is not an ACK.
 if(esp_now_send(BROADCAST_MAC,(const uint8_t*)&p,sizeof(p))!=ESP_OK)sendErrors++;

}
void serviceMonitor(uint32_t now){
 if(localFace()!=0 || !gatewayReady || uint32_t(now-lastGateway)<1000)return;
 lastGateway=now;if(!WiFi.softAPgetStationNum())return;
 for(unsigned i=0;i<FACE_COUNT;i++){
  if(!faceEnabled(i))continue;MonitorPacket packet;packet.face=i;
  if(i==localFace()){packet.packet=localPacket;packet.sourceAgeMs=now-localSampleAt;}
  else {PeerBattery peer=peerSnapshot(i);if(!peer.seen)continue;packet.packet=peer.packet;packet.sourceAgeMs=uint32_t(millis()-peer.receivedAt);}
  // The sole dashboard has a fixed private address; unicast avoids broadcast-only delivery gaps.
  monitorUdp.beginPacket(IPAddress(192,168,4,2),MONITOR_PORT);monitorUdp.write((const uint8_t*)&packet,sizeof(packet));if(monitorUdp.endPacket())gatewaySent++;
 }
}
#if !FACE_TOUCH
#include "FeatherFind.h"
bool accessoryFindCapable(){return !isRetiredGreenDevice()&&haptic.present&&haptic.ready;}
bool accessoryOutlineVisible(uint32_t now){return haptic.held||haptic.creature.active||haptic.breathing.active||(haptic.pulse.hasStarted&&uint32_t(now-haptic.pulse.began)<600);}
uint32_t accessoryIoFlags(uint32_t now){
 uint32_t flags=accessory::LIVE_IO|accessory::ioDriveBits(haptic.commanded);
 if(qwiicButton.ready){flags|=accessory::BUTTON_READY;if(qwiicButton.state.pressed)flags|=accessory::BUTTON_DOWN;if(qEverPressed&&uint32_t(now-lastQPress)<1500)flags|=accessory::RECENT_PRESS;}
 if(haptic.busy()&&haptic.commanded)flags|=accessory::MOTOR_ACTIVE;
 if(haptic.creature.active||haptic.breathing.active)flags|=accessory::BREATHING;
 if(haptic.held)flags|=accessory::HELD;if(creatureEnabled)flags|=accessory::BREATH_ENABLED;
 if(accessoryOutlineVisible(now)){flags|=accessory::OUTLINE_VISIBLE;if(haptic.busy())flags|=accessory::OUTLINE_ACTIVE;}return flags;
}
void accessoryFindStop(){if(haptic.busy()&&!haptic.held)haptic.stop();}
bool accessoryFindPulse(uint32_t now){
 if(haptic.breathing.active||haptic.creature.active){haptic.stop();return false;}
 if(haptic.busy()||!haptic.quiet(now)||!haptic.pulse.canStart(now))return false;
 uint8_t previous=haptic.level;haptic.level=1;bool ok=haptic.trigger(now);haptic.level=previous;return ok;
}

#endif
uint32_t lastSave=0,noticeAt=0;
const char *notice="";
void saveTest(){
 if(test.record.outcome==TestOutcome::Empty)return;
 if(test.record.kind==TestKind::Watch)lastWatch=test.record;else lastRun=test.record;
 if(storageReady)storage.putBytes(test.record.kind==TestKind::Watch?"watch":"run",&test.record,sizeof(test.record));
}
void testAction(uint32_t now){
 if(page<=1 || selectedFace!=localFace() || FACE_TOUCH){batteryFace=true;notice="AUTO FACE";noticeAt=now;return;}
 TestKind kind=page==2?TestKind::Watch:TestKind::Runtime;
 if(test.running()){
  if(test.record.kind!=kind){notice="STOP OTHER";noticeAt=now;return;}
  test.update(battery,now,usbPowered());test.stop();saveTest();notice="STOPPED";noticeAt=now;return;
 }
 if(!diagnostics.warmReady){notice="WAIT 2 MIN";noticeAt=now;return;}
 if(kind==TestKind::Runtime && usbPowered()){notice="UNPLUG USB";noticeAt=now;return;}
 if(!test.start(kind,battery,now)){notice="CHARGE 25%";noticeAt=now;return;}
 lastSave=now;saveTest();notice="STARTED";noticeAt=now;
}
uint32_t lastBattery=0;
DoodleAnimator animation;
FaceState pose=restingDoodle();
uint8_t pixels[1024]={};
uint32_t frames=0,lastFrame=0,lastReport=0,lastMood=0,maxRenderMs=0;
#if FACE_TOUCH
CachedFace cachedFaces[FACE_COUNT];
#endif
bool demo=false,gauge=false,reportStream=false;
char command[100]={};unsigned used=0;bool overflow=false;
void report() {
 uint8_t id[6]={};esp_read_mac(id,ESP_MAC_WIFI_STA);char mac[18];
 snprintf(mac,sizeof(mac),"%02x:%02x:%02x:%02x:%02x:%02x",id[0],id[1],id[2],id[3],id[4],id[5]);
 Serial.printf("FACE build=%s mac=%s mood=%s demo=%d frames=%lu uptime_ms=%lu settled=%d hash=%08lx tft=%dx%d gauge=%d\n",BUILD,mac,expressionName(pose.expression),demo,(unsigned long)frames,(unsigned long)millis(),animation.settled(),(unsigned long)faceHash(pixels),DISPLAY_WIDTH,DISPLAY_HEIGHT,gauge);
 Serial.printf("BATT valid=%d pct=%.2f volts=%.3f rate=%.3f hours=%.3f state=%s\n",battery.valid,battery.percent,battery.volts,battery.rate,battery.hoursLeft,battery.label());
 Serial.printf("DIAG evidence=%s warm=%d window=%d dp=%.3f dv=%.4f page=%u auto=%d test=%u outcome=%u seconds=%lu saved=%d\n",diagnostics.label(),diagnostics.warmReady,diagnostics.windowReady,diagnostics.deltaPercent,diagnostics.deltaVolts,page,batteryFace,(unsigned)test.record.kind,(unsigned)test.record.outcome,(unsigned long)test.record.elapsedSeconds,storageReady);
 Serial.printf("MICRO seconds=%u dp=%.4f dv=%.5f direction=%d samples=%u\n",micro.seconds,micro.deltaPercent,micro.deltaVolts,(diagnostics.warmReady && micro.seconds>=110)?micro.direction():0,micro.count);
 Serial.printf("DISPLAY selected=%s page=%u gestures=%lu\n",FACE_NAMES[selectedFace],page,(unsigned long)gestures);
 Serial.printf("RADIO ready=%d paused=%d channel=%d wifi_status=%d sent=%lu errors=%lu sta=%s\n",radioReady,radioPaused,WiFi.channel(),WiFi.status(),(unsigned long)sent,(unsigned long)sendErrors,WiFi.macAddress().c_str());
 for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i) && i!=localFace()){
  PeerBattery remote=peerSnapshot(i);uint32_t now=millis();const auto &p=remote.packet;
  Serial.printf("PEER color=%s seen=%d fresh=%d age_ms=%lu rx=%lu seq=%lu uptime_ms=%lu valid=%lu pct=%.2f volts=%.3f current_valid=%lu ma=%.1f charger=%s\n",FACE_NAMES[i],remote.seen,remote.fresh(now),(unsigned long)(remote.seen?uint32_t(now-remote.receivedAt):0),(unsigned long)remote.count,(unsigned long)p.seq,(unsigned long)p.uptime,(unsigned long)p.valid,p.percent,p.volts,(unsigned long)p.currentValid,p.currentMilliAmps,chargerLabel(p.chargerState));
 }
 for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i) && i!=localFace()){uint32_t ok,lost;portENTER_CRITICAL(&peerMux);ok=ackOk[i];lost=ackLost[i];portEXIT_CRITICAL(&peerMux);Serial.printf("ACK color=%s ok=%lu lost=%lu\n",FACE_NAMES[i],(unsigned long)ok,(unsigned long)lost);}
 Serial.printf("GATEWAY ready=%d clients=%u sent=%lu\n",gatewayReady,localFace()==0?WiFi.softAPgetStationNum():0,(unsigned long)gatewaySent);
 Serial.printf("POWER current_valid=%d ma=%.1f charger=%s\n",hardwareBattery.currentValid,hardwareBattery.current,chargerLabel(hardwareBattery.chargerState));
#if FACE_YELLOW
 uint32_t completed;portENTER_CRITICAL(&peerMux);completed=broadcastCompleted;portEXIT_CRITICAL(&peerMux);int8_t tx=0;uint8_t proto=0;esp_wifi_get_max_tx_power(&tx);esp_wifi_get_protocol(WIFI_IF_STA,&proto);Serial.printf("YRADIO tx_quarter_dbm=%d protocol=%u paint=%d broadcast_completed=%lu\n",tx,proto,yellowPaint,(unsigned long)completed);
#endif
 reportFaceHardware();
#if !FACE_TOUCH
 if(!isRetiredGreenDevice()){haptic.reportCreature(creatureEnabled,creatureSeed);haptic.report();qwiicButton.report();if(localFace()==0)gamepad.report();}
#endif
 Serial.printf("PERF max_render_ms=%lu\n",(unsigned long)maxRenderMs);
 Serial.printf("DEVICE color=%s face_ink=%04x output=%s led=%u,%u,%u\n",deviceColor(),faceInk(),isRetiredGreenDevice()?"led":FACE_SILVER?"rlcd":FACE_TOUCH?"touch-tft":"tft",led.r,led.g,led.b);
}
void drawMicroGraph(const BatteryMicroTrend &micro,const BatteryDiagnostics &diagnostics){
#if FACE_SILVER
 const int x=8,y=48,w=184,h=20;
#else
 const int x=8,y=113,w=119,h=16;
#endif
 const uint16_t gray=0x4208;
 int dir=(diagnostics.warmReady && micro.seconds>=110)?micro.direction():0;
 uint16_t ink=dir>0?0x07e0:dir<0?0xf800:0xfd20;
 #if !FACE_SILVER
 // Arrow describes measured voltage/SOC movement, never charger input current.
 if(dir>0){canvas.fillTriangle(108,76,116,66,124,76,ink);canvas.fillRect(113,76,7,8,ink);}
 else if(dir<0){canvas.fillTriangle(108,75,116,85,124,75,ink);canvas.fillRect(113,67,7,8,ink);}
 else canvas.fillRect(109,73,15,4,ink);
 #endif
 canvas.drawRect(x-1,y-1,w+2,h+2,gray);
 if(!micro.count)return;
 float lo=micro.at(0).volts,hi=lo;
 for(unsigned i=1;i<micro.count;i++){lo=fminf(lo,micro.at(i).volts);hi=fmaxf(hi,micro.at(i).volts);}
 // A minimum40mV scale prevents tiny noise appearing as a huge swing.
 if(hi-lo<.04f){float mid=(lo+hi)*.5f;lo=mid-.02f;hi=mid+.02f;}
 uint32_t newest=micro.at(micro.count-1).time;
 bool have=false;int px=0,py=0;
 for(unsigned i=0;i<micro.count;i++){
  const auto &p=micro.at(i);uint32_t age=newest-p.time;if(age>120000)continue;
  int tx=x+w-1-int(age*(w-1)/120000U);
  int ty=y+h-1-(int)((p.volts-lo)/(hi-lo)*(h-1));ty=constrain(ty,y,y+h-1);
  if(have)canvas.drawLine(px,py,tx,ty,ink);else canvas.drawPixel(tx,ty,ink);
  px=tx;py=ty;have=true;
 }
 if(have)canvas.fillCircle(px,py,1,ink);
}
void drawBattery(const BatteryState &battery,const BatteryDiagnostics &diagnostics,const BatteryMicroTrend &micro,bool remote=false,bool fresh=true,bool seen=false,uint32_t age=0,bool currentValid=false,float current=0,uint32_t chargerState=0) {

#if FACE_SILVER
 canvas.setTextWrap(false);canvas.setTextColor(0xffff);canvas.setTextSize(1);canvas.setCursor(8,0);canvas.print(faceTitle(selectedFace));if(voltageEstimated(selectedFace))canvas.print(" EST");if(remote&&!fresh)canvas.print(" OFFLINE");
 if(page==0){
  canvas.setTextSize(3);canvas.setCursor(8,14);if(!hasBatteryMeasurement(selectedFace))canvas.print("USB");else if(battery.valid)canvas.printf(voltageEstimated(selectedFace)?"~%d%%":"%d%%",(int)lroundf(battery.percent));else canvas.print("--%");
  canvas.setTextSize(2);canvas.setCursor(104,18);if(battery.valid)canvas.printf("%.2fV",battery.volts);else canvas.print("--.--V");
  canvas.setTextSize(1);canvas.setCursor(8,44);canvas.print(!hasBatteryMeasurement(selectedFace)?"NO CELL / USB POWER":remote&&!fresh?"WAITING FOR BATTERY":voltageEstimated(selectedFace)?"VOLTAGE EST / mA UNKNOWN":diagnostics.label());
  canvas.setCursor(8,57);if(battery.valid&&micro.seconds)canvas.printf("%lus: %+.2fpp %+.3fV",(unsigned long)micro.seconds,micro.deltaPercent,micro.deltaVolts);else canvas.print("LEARNING BATTERY TREND");
 }else{
  canvas.setCursor(8,14);canvas.setTextSize(2);canvas.print("2 MIN TREND");canvas.setCursor(8,34);canvas.setTextSize(1);
  if(micro.seconds)canvas.printf("%lus %+.2fpp %+.3fV",(unsigned long)micro.seconds,micro.deltaPercent,micro.deltaVolts);else canvas.print("LEARNING / NO CHANGE YET");
  drawMicroGraph(micro,diagnostics);
 }
 return;
#endif

#if FACE_YELLOW
 if(!hasBatteryMeasurement(selectedFace)){canvas.setTextColor(FACE_INKS[selectedFace]);canvas.setTextSize(2);canvas.setCursor(16,8);canvas.print(faceTitle(selectedFace));canvas.setTextColor(0xffff);canvas.setTextSize(5);canvas.setCursor(16,32);canvas.print(remote&&!fresh?"OFF":"USB");canvas.setTextSize(2);canvas.setCursor(16,88);canvas.print("NO BATTERY GAUGE");canvas.setCursor(16,122);canvas.print(remote&&!fresh?"OFFLINE":"USB POWERED");return;}
 canvas.setTextWrap(false);canvas.setTextSize(2);canvas.setTextColor(FACE_INKS[selectedFace]);canvas.setCursor(16,8);
 canvas.print(faceTitle(selectedFace));
 canvas.setTextColor(0xffff);canvas.setTextSize(5);canvas.setCursor(16,28);
 if(battery.valid)canvas.printf(voltageEstimated(selectedFace)?"~%d%%":"%d%%",(int)lroundf(battery.percent));else canvas.print("--%");
 canvas.setTextSize(3);canvas.setCursor(16,72);if(battery.valid)canvas.printf("%.3fV",battery.volts);else canvas.print("--.---V");
 canvas.setTextSize(2);canvas.setCursor(16,103);
 if(remote&&!fresh)canvas.print(seen?"OFFLINE":"WAITING");
 else if(page==1){if(micro.available())canvas.printf("%+.2fpp %us",micro.deltaPercent,micro.seconds);else canvas.print("LEARNING TREND");}
 else canvas.print(chargerState?chargerLabel(chargerState):voltageEstimated(selectedFace)?"VOLT EST":diagnostics.label());
 canvas.setCursor(16,127);
 if(remote&&!fresh){if(seen)canvas.printf("%lus AGO",(unsigned long)(age/1000));else canvas.print("FOR PEER");}
 else if(page==1){if(micro.available())canvas.printf("%+.3fV",micro.deltaVolts);else canvas.print("--.---V");}
 else if(currentValid)canvas.printf("%+.0fmA CELL",current);
 else canvas.print(voltageEstimated(selectedFace)?"SOC ~ / mA UNKNOWN":"AMPS UNKNOWN");
 canvas.setTextSize(1);canvas.setCursor(160,14);canvas.setTextColor(fresh?0x07e0:0xfd20);if(remote)canvas.print(fresh?"LIVE":"OFFLINE");
 if(page==1&&micro.count){
  float lo=micro.at(0).volts,hi=lo;for(unsigned n=1;n<micro.count;n++){lo=fminf(lo,micro.at(n).volts);hi=fmaxf(hi,micro.at(n).volts);}if(hi-lo<.04f)hi=lo+.04f;
  int px=0,py=0;bool have=false;uint32_t newest=micro.at(micro.count-1).time;
  for(unsigned n=0;n<micro.count;n++){auto p=micro.at(n);uint32_t a=newest-p.time;if(a>120000)continue;int x=16+207-a*207/120000U,y=159-(p.volts-lo)/(hi-lo)*12;if(have)canvas.drawLine(px,py,x,y,FACE_INKS[selectedFace]);px=x;py=y;have=true;}
 }
 return;
#endif
 if(!hasBatteryMeasurement(selectedFace)){
  canvas.setTextColor(FACE_INKS[selectedFace]);canvas.setTextSize(1);canvas.setCursor(8,0);canvas.print(faceTitle(selectedFace));
  canvas.setTextColor(0xffff);canvas.setTextSize(3);canvas.setCursor(8,18);canvas.print(remote&&!fresh?"OFFLINE":"USB");
  canvas.setTextSize(2);canvas.setCursor(8,56);canvas.print("NO CELL");canvas.setCursor(8,80);canvas.print("NO GAUGE");
  canvas.setTextSize(1);canvas.setCursor(8,112);canvas.print("SWIPE: OTHER BATTERY");return;
 }
 const uint16_t white=0xffff;
 uint16_t color=battery.valid?(battery.percent<=15?0xf800:0x07e0):0x8410;
 canvas.setTextWrap(false);canvas.setTextSize(1);canvas.setTextColor(FACE_INKS[selectedFace]);canvas.setCursor(8,0);canvas.print(faceTitle(selectedFace));
 if(remote){canvas.setCursor(86,0);canvas.setTextColor(fresh?0x07e0:0xfd20);canvas.print(fresh?"LIVE":seen?"OFFLINE":"WAITING");}
 canvas.setTextColor(white);
 // 24x32 pixel characters: four times the original small text dimensions.
 canvas.setTextSize(4);canvas.setCursor(8,10);
 if(battery.valid)canvas.printf(voltageEstimated(selectedFace)?"~%d%%":"%d%%",(int)lroundf(battery.percent));else canvas.print("--%");
 canvas.setTextSize(2);canvas.setCursor(8,44);
 if(battery.valid)canvas.printf("%.3fV",battery.volts);else canvas.print("--.--V");
 canvas.drawRoundRect(92,43,31,19,3,color);canvas.fillRect(123,49,4,7,color);
 if(battery.valid){int fill=(int)(battery.percent*27/100);if(fill)canvas.fillRect(94,45,fill,15,color);}
 canvas.setTextSize(2);
 if(remote && !fresh){
  canvas.setCursor(8,67);canvas.print(seen?"OFFLINE":"WAITING");
  canvas.setCursor(8,91);if(seen)canvas.printf("%lus AGO",(unsigned long)(age/1000));else canvas.print("FOR PEER");
  canvas.setTextSize(1);canvas.setCursor(8,116);canvas.print(FACE_TOUCH?"SWIPE: NEXT BATTERY":"D0: NEXT BATTERY");
 }else if(page==0){
  canvas.setCursor(8,67);
  if(chargerState)canvas.print(chargerLabel(chargerState));
  else if(voltageEstimated(selectedFace))canvas.print("VOLT EST");
  else if(!remote && diagnostics.evidence==ChargeEvidence::Learning && diagnostics.started){uint32_t age=millis()-diagnostics.first;canvas.printf("LEARN %lum",(unsigned long)((age<420000?420000-age:0)+59999)/60000);}
  else canvas.print(diagnostics.label());
  bool toFull=diagnostics.evidence==ChargeEvidence::Gaining;
  float hours=toFull?diagnostics.hoursToFull(battery):diagnostics.hoursLeft(battery);
  canvas.setCursor(8,91);
  if(voltageEstimated(selectedFace) && !currentValid)canvas.print("--h--m");
  else if(currentValid)canvas.printf("%+.0fmA",current);
  else if(hours>=0 && hours<100){unsigned m=(unsigned)lroundf(hours*60);canvas.printf("~%uh%02um",m/60,m%60);}
  else if(hours>=100)canvas.print(">99h");else canvas.print("--h--m");
  canvas.setCursor(8,111);canvas.print(currentValid?"CELL mA":toFull?"FULL (EST)":"LEFT (EST)");
 }else if(page==1){
  canvas.setCursor(8,67);
  if(micro.available())canvas.printf(fabsf(micro.deltaPercent)<10?"%+.2fpp":"%+.1fpp",micro.deltaPercent);else canvas.print("--.--pp");
  canvas.setCursor(8,91);
  if(micro.available())canvas.printf("%+.3fV",micro.deltaVolts);else canvas.print("--.---V");
  canvas.setCursor(96,91);
  if(micro.seconds>=110)canvas.print("2m");else if(micro.seconds>=60)canvas.print("1m");else canvas.printf("%us",micro.seconds);
  drawMicroGraph(micro,diagnostics);
 }else{
  TestKind kind=page==2?TestKind::Watch:TestKind::Runtime;
  const SessionRecord &r=(test.record.kind==kind && test.record.outcome!=TestOutcome::Empty)?test.record:(page==2?lastWatch:lastRun);
  canvas.setCursor(8,67);
  if(r.outcome==TestOutcome::Empty)canvas.print(page==2?"WATCH TEST":"RUN TEST");
  else canvas.printf("%s%lum",page==2?"WATCH":"RUN",(unsigned long)r.elapsedSeconds/60);
  canvas.setCursor(8,91);
  if(r.outcome==TestOutcome::Empty)canvas.print(page==2?"TRACK GAIN":"UNPLUG USB");
  else {
   float delta=page==2?r.endPercent-r.startPercent:r.startPercent-r.endPercent;
   if(page==2)canvas.printf(fabsf(delta)<10?"GAIN%+.1f%%":"GAIN%+.0f%%",delta);
   else canvas.printf(fabsf(delta)<10?"USED%.1f%%":"USED%.0f%%",delta);
  }
  canvas.setCursor(8,111);
  if(r.outcome==TestOutcome::Running && page==2 && r.elapsedSeconds>=600 && r.endPercent-r.startPercent<.7f && r.endVolts-r.startVolts<.01f && diagnostics.evidence!=ChargeEvidence::NearFull)canvas.print("NO GAIN");
  else if(r.outcome==TestOutcome::Running)canvas.print("USB: STOP");
  else if(r.outcome==TestOutcome::LowStop)canvas.print("RECHARGE");
  else if(r.outcome==TestOutcome::UsbStop)canvas.print("USB STOP");
  else if(r.outcome==TestOutcome::Interrupted)canvas.print("INTERRUPT");
  else canvas.print("USB: START");
 }
 if(*notice && uint32_t(millis()-noticeAt)<2500){canvas.fillRect(0,108,135,20,0);canvas.setCursor(8,111);canvas.print(notice);}

}
void cycleMood(uint32_t now) {
 Expression next=(Expression)((pose.expression+1)%6);
 pose=restingDoodle();pose.expression=next;lastMood=now;
}
void serialTxTimeout(uint32_t ms){
#if !FACE_CYD
 Serial.setTxTimeoutMs(ms);
#endif
}
struct CommandOutput {CommandOutput(){serialTxTimeout(10);}~CommandOutput(){serialTxTimeout(0);}};
void execute(uint32_t now) {
 CommandOutput commandOutput;
#if !FACE_TOUCH
 if(!strncmp(command,"haptic ",7)&&strcmp(command,"haptic status")&&strcmp(command,"haptic sample")){findLocalAcknowledge();creatureEnabled=false;}
 // Long USB transfers must not extend a motor pulse.
 if(haptic.busy()&&strcmp(command,"haptic status")&&strcmp(command,"haptic sample")&&strcmp(command,"gamepad sample")){haptic.stop();if(haptic.busy()){Serial.println("ERR haptic stop pending");return;}}
#endif
 if(!strcmp(command,"stream on")){
#if ARDUINO_USB_MODE == 1
  reportStream=true;Serial.println("OK stream on");
#else
  Serial.println("ERR stream unavailable; use status");
#endif
 }
 else if(!strcmp(command,"stream off")){reportStream=false;Serial.println("OK stream off");}
 else if(!strcmp(command,"status"))report();
 else if(!strcmp(command,"demo on")){batteryFace=false;demo=true;lastMood=now;Serial.println("OK demo on");}
 else if(!strcmp(command,"demo off")){batteryFace=true;demo=false;Serial.println("OK demo off");}
 else if(!strncmp(command,"doodle ",7)) {
  FaceState next;
  if(parseDoodle(command+7,next)){pose=next;batteryFace=false;demo=false;Serial.println("OK doodle");}
  else Serial.println("ERR invalid doodle");
 }
 else if(!strcmp(command,"battery on")){batteryFace=true;demo=false;Serial.println("OK battery on");}
 else if(!strncmp(command,"select ",7)){
  bool found=false;for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i) && !strcmp(command+7,FACE_NAMES[i])){selectFace(i);found=true;Serial.printf("OK select %s\n",FACE_NAMES[i]);}
  if(!found)Serial.println("ERR unknown face");
 }
#if FACE_YELLOW
 else if(!strcmp(command,"panel off")){yellowPaint=false;digitalWrite(15,LOW);Serial.println("OK panel paused");}
 else if(!strcmp(command,"panel on")){yellowPaint=true;digitalWrite(15,HIGH);Serial.println("OK panel resumed");}
 else if(!strncmp(command,"radio rate ",11)){
  unsigned rate=atoi(command+11);if(rate!=1 && rate!=6){Serial.println("ERR rate 1 or 6");return;}
  esp_now_rate_config_t config={};config.phymode=rate==6?WIFI_PHY_MODE_11G:WIFI_PHY_MODE_11B;config.rate=rate==6?WIFI_PHY_RATE_6M:WIFI_PHY_RATE_1M_L;
  unsigned errors=0;for(unsigned i=0;i<FACE_COUNT;i++)if(faceEnabled(i) && i!=localFace()&&esp_now_set_peer_rate_config(FACE_MACS[i],&config)!=ESP_OK)errors++;
  Serial.printf("OK rate %u errors=%u\n",rate,errors);
 }
#endif
 else if(!strcmp(command,"radio off")){radioPaused=true;Serial.println("OK radio paused");}
 else if(!strcmp(command,"radio on")){radioPaused=false;Serial.println("OK radio resumed");}
 else if(!strcmp(command,"view default")){selectFace(localFace());page=0;Serial.println("OK default battery face");}
 else if(!strcmp(command,"view trend")){page=1;Serial.println("OK view trend");}
 else if(!strcmp(command,"view face")){page=0;Serial.println("OK view face");}
 else if(!strcmp(command,"view watch")){selectFace(localFace());page=FACE_TOUCH?1:2;Serial.println("OK view watch");}
 else if(!strcmp(command,"view runtime")){selectFace(localFace());page=FACE_TOUCH?1:3;Serial.println("OK view runtime");}
#if !FACE_TOUCH
 else if(!strcmp(command,"i2c scan")){if(localFace()!=0){Serial.println("ERR scan is Blue only");return;}Serial.println("I2C_SCAN_BEGIN");for(uint8_t a=8;a<0x78;a++){Wire.beginTransmission(a);if(Wire.endTransmission()==0)Serial.printf("I2C_ADDRESS %02x\n",a);}Serial.println("I2C_SCAN_END");}
 else if(!strcmp(command,"gamepad sample")){if(localFace()==0){gamepad.report();}else Serial.println("ERR gamepad is Blue only");}
 else if(!strcmp(command,"gamepad auto")){gamepad.controls.automatic();batteryFace=true;demo=false;gamepad.report();}
 else if(!strcmp(command,"gamepad probe")){if(localFace()==0)gamepad.begin(millis());gamepad.report();}
 else if(!strcmp(command,"button status")){qwiicButton.report();}
 else if(!strcmp(command,"button probe")){qwiicButton.begin(millis());qwiicButton.report();}
 else if(!strcmp(command,"button led on")){qwiicButton.lit=true;qwiicButton.light(qwiicButton.state.pressed?128:16);qwiicButton.report();}
 else if(!strcmp(command,"button led off")){qwiicButton.lit=false;qwiicButton.light(0);qwiicButton.report();}
 else if(!strcmp(command,"haptic next")){bool ok=haptic.cycleAndTrigger(millis());Serial.printf("HAPTIC_TRIGGER accepted=%d source=usb_level\n",ok);haptic.report();}
 else if(!strcmp(command,"haptic sample")){HardwareBattery sample=readFaceBattery();Serial.printf("HAPTIC_SAMPLE at_ms=%lu volts=%.3f pct=%.2f\n",(unsigned long)millis(),sample.volts,sample.percent);haptic.service(millis());qwiicButton.animate(haptic.commanded,haptic.busy(),haptic.creature.active||haptic.breathing.active);haptic.reportBreathing();haptic.reportCreature(creatureEnabled,creatureSeed);qwiicButton.report();haptic.report();}
 else if(!strcmp(command,"haptic creature on")){HardwareBattery sample=readFaceBattery();bool ok=haptic.ready&&isfinite(sample.volts)&&sample.volts>=3.45f;creatureEnabled=ok;Serial.printf("CREATURE_SET accepted=%d enabled=%d\n",ok,creatureEnabled);haptic.reportCreature(creatureEnabled,creatureSeed);haptic.report();}
 else if(!strcmp(command,"haptic creature off")){creatureEnabled=false;haptic.stop();haptic.reportCreature(creatureEnabled,creatureSeed);haptic.report();}
 else if(!strcmp(command,"haptic stop")){haptic.stop();Serial.printf("HAPTIC_STOP accepted=%d\n",!haptic.busy());haptic.reportBreathing();haptic.report();}
 else if(!strncmp(command,"haptic breathe ",15)){unsigned peak=0,seconds=0;bool ok=false;if(sscanf(command+15,"%u %u",&peak,&seconds)==2&&peak>=4&&peak<=32&&seconds>=10&&seconds<=60&&seconds%10==0){HardwareBattery sample=readFaceBattery();if(isfinite(sample.volts)&&sample.volts>=3.45f)ok=haptic.startBreathing(millis(),peak,seconds);}Serial.printf("HAPTIC_BREATHE accepted=%d\n",ok);haptic.reportBreathing();haptic.reportCreature(creatureEnabled,creatureSeed);qwiicButton.report();haptic.report();}
 else if(!strncmp(command,"haptic level ",13)){const char *name=command+13;bool known=false;for(unsigned k=0;k<3;k++)if(!strcmp(name,k==0?"low":k==1?"med":"high")){haptic.level=k;known=true;}Serial.printf("HAPTIC_LEVEL accepted=%d\n",known);haptic.report();}
 else if(!strncmp(command,"haptic test ",12)){char level[8];unsigned duration=0;bool accepted=false;if(sscanf(command+12,"%7s %u",level,&duration)==2&&duration>=50&&duration<=800){HardwareBattery sample=readFaceBattery();if(isfinite(sample.volts)&&sample.volts>=3.45f)for(unsigned k=0;k<3;k++)if(!strcmp(level,k==0?"low":k==1?"med":"high")){haptic.level=k;accepted=haptic.trigger(millis(),duration);}}Serial.printf("HAPTIC_TEST accepted=%d\n",accepted);haptic.report();}
 else if(!strcmp(command,"haptic status")){if(!isRetiredGreenDevice())haptic.report();}
 else if(!strcmp(command,"haptic pulse")){bool ok=!isRetiredGreenDevice()&&haptic.trigger(millis());Serial.printf("HAPTIC_TRIGGER accepted=%d source=usb\n",ok);haptic.report();}
 else if(!strcmp(command,"haptic probe")){bool ok=!isRetiredGreenDevice()&&haptic.begin();Serial.printf("HAPTIC_PROBE ready=%d\n",ok);haptic.report();}
#endif
 else if(!strcmp(command,"test start")){testAction(now);Serial.printf("OK test action %s\n",notice);}
 else if(!strcmp(command,"test stop")){test.update(battery,now,usbPowered());test.stop();saveTest();Serial.println("OK test stop");}
 else if(!strcmp(command,"screen")) {
  if(isRetiredGreenDevice()){Serial.println("ERR no display");return;}
  Serial.printf("SCREEN_BEGIN %d %d\n",DISPLAY_WIDTH,DISPLAY_HEIGHT);
  static char row[DISPLAY_WIDTH*4+1];const char hex[]="0123456789abcdef";
  for(unsigned y=0;y<DISPLAY_HEIGHT;y++){
   for(unsigned x=0;x<DISPLAY_WIDTH;x++){uint16_t c=canvas.getBuffer()[y*DISPLAY_WIDTH+x];for(unsigned n=0;n<4;n++)row[x*4+n]=hex[(c>>(12-n*4))&15];}
   row[DISPLAY_WIDTH*4]=0;Serial.println(row);delay(2);
  }
  Serial.println("SCREEN_END");
 }
 else if(!strcmp(command,"frame")) {
  static char line[2055];
  memcpy(line,"FRAME ",6);
  const char hex[]="0123456789abcdef";
  for(unsigned i=0;i<1024;i++){line[6+2*i]=hex[pixels[i]>>4];line[7+2*i]=hex[pixels[i]&15];}
  line[2054]=0;Serial.println(line);
 }
 else Serial.println("ERR command: status | battery on | select blue/green/orange/yellow | view face/trend/watch/runtime | radio on/off | test start/stop | demo on/off | doodle <8 fields> | frame");
}
void setup() {
 selectedFace=localFace();
#if ARDUINO_USB_MODE == 1 || FACE_CYD
 Serial.setTxBufferSize(4096);
#endif
 Serial.begin(FACE_CYD?460800:115200);
 serialTxTimeout(0); // Bound writes if a connected host stops reading.
#if FACE_CYD
 canvas.begin();
#endif
 // Never wait for a host; the face also runs on battery.
 if(!canvas.getBuffer()){while(true){Serial.printf("ERR framebuffer allocation width=%d height=%d free=%lu max=%lu\n",DISPLAY_WIDTH,DISPLAY_HEIGHT,(unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getMaxAllocHeap());delay(1000);}}
 initFaceHardware();hardwareBattery=readFaceBattery();battery.update(hardwareBattery.volts,hardwareBattery.percent,hardwareBattery.rate,millis());gauge=battery.valid;
#if !FACE_TOUCH
 if(!isRetiredGreenDevice()){uint64_t identity=ESP.getEfuseMac();creatureSeed=uint32_t(identity)^uint32_t(identity>>32);haptic.begin();qwiicButton.begin(millis());if(localFace()==0)gamepad.begin(millis());}
#endif
 storageReady=storage.begin("face-battery",false);
 if(storageReady){
  SessionRecord r;
  if(storage.getBytesLength("watch")==sizeof(r)){storage.getBytes("watch",&r,sizeof(r));BatterySession saved;saved.restore(r);lastWatch=saved.record;}
  if(storage.getBytesLength("run")==sizeof(r)){storage.getBytes("run",&r,sizeof(r));BatterySession saved;saved.restore(r);lastRun=saved.record;}
 }
 diagnostics.update(battery,millis());micro.update(battery,millis());pose=sourcePose(battery,localFace());animation.reset(pose,millis());lastMood=millis();
 startRadio();sendBattery(millis());
}
void loop() {
 uint32_t now=millis();serviceMonitor(now);
#if !FACE_TOUCH
 if(!isRetiredGreenDevice()){
  haptic.service(now);findService(millis());uint32_t lastPoll=qwiicButton.lastPoll;
  if(qwiicButton.service(millis())){qEverPressed=true;lastQPress=millis();findLocalAcknowledge();bool ok=qwiicButton.ready&&battery.valid&&battery.volts>=3.45f&&haptic.startHeld(millis());Serial.printf("QWIIC_PRESS held=%d level=%s\n",ok,haptic.levelName());}
  if(haptic.held&&(!qwiicButton.ready||qwiicButton.lastPoll!=lastPoll)){
   if(!qwiicButton.ready)creatureEnabled=false;
   haptic.confirmHeld(millis(),qwiicButton.state.pressed&&qwiicButton.state.raw,qwiicButton.ready);
  }
  if(!haptic.ready)creatureEnabled=false;
  bool qDown=qwiicButton.ready&&qwiicButton.state.pressed;
  if(creatureEnabled&&!haptic.busy()&&!findActive()&&!qDown&&haptic.quiet(millis()))haptic.startCreature(millis(),creatureSeed);
  qwiicButton.animate(haptic.commanded,haptic.busy(),haptic.creature.active||haptic.breathing.active);
  if(localFace()==0){
   if(gamepad.service(millis())){batteryFace=true;demo=false;selectFace(localFace());Serial.printf("GAMEPAD_EVENT manual=%d mood=%s x=%d y=%d presses=%u\n",gamepad.controls.manual,expressionName(gamepad.controls.expression),gamepad.controls.axisX,gamepad.controls.axisY,gamepad.controls.presses);}
   gamepad.controls.step(millis());
  }
 }
#endif
 if(uint32_t(now-lastBattery)>=2000) {
  lastBattery=now;
  hardwareBattery=readFaceBattery();battery.update(hardwareBattery.volts,hardwareBattery.percent,hardwareBattery.rate,now);gauge=battery.valid;
#if !FACE_TOUCH
  if((haptic.breathing.active||haptic.creature.active||haptic.held)&&(!battery.valid||battery.volts<3.45f)){creatureEnabled=false;haptic.stop();}
#endif
  diagnostics.update(battery,now);micro.update(battery,now);sendBattery(now);
  bool wasRunning=test.running();test.update(battery,now,usbPowered());
  if(wasRunning && (!test.running() || uint32_t(now-lastSave)>=60000)){saveTest();lastSave=now;}
 }
 while(Serial.available()) {
#if !FACE_TOUCH
 if(!isRetiredGreenDevice())haptic.service(millis());
#endif
  char c=Serial.read();
  if(c=='\n'||c=='\r') {
   if(overflow)Serial.println("ERR command too long");
   else if(used){command[used]=0;execute(now);
#if !FACE_TOUCH
 if(!isRetiredGreenDevice())haptic.service(millis());
#endif
 }
   used=0;overflow=false;
  } else if(c>=32 && c<=126) {
   if(used<sizeof(command)-1)command[used++]=c;else overflow=true;
  } else overflow=true;
 }
 // Debounce all three buttons; D0 is active LOW, D1/D2 active HIGH.
 static bool raw[3]={},stable[3]={};static uint32_t changed[3]={};
 bool pressed[3]={};readFaceButtons(pressed);
 for(unsigned b=0;b<3;b++) {
  if(pressed[b]!=raw[b]){raw[b]=pressed[b];changed[b]=now;}
  if(raw[b]!=stable[b] && uint32_t(now-changed[b])>=30) {
   stable[b]=raw[b];
#if !FACE_TOUCH
   if(stable[b]&&!isRetiredGreenDevice())findLocalAcknowledge();
#endif
   if(stable[b] && b==0 && !isRetiredGreenDevice()){advanceFace(1);Serial.printf("DISPLAY selected=%s page=%u\n",FACE_NAMES[selectedFace],page);}
   if(stable[b] && b==1){if(isRetiredGreenDevice())report();else page=(page+1)%2;}
   if(stable[b] && b==2){if(isRetiredGreenDevice())report();else {
#if !FACE_TOUCH
    creatureEnabled=!creatureEnabled;if(creatureEnabled&&(!haptic.ready||!battery.valid||battery.volts<3.45f))creatureEnabled=false;if(!haptic.held&&haptic.busy())haptic.stop();Serial.printf("CREATURE_TOGGLE enabled=%d\n",creatureEnabled);
#else
    testAction(now);
#endif
   }}
   Serial.printf("BUTTON D%u %d\n",b,stable[b]);
  }
 }
 if(FACE_TOUCH){
  int x=0,y=0;bool down=readTouchPoint(x,y);int gesture=swipe.update(down,x,y,millis());
  static bool wasDown=false;if(down!=wasDown){Serial.printf("TOUCH down=%d x=%d y=%d\n",down,x,y);wasDown=down;}
  if(gesture==2||gesture==1||gesture==-1){gestures++;if(gesture==2)page=(page+1)%2;else advanceFace(gesture);Serial.printf("GESTURE action=%s selected=%s page=%u count=%lu\n",gesture==2?"tap":gesture>0?"left":"right",FACE_NAMES[selectedFace],page,(unsigned long)gestures);}
 }
 if(batteryFace){demo=false;pose=sourcePose(battery,localFace());
#if !FACE_TOUCH
  if(localFace()==0&&gamepad.ready&&gamepad.controls.manual)pose.expression=gamepad.controls.expression;
#endif
 }
 else if(demo && uint32_t(now-lastMood)>=6000)cycleMood(now);
 FaceState target=pose;
#if !FACE_TOUCH
 if(haptic.busy()&&haptic.commanded>4)target.left=target.right=100;
#endif
 // Automatic blink only in demo; manual USB poses retain exact original pixels.
 if((stable[0] && isRetiredGreenDevice()) || ((demo||batteryFace) && now%4200>=3950 && now%4200<4090))target.left=target.right=100;
 animation.tick(target,now,true);
 if(uint32_t(now-lastFrame)>=50) {
  lastFrame=now;uint32_t renderStarted=millis();updateRemoteHistories();
  if(isRetiredGreenDevice()){
   led=stable[0]?BatteryLedColor{8,8,8}:batteryLed(battery,diagnostics,now);
   showIndicator(led,now);
  }else{
#if FACE_TOUCH
   memcpy(pixels,cachedFaces[localFace()].pixels(pose,(demo||batteryFace)&&now%4200>=3950&&now%4200<4090),1024);
#else
   drawDoodle(animation.current,pose.expression,pixels,animation.wiggle(0,now),animation.wiggle(1,now),animation.wiggle(2,now),0,gamepad.ready&&gamepad.controls.manual?gamepad.controls.lookX:0,gamepad.ready&&gamepad.controls.manual?gamepad.controls.lookY:0);
#endif
   if(selectedFace!=localFace()){
    unsigned which=selectedFace;PeerBattery remote=peerSnapshot(which);uint32_t receivedNow=millis();bool fresh=remote.fresh(receivedNow);
    const auto &p=remote.packet;BatteryState b;if(fresh && p.valid)b.update(p.volts,p.percent,0,receivedNow);
    BatteryMicroTrend m=remoteMicro[which];if(fresh && b.valid){m.deltaPercent=p.deltaPercent;m.deltaVolts=p.deltaVolts;m.seconds=p.seconds;}
    BatteryDiagnostics d;d.started=fresh && b.valid;d.first=receivedNow-p.diagnosticAge;d.warmReady=fresh && p.warm;
    d.evidence=fresh?(ChargeEvidence)p.evidence:ChargeEvidence::Unknown;
    d.windowReady=fresh && p.windowMilliseconds;d.windowMilliseconds=p.windowMilliseconds;d.deltaPercent=p.delta5Percent;d.deltaVolts=p.delta5Volts;
    FaceState state=sourcePose(b,which);
#if !FACE_TOUCH
    if(receivedNow%4200>=3950 && receivedNow%4200<4090)state.left=state.right=100;
#endif
    remoteAnimation[which].tick(state,receivedNow,true);
    static uint8_t peerPixels[1024];
#if FACE_TOUCH
    memcpy(peerPixels,cachedFaces[which].pixels(state,receivedNow%4200>=3950&&receivedNow%4200<4090),1024);
#else
    drawDoodle(remoteAnimation[which].current,state.expression,peerPixels,0,0,0);
#endif
    scaleFace(peerPixels,canvas.getBuffer(),fresh?FACE_INKS[which]:0x4208,fresh&&which==4);drawBattery(b,d,m,true,fresh,remote.seen,uint32_t(receivedNow-remote.receivedAt),fresh&&b.valid&&p.currentValid,p.currentMilliAmps,fresh?p.chargerState:0);
   }else {scaleFace(pixels,canvas.getBuffer(),faceInk(),localFace()==4);drawBattery(battery,diagnostics,micro,false,true,true,0,hardwareBattery.currentValid,hardwareBattery.current,hardwareBattery.chargerState);}
   if(FACE_TOUCH){canvas.setTextSize(1);canvas.setTextColor(0x8410);canvas.setCursor(FACE_SILVER?8:FACE_YELLOW?60:1,FACE_SILVER?142:FACE_YELLOW?272:233);canvas.print(FACE_SILVER?"BOOT:FACE KEY:INFO":FACE_STICK?"A:FACE B:INFO":"SWIPE FACE / TAP INFO");}
#if !FACE_TOUCH
   // Local accessories remain local even when browsing a remote battery.
   canvas.fillRect(0,131,135,25,0);canvas.setTextSize(1);canvas.setTextColor(qwiicButton.ready?(qwiicButton.state.pressed?0xffff:faceInk()):0xfd20);canvas.setCursor(5,133);canvas.print(qwiicButton.ready?(qwiicButton.state.pressed?"Q DOWN":"Q UP"):"NO Q");
   canvas.setCursor(51,133);canvas.setTextColor(haptic.ready?faceInk():0xf800);if(findActive())canvas.print("FIND ME");else if(haptic.held)canvas.printf("HOLD %u",haptic.commanded);else if(haptic.creature.active||haptic.breathing.active)canvas.printf("BREATH %u",haptic.commanded);else canvas.printf("%s %s",haptic.busy()?"BUZZ":"VIBE",haptic.ready?haptic.levelName():"FAULT");
   canvas.drawRect(5,145,125,8,0x4208);int amount=123*haptic.displayIntensity()/127;canvas.fillRect(6,146,amount,6,haptic.busy()?faceInk():0x4208);
   if(accessoryOutlineVisible(now))canvas.drawRoundRect(0,156,135,67,9,haptic.busy()?faceInk():0x4208);
   canvas.setTextColor(0x8410);canvas.setCursor(5,231);canvas.print(localFace()==0&&gamepad.ready?"PAD:LOOK SEL:AUTO":"Q:HOLD D2:BREATH");
#endif
   showCanvas(canvas.getBuffer());
  }
  maxRenderMs=max(maxRenderMs,uint32_t(millis()-renderStarted));frames++;
 }
 if(uint32_t(now-lastReport)>=2000){lastReport=now;if(reportStream && Serial.availableForWrite()>=3072)report();}
 delay(1);
}
