// SPDX-License-Identifier: MIT
#pragma once
#include <WiFi.h>
#include "../FeatherFace/CameraSnapshot.h"
#include "../FeatherFace/CameraSocket.h"
class CameraTcpReceiver{
 facecamera::FrameAssembler &frames;portMUX_TYPE &mux;uint8_t pixels[facecamera::FRAME_BYTES];unsigned pendingFormat=0;
 static void run(void *p){auto self=(CameraTcpReceiver*)p;for(;;){self->fetch();vTaskDelay(pdMS_TO_TICKS(1200));}}
 static bool readAll(WiFiClient &c,uint8_t *data,size_t size,uint32_t start){size_t at=0;while(at<size&&uint32_t(millis()-start)<2500){int n=c.available();if(n>0){int got=c.read(data+at,min(size-at,size_t(n)));if(got>0)at+=got;}else if(!c.connected())break;else vTaskDelay(pdMS_TO_TICKS(2));}return at==size;}
 void fetch(){
  if(WiFi.status()!=WL_CONNECTED)return;
  portENTER_CRITICAL(&mux);unsigned requested=pendingFormat;pendingFormat=0;portEXIT_CRITICAL(&mux);
  if(requested){WiFiClient q;uint32_t at=millis();if(q.connect(IPAddress(192,168,4,1),4244,500)){uint8_t command[]={'Q',uint8_t(requested)};writeCameraSocket(q.fd(),command,2,at,1000,[](){return millis();},[](){vTaskDelay(pdMS_TO_TICKS(2));});q.stop();}}
  WiFiClient c;uint32_t start=millis();
  if(!c.connect(IPAddress(192,168,4,1),4244,1000)){errors++;return;}c.setTimeout(1000);uint8_t request='F';if(!writeCameraSocket(c.fd(),&request,1,start,1500,[](){return millis();},[](){vTaskDelay(pdMS_TO_TICKS(2));})){c.stop();errors++;return;}
  uint8_t header[32];CameraSnapshotMeta m{};
  bool ok=readAll(c,header,sizeof(header),start)&&decodeCameraSnapshot(header,sizeof(header),m)&&m.source==0&&readAll(c,pixels,sizeof(pixels),start);c.stop();
  uint32_t elapsed=millis()-start;if(!ok||elapsed>=2500||m.age+elapsed>=facecamera::FRAME_TTL_MS){errors++;return;}
  // Preserve source age conservatively, including the whole request duration.
  uint32_t sourceTime=millis()-m.age-elapsed;uint8_t packet[facecamera::PACKET_BYTES];
  portENTER_CRITICAL(&mux);
  for(unsigned i=0;i<facecamera::CHUNK_COUNT;i++){size_t n=facecamera::buildChunkWithCrc(pixels,m.sequence,m.uptime,m.checksum,i,packet,sizeof(packet),m.session,0,m.format);frames.receive(packet,n,sourceTime);}
  portEXIT_CRITICAL(&mux);received++;
 }
 public:volatile uint32_t received=0,errors=0;
 CameraTcpReceiver(facecamera::FrameAssembler &f,portMUX_TYPE &m):frames(f),mux(m){}
 void requestFormat(unsigned format){portENTER_CRITICAL(&mux);pendingFormat=format;portEXIT_CRITICAL(&mux);}
 void begin(){xTaskCreate(run,"camera-fetch",4096,this,1,nullptr);}
};
