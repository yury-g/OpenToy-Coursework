// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <Wire.h>
#include <Adafruit_seesaw.h>
#include <SPI.h>
#include <esp_ota_ops.h>
#include <esp_mac.h>
// The build helper stages the canonical shared model and renderer.
#include "src/fluid/Particles.h"
#include "src/fluid/Gravity.h"
#include "src/fluid/Motion.h"
#include "NessoPixels.h"
#include "WaterOverlay.h"
#include "FeatherAxes.h"
#include "Telemetry.h"
#include "GamepadCursor.h"
SET_LOOP_TASK_STACK_SIZE(16*1024);
namespace {
feather_water::Telemetry telemetry;
constexpr int width=135,height=240;
constexpr char version[]="gravity-water-feather-v0.3.0";
constexpr char shortVersion[]="0.3.0",created[]="2026-11-27";
Adafruit_ST7789 display(TFT_CS,TFT_DC,TFT_RST);
GFXcanvas16 canvas(width,height);
Adafruit_LSM6DS3TRC imu;
// Use checked reads: failed I2C must never turn into a stuck virtual finger.
class Gamepad : public Adafruit_seesaw {
public:
 bool sample(int& x,int& y,uint32_t& buttons) {
  uint8_t a[2],b[2],c[4];
  if(!read(SEESAW_ADC_BASE,SEESAW_ADC_CHANNEL_OFFSET+14,a,2,500)
   ||!read(SEESAW_ADC_BASE,SEESAW_ADC_CHANNEL_OFFSET+15,b,2,500)
   ||!read(SEESAW_GPIO_BASE,SEESAW_GPIO_BULK,c,4))return false;
  x=(a[0]<<8)|a[1];y=(b[0]<<8)|b[1];
  buttons=(uint32_t(c[0])<<24)|(uint32_t(c[1])<<16)|(uint32_t(c[2])<<8)|c[3];
  return x<=1023&&y<=1023;
 }
} gamepad;
constexpr uint32_t padMask=(1UL<<0)|(1UL<<1)|(1UL<<2)|(1UL<<5)|(1UL<<6)|(1UL<<16);
bool padReady=false;
gravity_water::Particles<width,height,400> particles;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct State {
 gravity_water::Gravity gravity;gravity_water::Motion motion;
 float ax=0,ay=0,az=0;
 uint32_t samples=0,lastUs=0,shakeUs=0,stirs=0,presses=0,maxGap=0;
 bool heating=false;int battery=-1;
 bool padOk=false,padTouch=false,padHeat=false;int padX=67,padY=160,stickX=512,stickY=512;
 uint32_t padUs=0,padSamples=0,padErrors=0,padTouches=0,padButtons=0;
} state;
uint8_t imuAddress=0;bool ready=false;
uint32_t frames=0,steps=0,shockFrames=0,lastShake=0,lastStir=0;
uint32_t drawMax=0,pushMax=0,sampleAgeMax=0,shakeAgeMax=0;
uint64_t drawTotal=0,pushTotal=0;
State snapshot(){portENTER_CRITICAL(&mux);auto s=state;portEXIT_CRITICAL(&mux);return s;}
bool readRegister(uint8_t address,uint8_t reg,uint8_t *data,size_t length) {
 Wire.beginTransmission(address);Wire.write(reg);
 if(Wire.endTransmission(false)!=0)return false;
 if(Wire.requestFrom(address,length,true)!=length)return false;
 return Wire.readBytes(data,length)==length;
}
int16_t signed16(const uint8_t* p){return int16_t(uint16_t(p[0])|(uint16_t(p[1])<<8));}
struct Button {
 bool candidate=false,held=false;uint32_t changed=0;
 bool sample(bool raw,uint32_t now){if(raw!=candidate){candidate=raw;changed=now;}if(now-changed>=8)held=candidate;return held;}
};
void sensorTask(void*) {
 State s;Button heat,stir;bool previousStir=false;uint32_t batteryAt=millis()-2000;
 feather_water::GamepadCursor cursor;Button padHeat,padStir;bool oldPadStir=false;
 uint32_t padAt=micros();
 for(;;) {
  uint8_t status=0,data[12];uint32_t now=micros();
  if(imuAddress && readRegister(imuAddress,LSM6DS_STATUS_REG,&status,1) && (status&3)==3
     && readRegister(imuAddress,LSM6DS_OUTX_L_G,data,sizeof(data))) {
   now=micros();uint32_t gap=s.lastUs?now-s.lastUs:4808;
   if(s.lastUs)s.maxGap=std::max(s.maxGap,gap);
   s.lastUs=now;++s.samples;
   s.ax=signed16(data+6)*.000122f;s.ay=signed16(data+8)*.000122f;s.az=signed16(data+10)*.000122f;
   float gx=signed16(data)*.0175f,gy=signed16(data+2)*.0175f,gz=signed16(data+4)*.0175f;
   auto down=feather_water::screenDown(s.ax,s.ay);
   s.gravity.sample(down.x,down.y,gap*.000001f);
   uint32_t sequence=s.motion.sequence;
   s.motion.sample(down.x,down.y,s.az,std::sqrt(gx*gx+gy*gy+gz*gz),gap*.000001f);
   if(sequence!=s.motion.sequence)s.shakeUs=now;
  }
  uint32_t padNow=micros();
  if(padReady&&padNow-padAt>=10000){
   float dt=(padNow-padAt)*.000001f;padAt=padNow;uint32_t buttons;int x,y;
   s.padOk=gamepad.sample(x,y,buttons);
   if(s.padOk){
    s.padUs=micros();++s.padSamples;s.stickX=x;s.stickY=y;s.padButtons=(~buttons)&padMask;
    bool touching=cursor.update(x,y,s.padButtons&(1UL<<5),s.padButtons&(1UL<<2),dt);
    if(touching&&!s.padTouch)++s.padTouches;s.padTouch=touching;
    s.padX=cursor.x();s.padY=cursor.y();
   } else {++s.padErrors;s.padTouch=false;s.padButtons=0;}
   s.padHeat=padHeat.sample(s.padOk&&(s.padButtons&(1UL<<1)),millis());
   bool scatter=padStir.sample(s.padOk&&(s.padButtons&(1UL<<6)),millis());
   if(scatter&&!oldPadStir)++s.stirs;oldPadStir=scatter;
  }
  bool held=heat.sample(digitalRead(1)==HIGH,millis())||(s.padOk&&s.padHeat);
  if(held&&!s.heating)++s.presses;s.heating=held;
  bool stirred=stir.sample(digitalRead(2)==HIGH,millis());
  if(stirred&&!previousStir)++s.stirs;previousStir=stirred;
  if(millis()-batteryAt>=2000){batteryAt=millis();uint8_t soc[2];
   s.battery=readRegister(0x36,0x04,soc,2)?std::min(100,int(soc[0])):-1;
  }
  portENTER_CRITICAL(&mux);state=s;portEXIT_CRITICAL(&mux);
  vTaskDelay(1);
 }
}
uint16_t rgb(int r,int g,int b){return nesso_water::pixelColor(r,g,b);}
void pushRows(int top,int bottom) {
 display.startWrite();display.setAddrWindow(0,top,width,bottom-top+1);
 display.writePixels(canvas.getBuffer()+top*width,width*(bottom-top+1),true,true);display.endWrite();
}
void draw(const State& s) {
 uint32_t start=micros();static uint32_t previous=start,accumulator=0;
 accumulator=std::min<uint32_t>(49998,accumulator+uint32_t(start-previous));previous=start;
 bool fresh=s.lastUs&&start-s.lastUs<100000;
 auto d=s.gravity.down();float strength=fresh?s.gravity.strength():0;
 bool padFresh=s.padOk&&start-s.padUs<100000,touch=padFresh&&s.padTouch;
 if(touch)particles.repel(s.padX,s.padY);
 if(fresh&&s.motion.sequence!=lastShake){lastShake=s.motion.sequence;particles.burst(s.motion.kickX,s.motion.kickY,s.motion.power);++shockFrames;shakeAgeMax=std::max(shakeAgeMax,start-s.shakeUs);}
 if(s.stirs!=lastStir){lastStir=s.stirs;particles.burst(0,-800,220);}
 while(accumulator>=16666){particles.step(int(d.x*strength*256),int(d.y*strength*256),s.heating,(fresh&&s.motion.moving)||touch);accumulator-=16666;++steps;}
 nesso_water::drawParticles(canvas.getBuffer(),width,height,0,particles);
 if(padFresh){
  canvas.drawCircle(s.padX,s.padY,5,touch?rgb(255,216,126):rgb(140,180,192));
  canvas.drawPixel(s.padX,s.padY,rgb(240,248,246));
 }
 nesso_water::drawWaterOverlay(canvas.getBuffer(),width,height,"WATER",shortVersion,created,s.battery,fresh,true);
 uint32_t prepared=micros();
 static nesso_water::WaterDirtyTracker dirty;
 auto rows=nesso_water::occupiedRows(particles,0,height);
 if(padFresh){rows.first=std::min(rows.first,std::max(0,s.padY-5));rows.last=std::max(rows.last,std::min(height-1,s.padY+5));}
 nesso_water::DirtySpan spanA,spanB;
 dirty.compute(rows,height,s.battery,fresh,true,spanA,spanB);
 if(spanA.valid())pushRows(spanA.first,spanA.last);
 if(spanB.valid())pushRows(spanB.first,spanB.last);
 uint32_t end=micros();drawTotal+=prepared-start;pushTotal+=end-prepared;
 drawMax=std::max(drawMax,prepared-start);pushMax=std::max(pushMax,end-prepared);
 if(s.lastUs)sampleAgeMax=std::max(sampleAgeMax,end-s.lastUs);++frames;
}
void report() {
 if(telemetry.pending())return;
 static uint32_t at=0,oldSamples=0,oldFrames=0,oldSteps=0;
 auto s=snapshot();uint32_t now=millis(),elapsed=now-at;if(!elapsed)return;
 auto d=s.gravity.down();
 telemetry.append("WATER build=%s ms=%lu sensor_hz=%.1f fps=%.1f sim_hz=%.1f frames=%lu fresh=%d battery=%d\n",version,(unsigned long)now,(s.samples-oldSamples)*1000.f/elapsed,(frames-oldFrames)*1000.f/elapsed,(steps-oldSteps)*1000.f/elapsed,(unsigned long)frames,s.lastUs&&micros()-s.lastUs<100000,s.battery);
 telemetry.append("PLAY heat=%d energy=%d sleeping=%d held=%d presses=%lu stirs=%lu shakes=%lu shock_frames=%lu accel=%.4f,%.4f,%.4f down=%.4f,%.4f strength=%.3f\n",particles.heat(),particles.meanEnergy(),particles.sleeping(),s.heating,(unsigned long)s.presses,(unsigned long)s.stirs,(unsigned long)s.motion.sequence,(unsigned long)shockFrames,s.ax,s.ay,s.az,d.x,d.y,s.gravity.strength());
 telemetry.append("TIMING draw_avg_us=%llu push_avg_us=%llu draw_max_us=%lu push_max_us=%lu sample_age_max_us=%lu gap_max_us=%lu shake_apply_max_us=%lu heap=%lu\n",frames?drawTotal/frames:0,frames?pushTotal/frames:0,(unsigned long)drawMax,(unsigned long)pushMax,(unsigned long)sampleAgeMax,(unsigned long)s.maxGap,(unsigned long)shakeAgeMax,(unsigned long)ESP.getFreeHeap());
 telemetry.append("PAD ready=%d fresh=%d samples=%lu errors=%lu raw=%d,%d cursor=%d,%d touch=%d touches=%lu buttons=0x%lx\n",padReady,s.padOk&&micros()-s.padUs<100000,(unsigned long)s.padSamples,(unsigned long)s.padErrors,s.stickX,s.stickY,s.padX,s.padY,s.padTouch,(unsigned long)s.padTouches,(unsigned long)s.padButtons);
 at=now;oldSamples=s.samples;oldFrames=frames;oldSteps=steps;
}
}
void setup() {
 Serial.setTxTimeoutMs(0);Serial.begin(115200);
 pinMode(1,INPUT_PULLDOWN);pinMode(2,INPUT_PULLDOWN);
 pinMode(TFT_I2C_POWER,OUTPUT);digitalWrite(TFT_I2C_POWER,HIGH);delay(20);
 Wire.begin(SDA,SCL,400000);Wire.setTimeOut(15);
 for(uint8_t address:{uint8_t(0x6a),uint8_t(0x6b)})if(imu.begin_I2C(address,&Wire)){imuAddress=address;break;}
 if(imuAddress){imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);imu.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);imu.setAccelDataRate(LSM6DS_RATE_208_HZ);imu.setGyroDataRate(LSM6DS_RATE_208_HZ);}
 // Verify the gamepad product before configuring its buttons. No accessory firmware writes.
 padReady=gamepad.begin(0x50,-1,false)&&((gamepad.getVersion()>>16)==5743);
 if(padReady)gamepad.pinModeBulk(padMask,INPUT_PULLUP);
 pinMode(TFT_BACKLITE,OUTPUT);digitalWrite(TFT_BACKLITE,LOW);
 SPI.begin(SCK,MISO,MOSI,TFT_CS);display.init(width,height);display.setRotation(0);display.setSPISpeed(24000000);display.fillScreen(ST77XX_BLACK);analogWrite(TFT_BACKLITE,115);
 if(!canvas.getBuffer()){telemetry.append("ERR framebuffer\n");return;}
 auto *part=esp_ota_get_running_partition();uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_STA);
 telemetry.append("BOOT %s factory=%02x:%02x:%02x:%02x:%02x:%02x app=0x%lx size=0x%lx imu=0x%02x display=135x240 overlay=1 buttons=D1:heat,D2:stir\n",version,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],(unsigned long)part->address,(unsigned long)part->size,imuAddress);
 if(!imuAddress)telemetry.append("NOTE IMU missing; check STEMMA QT cable\n");
 ready=xTaskCreatePinnedToCore(sensorTask,"water-sensor",4096,nullptr,2,nullptr,0)==pdPASS;
 if(!ready)telemetry.append("ERR sensor task\n");
}
void loop() {
 telemetry.pump(Serial);
 if(!ready){delay(100);return;}
 static uint32_t lastFrame=0,lastReport=0;uint32_t now=micros();
 if(now-lastFrame>=16667){lastFrame=now;draw(snapshot());}
 bool requested=false;for(int i=0;i<16&&Serial.available();i++)if(Serial.read()=='?')requested=true;
 if(Serial&&(millis()-lastReport>=2000||requested)){lastReport=millis();report();}
 delay(1);
}
