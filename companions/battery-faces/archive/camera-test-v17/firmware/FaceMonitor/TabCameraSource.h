// SPDX-License-Identifier: MIT
#pragma once
#include "TabCamera.h"
#include "../FeatherFace/CameraSnapshot.h"
#include "../FeatherFace/CameraSocket.h"
// Native capture and upload have a single background owner. UI reads a locked
// copy of status, never driver state while an interrupt may be updating it.
class TabCameraSource{
 TabCamera camera;facecamera::FrameAssembler &frames;portMUX_TYPE &mux;
 uint8_t pixels[facecamera::FRAME_BYTES];uint32_t sequence=0,session=0;
 bool enabled_=true;uint32_t sent_=0,errors_=0,captured_=0,failed_=0,clock_=0;uint16_t chip_=0;bool ready_=false;const char *stage_="starting";esp_err_t error_=ESP_OK;
 static void run(void *p){((TabCameraSource*)p)->work();}
 void snapshot(){portENTER_CRITICAL(&mux);ready_=camera.ready();chip_=camera.chipId();stage_=camera.stage();error_=camera.error();captured_=camera.framesCaptured();failed_=camera.captureErrors();clock_=camera.clockHz();portEXIT_CRITICAL(&mux);}
 bool enabled(){portENTER_CRITICAL(&mux);bool e=enabled_;portEXIT_CRITICAL(&mux);return e;}
 void work(){
  session=esp_random();uint32_t lastBegin=millis()-5000;
  for(;;){
   if(enabled()&&!camera.ready()&&uint32_t(millis()-lastBegin)>=5000){lastBegin=millis();camera.begin();snapshot();}
   if(enabled()&&camera.ready()){
    if(camera.capture(pixels,sizeof(pixels),500)){
     CameraSnapshotMeta m{++sequence,camera.lastCaptureAt(),session,facecamera::crc32(pixels,sizeof(pixels)),uint32_t(millis()-camera.lastCaptureAt()),1};uint8_t packet[facecamera::PACKET_BYTES];
     portENTER_CRITICAL(&mux);uint32_t sourceTime=millis()-m.age;
     for(unsigned i=0;i<facecamera::CHUNK_COUNT;i++){auto n=facecamera::buildChunkWithCrc(pixels,m.sequence,m.uptime,m.checksum,i,packet,sizeof(packet),m.session,1);frames.receive(packet,n,sourceTime);}portEXIT_CRITICAL(&mux);
     upload(m);
    }
    snapshot();
   }
   vTaskDelay(pdMS_TO_TICKS(1800));
  }
 }
 void upload(CameraSnapshotMeta m){
  if(WiFi.status()!=WL_CONNECTED)return;WiFiClient c;uint32_t start=millis();
  if(!c.connect(IPAddress(192,168,4,1),4244,500)){portENTER_CRITICAL(&mux);errors_++;portEXIT_CRITICAL(&mux);return;}
  m.age=millis()-m.uptime;uint8_t header[33];header[0]='P';encodeCameraSnapshot(header+1,m);
  bool ok=writeCameraSocket(c.fd(),header,sizeof(header),start,1000,[](){return millis();},[](){vTaskDelay(pdMS_TO_TICKS(2));})&&writeCameraSocket(c.fd(),pixels,sizeof(pixels),start,1000,[](){return millis();},[](){vTaskDelay(pdMS_TO_TICKS(2));});
  while(ok&&!c.available()&&c.connected()&&uint32_t(millis()-start)<1500)vTaskDelay(pdMS_TO_TICKS(2));
  ok=ok&&c.available()&&c.read()=='K';c.stop();portENTER_CRITICAL(&mux);if(ok)sent_++;else errors_++;portEXIT_CRITICAL(&mux);
 }
 public:
 TabCameraSource(facecamera::FrameAssembler &f,portMUX_TYPE &m):frames(f),mux(m){}
 void begin(){xTaskCreate(run,"tab5-capture",6144,this,1,nullptr);}
 void setEnabled(bool enable){portENTER_CRITICAL(&mux);enabled_=enable;portEXIT_CRITICAL(&mux);}
 void report(){
  portENTER_CRITICAL(&mux);bool r=ready_,e=enabled_;uint16_t chip=chip_;auto stage=stage_;int error=error_;uint32_t captured=captured_,failed=failed_,sent=sent_,errors=errors_,clock=clock_;portEXIT_CRITICAL(&mux);
  Serial.printf("TAB_CAMERA ready=%d enabled=%d chip=%04x stage=%s error=%d captured=%lu failed=%lu uploaded=%lu upload_errors=%lu clock_hz=%lu\n",r,e,chip,stage,error,(unsigned long)captured,(unsigned long)failed,(unsigned long)sent,(unsigned long)errors,(unsigned long)clock);
 }
};
