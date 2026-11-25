// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_seesaw.h>
#include <esp_ota_ops.h>
#include <esp_mac.h>
#include "Controls.h"
#include "EyePixels.h"
#include "DoodlePixels.h"
#include "Telemetry.h"
#ifndef FACE_VERSION
#error FACE_VERSION must be 1 or 2
#endif
static_assert(FACE_VERSION==1||FACE_VERSION==2,"version");
SET_LOOP_TASK_STACK_SIZE(16*1024);
namespace {
using namespace feather_face;
feather_water::Telemetry telemetry;
Adafruit_ST7789 display(TFT_CS,TFT_DC,TFT_RST);
GFXcanvas16 canvas(240,135);
class Gamepad:public Adafruit_seesaw {
public:
 bool sample(int& x,int& y,uint32_t& buttons){
  uint8_t a[2],b[2],c[4];
  if(!read(SEESAW_ADC_BASE,SEESAW_ADC_CHANNEL_OFFSET+14,a,2,500)||!read(SEESAW_ADC_BASE,SEESAW_ADC_CHANNEL_OFFSET+15,b,2,500)||!read(SEESAW_GPIO_BASE,SEESAW_GPIO_BULK,c,4))return false;
  x=(a[0]<<8)|a[1];y=(b[0]<<8)|b[1];buttons=(~((uint32_t(c[0])<<24)|(uint32_t(c[1])<<16)|(uint32_t(c[2])<<8)|c[3]))&buttonsMask;
  return x<=1023&&y<=1023;
 }
} gamepad;
struct Input {int x=512,y=512;uint32_t buttons=0,at=0,samples=0,errors=0,product=0;bool ok=false;int battery=-1;};
Input input;portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
uint32_t pauseUntil=0;bool ready=false;
Input snapshot(){portENTER_CRITICAL(&mux);auto s=input;portEXIT_CRITICAL(&mux);return s;}
void sensorTask(void*){
 Input s;bool connected=false;uint32_t retry=millis()-1000,batteryAt=millis()-2000;
 for(;;){
  uint32_t pause;portENTER_CRITICAL(&mux);pause=pauseUntil;portEXIT_CRITICAL(&mux);
  if(int32_t(pause-millis())>0){vTaskDelay(pdMS_TO_TICKS(10));continue;}
  if(!connected&&millis()-retry>=1000){
   retry=millis();s.product=0;
   if(gamepad.begin(0x50,-1,false))s.product=gamepad.getVersion()>>16;
   connected=s.product==5743;if(connected)gamepad.pinModeBulk(buttonsMask,INPUT_PULLUP);
  }
  if(connected){
   s.ok=gamepad.sample(s.x,s.y,s.buttons);
   if(s.ok){s.at=micros();++s.samples;}else{++s.errors;s.buttons=0;connected=false;}
  }else{s.ok=false;s.buttons=0;}
  if(millis()-batteryAt>=2000){
   batteryAt=millis();uint8_t soc[2];Wire.beginTransmission(0x36);Wire.write(0x04);
   s.battery=Wire.endTransmission(false)==0&&Wire.requestFrom(uint8_t(0x36),size_t(2),true)==2&&Wire.readBytes(soc,2)==2?std::min(100,int(soc[0])):-1;
  }
  portENTER_CRITICAL(&mux);input=s;portEXIT_CRITICAL(&mux);
  vTaskDelay(pdMS_TO_TICKS(10));
 }
}
Controls controls(FACE_VERSION);FaceState face;DoodleAnimator animation;
uint8_t mono[1024];uint32_t frames=0,hash=0,drawMax=0,pushMax=0;
uint16_t color(uint16_t c){return (c>>8)|(c<<8);}
void draw(const Input& s){
 uint32_t begin=micros();bool valid=fresh(s.ok,s.at,begin);
 face=controls.update(s.x,s.y,s.buttons,valid);
 if(FACE_VERSION==2){animation.tick(face,millis(),true);drawDoodle(animation.current,face.expression,mono,animation.wiggle(0,millis()),animation.wiggle(1,millis()),animation.wiggle(2,millis()));}
 else{
  memset(mono,0,sizeof mono);
  for(unsigned eye=0;eye<2;++eye)for(int y=0;y<32;++y)for(int x=0;x<28;++x)if(eyeLayerPixel(face,eye,x,y)){
   int px=(eye?78:22)+x,py=eyeWhiteY(face)+y;
   if(px>=0&&px<128&&py>=0&&py<64)mono[(py/8)*128+px]|=1<<(py%8);
  }
 }
 hash=2166136261U;for(auto b:mono){hash^=b;hash*=16777619U;}
 auto* pixels=canvas.getBuffer();
 for(int y=0;y<120;++y)for(int x=0;x<240;++x){int sx=x*128/240,sy=y*64/120;pixels[(y+15)*240+x]=(mono[(sy/8)*128+sx]&(1<<(sy%8)))?0xffff:0;}
 canvas.fillRect(0,0,240,15,0);canvas.setCursor(3,3);canvas.setTextColor(color(FACE_VERSION==1?0x07ff:0xfd20));
 canvas.print("FACE V");canvas.print(FACE_VERSION);canvas.print("  ");
 if(!valid)canvas.print("CONNECT GAMEPAD");
 else{canvas.print(expressionName(face.expression));canvas.print(" ");canvas.print((s.buttons&SELECT)&&(s.buttons&START)?"BLINK":s.buttons&START?"LIDS":s.buttons&SELECT?(FACE_VERSION==1?"EYES":"MOUTH"):(FACE_VERSION==1?"PUPILS":"EYES"));}
 // Same compact gauge icon as Gravity Water, at the top right.
 auto ink=color(0xbefb);canvas.drawRect(213,3,22,7,ink);canvas.fillRect(235,5,2,3,ink);
 if(s.battery>=0)canvas.fillRect(215,5,18*s.battery/100,3,color(s.battery<=15?0xec2c:0x76f7));
 canvas.setTextColor(ink);canvas.setCursor(187,3);if(s.battery<0)canvas.print("--");else canvas.print(s.battery);
 uint32_t prepared=micros();display.startWrite();display.setAddrWindow(0,0,240,135);display.writePixels(pixels,240*135,true,true);display.endWrite();
 uint32_t end=micros();drawMax=std::max(drawMax,prepared-begin);pushMax=std::max(pushMax,end-prepared);++frames;
}
void report(){
 if(telemetry.pending())return;auto s=snapshot();auto* part=esp_ota_get_running_partition();
 telemetry.append("FACE v=%d ms=%lu frames=%lu hash=%08lx settled=%d emotion=%s app=0x%lx\n",FACE_VERSION,(unsigned long)millis(),(unsigned long)frames,(unsigned long)hash,FACE_VERSION==1||animation.settled(),expressionName(face.expression),(unsigned long)part->address);
 telemetry.append("PAD product=%lu fresh=%d samples=%lu errors=%lu raw=%d,%d buttons=0x%lx age_us=%lu\n",(unsigned long)s.product,fresh(s.ok,s.at,micros()),(unsigned long)s.samples,(unsigned long)s.errors,s.x,s.y,(unsigned long)s.buttons,(unsigned long)(micros()-s.at));
 telemetry.append("POSE pupils=%d,%d y=%d lids=%u,%u bottom=%d eye=%d brow=%d mouth=%d skew=%d jaw=%u\n",face.pupilX,face.pupilY,face.y,face.left,face.right,face.bottomLids,face.eyeBend,face.browBend,face.mouthBend,face.mouthSkew,face.jaw);
 telemetry.append("TIMING draw_max_us=%lu push_max_us=%lu heap=%lu battery=%d\n",(unsigned long)drawMax,(unsigned long)pushMax,(unsigned long)ESP.getFreeHeap(),s.battery);
}
}
void setup(){
 Serial.setTxTimeoutMs(0);Serial.begin(115200);
 pinMode(TFT_I2C_POWER,OUTPUT);digitalWrite(TFT_I2C_POWER,HIGH);delay(20);
 Wire.begin(SDA,SCL,400000);Wire.setTimeOut(15);
 pinMode(TFT_BACKLITE,OUTPUT);digitalWrite(TFT_BACKLITE,LOW);
 SPI.begin(SCK,MISO,MOSI,TFT_CS);display.init(135,240);display.setRotation(1);display.setSPISpeed(24000000);display.fillScreen(ST77XX_BLACK);analogWrite(TFT_BACKLITE,115);
 if(!canvas.getBuffer()){telemetry.append("ERR framebuffer\n");return;}
 uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_STA);
 telemetry.append("BOOT face-feather-0.1.0 v=%d factory=%02x:%02x:%02x:%02x:%02x:%02x display=240x135\n",FACE_VERSION,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
 ready=xTaskCreatePinnedToCore(sensorTask,"face-pad",4096,nullptr,2,nullptr,0)==pdPASS;
 if(!ready)telemetry.append("ERR input task\n");
}
void loop(){
 telemetry.pump(Serial);if(!ready){delay(100);return;}
 static uint32_t frameAt=0,reportAt=0;
 if(micros()-frameAt>=33333){frameAt=micros();draw(snapshot());}
 bool requested=false;
 for(int n=0;n<16&&Serial.available();++n){int c=Serial.read();if(c=='?')requested=true;if(c=='!'){portENTER_CRITICAL(&mux);pauseUntil=millis()+1500;portEXIT_CRITICAL(&mux);requested=true;}}
 if(Serial&&(requested||millis()-reportAt>=1000)){reportAt=millis();report();}
 delay(1);
}
