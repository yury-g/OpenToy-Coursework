// SPDX-License-Identifier: MIT
// XIAO ESP32S3 Sense snapshot sender. No microphone, SD, stored images, or IP service.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <atomic>
#include "../FeatherFace/CameraProtocol.h"
#include "../FeatherFace/CameraCapture.h"
#include "../FeatherFace/CameraControl.h"
#include "../FeatherFace/PeerBattery.h"

namespace {
constexpr unsigned FIRMWARE_VERSION=17,SOURCE_INDEX=7;
constexpr uint32_t FRAME_INTERVAL_MS=2000,PRESENCE_INTERVAL_MS=2000;
constexpr uint8_t CHANNEL=1,BROADCAST[6]={255,255,255,255,255,255};
uint8_t pixels[facecamera::FRAME_BYTES],packet[facecamera::PACKET_BYTES],stationMac[6];
bool cameraReady=false,radioReady=false,streaming=true,radioWanted=true;
uint32_t lastRadioTry=0,maxSendWait=0,sendTimeouts=0;
std::atomic<unsigned> requestedFormat{2};unsigned lastFormat=2;
void onReceive(const esp_now_recv_info_t *info,const uint8_t *data,int size){unsigned format=0;if(info&&size>=0&&decodeCameraControl(info->src_addr,data,size,format))requestedFormat.store(format);}
std::atomic<uint32_t> txCallbacks{0};
uint16_t sensorPid=0;
esp_err_t cameraInitError=ESP_OK;
uint32_t bootSession=0,staleDiscarded=0,lastCaptureAge=0;
uint32_t frameSeq=0,presenceSeq=0,framesCaptured=0,framesSent=0;
uint32_t captureErrors=0,sendErrors=0,initErrors=0,lastCrc=0,lastCaptureAt=0,lastPresenceAt=0;
std::atomic<bool> txDone{true},txSucceeded{false};

void onSent(const esp_now_send_info_t*,esp_now_send_status_t status){
 txCallbacks.fetch_add(1);txSucceeded.store(status==ESP_NOW_SEND_SUCCESS);txDone.store(true);
}
void radioOff(){
 if(radioReady)esp_now_deinit();
 radioReady=false;WiFi.mode(WIFI_OFF);txDone.store(true);
}
bool radioOn(){
 if(radioReady)return true;lastRadioTry=millis();
 WiFi.persistent(false);
 if(!WiFi.mode(WIFI_STA)){++initErrors;return false;}
 uint32_t started=millis();while(!WiFi.STA.started()&&uint32_t(millis()-started)<2000)delay(10);
 if(!WiFi.STA.started()){++initErrors;return false;}
 // Keep the existing NVS credentials intact; do not associate with any network.
 WiFi.disconnect(false,false);
 if(esp_wifi_set_channel(CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK||
    esp_wifi_set_ps(WIFI_PS_NONE)!=ESP_OK||esp_wifi_set_max_tx_power(34)!=ESP_OK|| // 8.5 dBm
    esp_now_init()!=ESP_OK){++initErrors;radioOff();return false;}
 esp_now_peer_info_t peer={};memcpy(peer.peer_addr,BROADCAST,6);
 peer.channel=CHANNEL;peer.ifidx=WIFI_IF_STA;peer.encrypt=false;
 if(esp_now_register_send_cb(onSent)!=ESP_OK||esp_now_register_recv_cb(onReceive)!=ESP_OK||esp_now_add_peer(&peer)!=ESP_OK){
  ++initErrors;esp_now_deinit();radioOff();return false;
 }
 radioReady=true;return true;
}
bool sendPacket(const void *data,size_t n){
 if(!radioReady)return false;
 txDone.store(false);txSucceeded.store(false);
 if(esp_now_send(BROADCAST,static_cast<const uint8_t*>(data),n)!=ESP_OK){++sendErrors;txDone.store(true);return false;}
 const uint32_t started=millis();
 while(!txDone.load()&&uint32_t(millis()-started)<500)delay(1);
 maxSendWait=max(maxSendWait,uint32_t(millis()-started));
 if(!txDone.load()){
  ++sendTimeouts;
  // Deinitializing also prevents a late callback from satisfying the next send.
  ++sendErrors;radioOff();return false;
 }
 if(!txSucceeded.load()){++sendErrors;return false;}
 return true;
}
bool initCamera(){
 if(!psramFound()||ESP.getPsramSize()<8u*1024u*1024u){cameraInitError=ESP_ERR_NO_MEM;++initErrors;return false;}
 camera_config_t c={};
 c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
 // Official Seeed XIAO ESP32S3 Sense pin map: D0..D7 = Y2..Y9.
 c.pin_d0=15;c.pin_d1=17;c.pin_d2=18;c.pin_d3=16;
 c.pin_d4=14;c.pin_d5=12;c.pin_d6=11;c.pin_d7=48;
 c.pin_xclk=10;c.pin_pclk=13;c.pin_vsync=38;c.pin_href=47;
 c.pin_sccb_sda=40;c.pin_sccb_scl=39;c.pin_pwdn=-1;c.pin_reset=-1;
 c.xclk_freq_hz=20000000;c.pixel_format=PIXFORMAT_RGB565;c.frame_size=FRAMESIZE_QVGA;
 c.fb_location=CAMERA_FB_IN_PSRAM;c.fb_count=1;c.grab_mode=CAMERA_GRAB_WHEN_EMPTY;
 c.jpeg_quality=12;
 cameraInitError=esp_camera_init(&c);
 if(cameraInitError!=ESP_OK){++initErrors;return false;}
 sensor_t *s=esp_camera_sensor_get();if(s)sensorPid=s->id.PID;
 return true;
}
bool captureFrame(){
 const uint32_t uptime=millis();lastCaptureAt=uptime;
 if(!cameraReady){++captureErrors;return false;}
 camera_fb_t *fb=nullptr;
 // A single-buffer camera can retain an image through a long pause. Drop it
 // using the driver timestamp; only a newly captured image may become LIVE.
 for(unsigned attempt=0;attempt<3;attempt++){
  fb=esp_camera_fb_get();if(!fb)break;
  int64_t captured=int64_t(fb->timestamp.tv_sec)*1000000+fb->timestamp.tv_usec;
  int64_t age=esp_timer_get_time()-captured;lastCaptureAge=age<0?0:uint32_t(age/1000);
  if(freshCameraCapture(esp_timer_get_time(),fb->timestamp.tv_sec,fb->timestamp.tv_usec))break;
  esp_camera_fb_return(fb);fb=nullptr;staleDiscarded++;
 }
 if(!fb){++captureErrors;return false;}
 lastFormat=requestedFormat.load();
 const bool converted=fb->format==PIXFORMAT_RGB565&&(lastFormat==2?
  facecamera::downsampleGray2(fb->buf,fb->len,fb->width,fb->height,pixels,sizeof pixels):
  facecamera::downsampleRgb565be(fb->buf,fb->len,fb->width,fb->height,pixels,sizeof pixels));
 esp_camera_fb_return(fb);
 if(!converted){++captureErrors;return false;}
 ++framesCaptured;++frameSeq;lastCrc=facecamera::crc32(pixels,sizeof pixels);
 if(!radioReady)return true; // A manual frame can inspect capture while radio is off.
 bool sentComplete=false;
 // Whole-frame repetition recovers a dropped first-copy chunk without 22 ACKs.
 // A receiver ignores a completed duplicate and restarts an incomplete copy.
 for(unsigned repeat=0;repeat<2&&radioReady;repeat++){
  bool sentCopy=true;
  for(unsigned i=0;i<facecamera::CHUNK_COUNT;i++){
   const size_t n=facecamera::buildChunkWithCrc(pixels,frameSeq,uptime,lastCrc,i,packet,sizeof packet,bootSession,0,lastFormat);
   if(!n||!sendPacket(packet,n)){sentCopy=false;break;}
   delay(4); // Leave airtime for the battery peers and legacy ESP-NOW receivers.
  }
  sentComplete|=sentCopy;delay(12);
 }
 if(sentComplete)++framesSent;
 return sentComplete;
}
void sendPresence(){
 PeerPacket p;p.seq=++presenceSeq;p.uptime=millis();p.valid=0;
 // Identity is the station MAC, with registry index 7 at the receivers. There
 // is no standard onboard battery divider/gauge; never manufacture a voltage.
 sendPacket(&p,sizeof p);lastPresenceAt=millis();
}
void printStatus(){
 Serial.printf("{\"firmware\":%u,\"role\":\"camera\",\"source_index\":%u,\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
  "\"camera_ready\":%s,\"camera_init_error\":%d,\"sensor_pid\":%u,\"psram_bytes\":%u,\"free_psram\":%u,"
  "\"radio\":%s,\"stream\":%s,\"channel\":%u,\"battery_capable\":false,\"frame_seq\":%u,"
  "\"frames_captured\":%u,\"frames_sent\":%u,\"capture_errors\":%u,\"send_errors\":%u,\"init_errors\":%u,"
  "\"last_crc\":%u,\"uptime\":%u,\"tx_callbacks\":%u,\"send_timeouts\":%u,\"max_send_wait_ms\":%u,\"capture_age_ms\":%u,\"stale_discarded\":%u,\"format\":%u,\"width\":%u,\"height\":%u}\n",
  FIRMWARE_VERSION,SOURCE_INDEX,stationMac[0],stationMac[1],stationMac[2],stationMac[3],stationMac[4],stationMac[5],
  cameraReady?"true":"false",int(cameraInitError),sensorPid,unsigned(ESP.getPsramSize()),unsigned(ESP.getFreePsram()),
  radioReady?"true":"false",streaming?"true":"false",CHANNEL,frameSeq,framesCaptured,framesSent,
  captureErrors,sendErrors,initErrors,lastCrc,millis(),unsigned(txCallbacks.load()),sendTimeouts,maxSendWait,lastCaptureAge,staleDiscarded,lastFormat,facecamera::frameWidth(lastFormat),facecamera::frameHeight(lastFormat));
}
void pollCommands(){
 static char line[32];static size_t used=0;static bool overflow=false;
 while(Serial.available()){
  const char c=Serial.read();if(c=='\r')continue;
  if(c=='\n'){
   line[used]=0;
   if(!overflow){
    if(!strncmp(line,"sync ",5))Serial.printf("SYNC %s\n",line+5);
    else if(!strcmp(line,"frame"))captureFrame();
    else if(!strcmp(line,"radio off")){radioWanted=false;radioOff();}
    else if(!strcmp(line,"radio on")){radioWanted=true;radioOn();}
    else if(!strcmp(line,"quality color"))requestedFormat.store(1);
    else if(!strcmp(line,"quality gray"))requestedFormat.store(2);
    else if(!strcmp(line,"stream off"))streaming=false;
    else if(!strcmp(line,"stream on"))streaming=true;
    else if(strcmp(line,"status")){used=0;continue;}
    printStatus();
   }
   used=0;overflow=false;
  }else if(used<sizeof(line)-1)line[used++]=c;else overflow=true;
 }
}
} // namespace
void setup(){
 Serial.begin(115200); // Never wait for a USB reader.
 esp_read_mac(stationMac,ESP_MAC_WIFI_STA);bootSession=esp_random();
 cameraReady=initCamera();radioOn();printStatus();
}
void loop(){
 pollCommands();const uint32_t now=millis();
 if(radioWanted&&!radioReady&&uint32_t(now-lastRadioTry)>=5000)radioOn();
 if(radioReady&&uint32_t(now-lastPresenceAt)>=PRESENCE_INTERVAL_MS)sendPresence();
 if(streaming&&radioReady&&uint32_t(now-lastCaptureAt)>=FRAME_INTERVAL_MS)captureFrame();
 delay(2);
}
