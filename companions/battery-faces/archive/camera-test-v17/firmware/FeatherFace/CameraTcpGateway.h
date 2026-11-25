// SPDX-License-Identifier: MIT
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include "CameraSnapshot.h"
#include "CameraSocket.h"
// Blue serves Teal snapshots and accepts Tab5 snapshots on the private AP.
// No network or radio work runs inside the display loop or a critical section.
class CameraTcpGateway{
 facecamera::FrameAssembler *frames;portMUX_TYPE &mux;WiFiServer server{4244};bool (*canRelay)();void (*qualityRequest)(unsigned);
 uint8_t pixels[facecamera::FRAME_BYTES];
 static void run(void *p){auto self=(CameraTcpGateway*)p;for(;;){self->serve();vTaskDelay(pdMS_TO_TICKS(10));}}
 static bool readAll(WiFiClient &c,uint8_t *out,size_t size,uint32_t start){size_t at=0;while(at<size&&uint32_t(millis()-start)<1500){int n=c.available();if(n>0){int got=c.read(out+at,min(size-at,size_t(n)));if(got>0)at+=got;}else if(!c.connected())break;else vTaskDelay(pdMS_TO_TICKS(2));}return at==size;}
 void serve(){
  WiFiClient client=server.accept();if(!client)return;client.setTimeout(300);
  if(client.remoteIP()!=IPAddress(192,168,4,2)){client.stop();return;}
  uint8_t request=0;if(client.readBytes(&request,1)!=1){client.stop();return;}
  uint8_t header[32];CameraSnapshotMeta m{};uint32_t started=millis();
  if(request=='Q'){uint8_t format=0;if(client.readBytes(&format,1)==1&&(format==1||format==2)){qualityRequest(format);uint8_t ack='K';writeAll(client,&ack,1,started);}client.stop();return;}
  if(request=='P'){
   bool ok=readAll(client,header,sizeof(header),started)&&decodeCameraSnapshot(header,sizeof(header),m)&&m.source==1&&readAll(client,pixels,sizeof(pixels),started);
   uint32_t elapsed=millis()-started;
   if(!ok||m.age+elapsed>=1500||facecamera::crc32(pixels,sizeof(pixels))!=m.checksum){errors++;client.stop();return;}
   bool complete=false;uint8_t packet[facecamera::PACKET_BYTES];
   portENTER_CRITICAL(&mux);uint32_t sourceTime=millis()-m.age-elapsed;
   for(unsigned i=0;i<facecamera::CHUNK_COUNT;i++){auto n=facecamera::buildChunkWithCrc(pixels,m.sequence,m.uptime,m.checksum,i,packet,sizeof(packet),m.session,1,m.format);complete=frames[1].receive(packet,n,sourceTime)||complete;}
   portEXIT_CRITICAL(&mux);
   uint8_t ack='K';writeAll(client,&ack,1,started);client.stop();
   if(complete){uploaded++;relay(m);}return;
  }
  if(request!='F'){client.stop();return;}
  bool fresh;portENTER_CRITICAL(&mux);uint32_t now=millis();
  fresh=frames[0].copyPixels(pixels,now);m.sequence=frames[0].sequence();m.uptime=frames[0].senderUptime();m.session=frames[0].senderSession();m.age=now-frames[0].completedAt();m.format=frames[0].format();portEXIT_CRITICAL(&mux);
  if(!fresh){client.stop();return;}m.checksum=facecamera::crc32(pixels,sizeof(pixels));encodeCameraSnapshot(header,m);
  bool ok=writeAll(client,header,sizeof(header),started)&&writeAll(client,pixels,sizeof(pixels),started);if(ok)sent++;client.stop();
 }
 void relay(const CameraSnapshotMeta &m){
  static constexpr uint8_t broadcast[6]={255,255,255,255,255,255};uint8_t packet[facecamera::PACKET_BYTES];
  if(!canRelay())return;
  for(unsigned copy=0;copy<2;copy++)for(unsigned i=0;i<facecamera::CHUNK_COUNT;i++){
   if(!canRelay())return;auto n=facecamera::buildChunkWithCrc(pixels,m.sequence,m.uptime,m.checksum,i,packet,sizeof(packet),m.session,1,m.format);
   if(esp_now_send(broadcast,packet,n)!=ESP_OK)errors++;vTaskDelay(pdMS_TO_TICKS(8));
  }
  relayed++;
 }
 static bool writeAll(WiFiClient &c,const uint8_t *data,size_t size,uint32_t start){return writeCameraSocket(c.fd(),data,size,start,2000,[](){return millis();},[](){vTaskDelay(pdMS_TO_TICKS(2));});}
 public:volatile uint32_t sent=0,uploaded=0,relayed=0,errors=0;
 CameraTcpGateway(facecamera::FrameAssembler *f,portMUX_TYPE &m,bool (*allowed)(),void (*quality)(unsigned)):frames(f),mux(m),canRelay(allowed),qualityRequest(quality){}
 void begin(){server.begin();xTaskCreate(run,"camera-tcp",4096,this,1,nullptr);}
};
