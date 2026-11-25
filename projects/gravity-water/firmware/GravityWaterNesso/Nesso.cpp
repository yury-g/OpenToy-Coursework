// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <M5Unified.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <atomic>
#include <driver/ledc.h>
#include <hal/ledc_ll.h>
#include "src/fluid/PwmEnvelope.h"
#include <esp_timer.h>
#include "src/fluid/Motion.h"
#include "src/fluid/TextureSound.h"
#include "src/fluid/Gravity.h"
#include "src/fluid/Particles.h"
#include "NessoPixels.h"
#include "NessoAxes.h"
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
using namespace gravity_water;
namespace {
constexpr int width=135,height=240,bar=13;
constexpr uint32_t framePeriod=16667;
constexpr char version[]="gravity-water-v0.3.0";
M5Canvas canvas(&M5.Display);
Particles<width,height-bar,400> particles;
uint32_t simulationSteps=0,simulationMax=0;
uint64_t simulationTotal=0;
uint32_t accumulator=0;
portMUX_TYPE stateMux=portMUX_INITIALIZER_UNLOCKED;
struct State {
 Gravity gravity;
 Motion motion;
 bool heating=false,touched=false;
 int16_t touchX=0,touchY=0;
 uint32_t presses=0,touches=0,shakeUs=0;
 uint32_t samples=0,lastUs=0,minGap=UINT32_MAX,maxGap=0,maxRead=0;
 float ax=0,ay=0,az=0;
 int battery=-1;
} state;
bool ready=false,audioReady=false;
std::atomic<uint32_t> audioLevels{0},audioTicks{0},audioGaps{0};
std::atomic<uint32_t> audioMaxUs{0};
uint32_t shakeApplied=0,shakeAgeMax=0,shockFrames=0;
TextureSound texture;
PwmEnvelope soundEnvelope;
esp_timer_handle_t soundTimer=nullptr;
// Direct PWM avoids the speaker driver's indefinitely waiting DMA queue.
// Four 8kHz synthesis samples averaged into one 2kHz output tick.
// Fixed work and a timer skip policy limit catch-up after scheduling delays.
void soundTick(void*) {
 uint32_t begin=micros();static uint32_t previous=0;static int lastDuty=-1;
 uint32_t levels=audioLevels.load(std::memory_order_relaxed);
 int16_t samples[4];texture.render(samples,4,levels&255,(levels>>8)&255,levels&(1<<16));
 int sample=(int(samples[0])+samples[1]+samples[2]+samples[3])/4;
 bool active=!texture.quiet();
 if(active && previous && begin-previous>1500)++audioGaps;
 previous=begin;
 int duty=soundEnvelope.step(sample,active);
 if(duty!=lastDuty) {
  // This callback exclusively owns the channel after setup; no fade ISR is used.
  ledc_ll_set_duty_int_part(LEDC_LL_GET_HW(),LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty);
  ledc_ll_set_duty_start(LEDC_LL_GET_HW(),LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
  ledc_ll_ls_channel_update(LEDC_LL_GET_HW(),LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
  lastDuty=duty;
 }
 ++audioTicks;
 uint32_t cost=micros()-begin;if(cost>audioMaxUs.load())audioMaxUs=cost;
}
bool startSound() {
 ledc_timer_config_t timer={};timer.speed_mode=LEDC_LOW_SPEED_MODE;timer.duty_resolution=LEDC_TIMER_8_BIT;
 timer.timer_num=LEDC_TIMER_0;timer.freq_hz=31250;timer.clk_cfg=LEDC_AUTO_CLK;
 if(ledc_timer_config(&timer)!=ESP_OK)return false;
 ledc_channel_config_t channel={};channel.gpio_num=11;channel.speed_mode=LEDC_LOW_SPEED_MODE;
 channel.channel=LEDC_CHANNEL_0;channel.intr_type=LEDC_INTR_DISABLE;channel.timer_sel=LEDC_TIMER_0;channel.duty=0;
 if(ledc_channel_config(&channel)!=ESP_OK)return false;
 esp_timer_create_args_t args={};args.callback=soundTick;args.name="water-sound";args.skip_unhandled_events=true;
 if(esp_timer_create(&args,&soundTimer)!=ESP_OK)return false;
 if(esp_timer_start_periodic(soundTimer,500)!=ESP_OK){esp_timer_delete(soundTimer);soundTimer=nullptr;return false;}
 return true;
}
uint8_t imuAddress=0;
uint32_t frames=0,drawMax=0,pushMax=0,ageMax=0,frameGapMax=0,lastFrame=0;
uint64_t drawTotal=0,pushTotal=0,ageTotal=0;
uint32_t reportAt=0,previousFrames=0,previousSamples=0,previousSteps=0;
State snapshot() {
 portENTER_CRITICAL(&stateMux); State copy=state; portEXIT_CRITICAL(&stateMux);
 return copy;
}
bool configureImu() {
 if(!M5.Imu.isEnabled() || M5.Imu.getType()!=m5::imu_t::imu_bmi270) return false;
 for(uint8_t a: {uint8_t(0x68),uint8_t(0x69)}) {
  if(M5.In_I2C.readRegister8(a,0x00,400000)==0x24) {imuAddress=a; break;}
 }
 if(!imuAddress) return false;
 // Bosch BMI270 ACC_CONF: performance filtering, normal bandwidth, 200 Hz.
 // GYR_CONF: normal bandwidth, performance noise/filter, 200 Hz.
 bool ok=M5.In_I2C.writeRegister8(imuAddress,0x40,0xa9,400000);
 ok &= M5.In_I2C.writeRegister8(imuAddress,0x42,0xe9,400000);
 M5.Imu.setCalibration(0,0,0);
 return ok && M5.In_I2C.readRegister8(imuAddress,0x40,400000)==0xa9
           && M5.In_I2C.readRegister8(imuAddress,0x42,400000)==0xe9;
}
void sensorTask(void*) {
 State s; uint32_t batteryAt=millis()-2000,inputAt=0;
 for(;;) {
  uint32_t begin=micros();
  const auto mask=M5.Imu.update();
  if(mask & m5::IMU_Class::sensor_mask_accel) {
   auto data=M5.Imu.getImuData();auto a=data.accel;auto gyro=data.gyro;
   const uint32_t now=micros(), gap=s.lastUs?now-s.lastUs:5000;
   if(s.lastUs) {s.minGap=std::min(s.minGap,gap);s.maxGap=std::max(s.maxGap,gap);}
   s.maxRead=std::max(s.maxRead,now-begin);
   s.lastUs=now; ++s.samples; s.ax=a.x; s.ay=a.y; s.az=a.z;
   const auto down=nesso_water::screenDown(a.x,a.y);
   s.gravity.sample(down.x,down.y,gap*.000001f);
   uint32_t previousShake=s.motion.sequence;
   s.motion.sample(down.x,down.y,a.z,std::sqrt(gyro.x*gyro.x+gyro.y*gyro.y+gyro.z*gyro.z),gap*.000001f);
   if(s.motion.sequence!=previousShake)s.shakeUs=now;
  }
  if(millis()-inputAt>=10) {
   inputAt=millis();M5.update();
   bool held=M5.BtnA.isPressed();if(held && !s.heating)++s.presses;s.heating=held;
   bool touch=false;
   if(M5.Touch.getCount()) {
    const auto &point=M5.Touch.getDetail(0);
    if(point.isPressed() && point.y>=bar && point.y<height && point.x>=0 && point.x<width) {
     touch=true;s.touchX=point.x;s.touchY=point.y-bar;
    }
   }
   if(touch && !s.touched)++s.touches;s.touched=touch;
  }
  if(millis()-batteryAt>=2000) {batteryAt=millis();s.battery=M5.Power.getBatteryLevel();}
  portENTER_CRITICAL(&stateMux); state=s; portEXIT_CRITICAL(&stateMux);
  vTaskDelay(1);
 }
}
uint16_t rgb(int r,int g,int b) {return canvas.color565(r,g,b);}
void drawBar(int battery,bool fresh) {
 const uint16_t bg=rgb(7,17,25),ink=rgb(192,225,222);
 canvas.fillRect(0,0,width,bar,bg);
 canvas.drawFastHLine(0,bar-1,width,rgb(28,59,70));
 // Tiny face, with neutral/unknown states between full and low.
 canvas.drawPixel(6,4,ink); canvas.drawPixel(10,4,ink);
 if(battery>=70) {canvas.drawPixel(5,7,ink);canvas.drawFastHLine(6,8,5,ink);canvas.drawPixel(11,7,ink);}
 else if(battery>=0 && battery<=15) {canvas.drawPixel(5,8,ink);canvas.drawFastHLine(6,7,5,ink);canvas.drawPixel(11,8,ink);}
 else canvas.drawFastHLine(6,8,5,ink);
 if(!fresh) {canvas.setTextColor(ink,bg);canvas.setCursor(18,2);canvas.print("IMU?");}
 canvas.drawRect(width-27,3,22,7,ink); canvas.fillRect(width-5,5,2,3,ink);
 if(battery>=0) canvas.fillRect(width-25,5,(18*std::min(100,battery))/100,3,battery<=15?rgb(239,133,100):rgb(115,222,188));
 canvas.setTextColor(ink,bg);canvas.setCursor(width-53,2);
 if(battery<0) canvas.print("--"); else canvas.printf("%d",battery);
}
void draw(State s) {
 const uint32_t start=micros();
 static uint32_t previous=start;
 accumulator=std::min<uint32_t>(accumulator+uint32_t(start-previous),49998);
 previous=start;
 auto direction=s.gravity.down();
 float strength=(s.lastUs && start-s.lastUs<100000)?s.gravity.strength():0;
 int gx=int(direction.x*strength*256),gy=int(direction.y*strength*256);
 uint32_t simStart=micros();
 bool fresh=s.lastUs && start-s.lastUs<100000;
 if(fresh && s.motion.sequence!=shakeApplied) {
  shakeApplied=s.motion.sequence;
  particles.burst(s.motion.kickX,s.motion.kickY,s.motion.power);
  ++shockFrames;shakeAgeMax=std::max(shakeAgeMax,start-s.shakeUs);
 }
 if(fresh && s.touched)particles.repel(s.touchX,s.touchY);
 while(accumulator>=16666) {
  particles.step(gx,gy,fresh && s.heating,fresh && (s.motion.moving||s.touched));accumulator-=16666;++simulationSteps;
 }
 uint32_t simTime=micros()-simStart;
 simulationTotal+=simTime;simulationMax=std::max(simulationMax,simTime);
 audioLevels=uint32_t(particles.heat())|(uint32_t(particles.meanEnergy())<<8)|(uint32_t(fresh && s.touched)<<16);
 nesso_water::drawParticles(static_cast<uint16_t*>(canvas.getBuffer()),width,height,bar,particles);
 drawBar(s.battery,s.lastUs && start-s.lastUs<100000);
 const uint32_t prepared=micros();
 // Clear old bead locations as well as drawing new ones; omit unchanged sky.
 static nesso_water::Rows previousRows{bar,height-1};
 static int previousBattery=-999;static bool previousFresh=false;
 auto rows=nesso_water::occupiedRows(particles,bar,height);
 int top=std::min(rows.first,previousRows.first),bottom=std::max(rows.last,previousRows.last);
 const auto *pixels=static_cast<const lgfx::swap565_t*>(canvas.getBuffer());
 if(!frames || previousBattery!=s.battery || previousFresh!=fresh)
  M5.Display.pushImage(0,0,width,bar,pixels);
 M5.Display.pushImage(0,top,width,bottom-top+1,pixels+top*width);
 previousRows=rows;previousBattery=s.battery;previousFresh=fresh;
 const uint32_t end=micros();
 uint32_t drawUs=prepared-start,pushUs=end-prepared;
 drawTotal+=drawUs;pushTotal+=pushUs;drawMax=std::max(drawMax,drawUs);pushMax=std::max(pushMax,pushUs);
 if(s.lastUs) {uint32_t age=end-s.lastUs; ageTotal+=age;ageMax=std::max(ageMax,age);}
 ++frames;
}
void report() {
 State s=snapshot(); uint32_t now=millis(), elapsed=now-reportAt;
 if(!elapsed) return;
 Serial.printf("WATER build=%s ms=%lu samples=%lu frames=%lu sensor_hz=%.1f fps=%.1f sim_hz=%.1f battery=%d fresh=%d\n",version,(unsigned long)now,(unsigned long)s.samples,(unsigned long)frames,(s.samples-previousSamples)*1000.f/elapsed,(frames-previousFrames)*1000.f/elapsed,(simulationSteps-previousSteps)*1000.f/elapsed,s.battery,s.lastUs && micros()-s.lastUs<100000);
 Serial.printf("TIMING gap_min_us=%lu gap_max_us=%lu read_max_us=%lu draw_avg_us=%llu draw_max_us=%lu push_avg_us=%llu push_max_us=%lu age_avg_us=%llu age_max_us=%lu frame_gap_max_us=%lu\n",(unsigned long)s.minGap,(unsigned long)s.maxGap,(unsigned long)s.maxRead,frames?drawTotal/frames:0,(unsigned long)drawMax,frames?pushTotal/frames:0,(unsigned long)pushMax,frames?ageTotal/frames:0,(unsigned long)ageMax,(unsigned long)frameGapMax);
 auto d=s.gravity.down();
 Serial.printf("GRAVITY accel=%.4f,%.4f,%.4f down=%.4f,%.4f strength=%.3f sim_steps=%lu sim_avg_us=%llu sim_max_us=%lu particles=400\n",s.ax,s.ay,s.az,d.x,d.y,s.gravity.strength(),(unsigned long)simulationSteps,frames?simulationTotal/frames:0,(unsigned long)simulationMax);
 Serial.printf("PLAY heat=%d energy=%d sleeping=%d held=%d touches=%lu presses=%lu shakes=%lu shock_frames=%lu shake_apply_max_us=%lu audio=%d audio_ticks=%lu audio_gaps=%lu audio_max_us=%lu heap=%lu\n",particles.heat(),particles.meanEnergy(),particles.sleeping(),s.heating,(unsigned long)s.touches,(unsigned long)s.presses,(unsigned long)s.motion.sequence,(unsigned long)shockFrames,(unsigned long)shakeAgeMax,audioReady,(unsigned long)audioTicks.load(),(unsigned long)audioGaps.load(),(unsigned long)audioMaxUs.load(),(unsigned long)ESP.getFreeHeap());
 reportAt=now;previousSamples=s.samples;previousFrames=frames;previousSteps=simulationSteps;
}
}
void setup() {
 Serial.setTxBufferSize(2048); Serial.setTxTimeoutMs(0); Serial.begin(115200);
 auto cfg=M5.config();cfg.internal_imu=true;cfg.internal_spk=false;cfg.internal_mic=false;
 M5.begin(cfg);
 if(M5.getBoard()!=m5::board_t::board_ArduinoNessoN1) {Serial.println("ERR board");return;}
 // C6 display DMA can remain stuck waiting for an empty output FIFO across
 // resets. Bypass that observed stall with FIFO transfers.
 auto *displayBus=static_cast<lgfx::Bus_SPI*>(M5.Display.panel()->bus());
 auto displayConfig=displayBus->config();displayConfig.dma_channel=0;displayBus->config(displayConfig);
 M5.Display.setRotation(0);M5.Display.setBrightness(115);
 // Nesso touch uses I2C while the LCD uses SPI. Do not let a touch read
 // end/restart a display transaction owned by the rendering task.
 if(auto *touch=M5.Display.touch()){auto config=touch->config();config.bus_shared=false;touch->config(config);}
 if(M5.Display.width()!=width||M5.Display.height()!=height) {Serial.println("ERR geometry");return;}
 canvas.setColorDepth(16);
 if(!canvas.createSprite(width,height)) {Serial.println("ERR framebuffer");return;}
 if(!configureImu()) {M5.Display.fillScreen(TFT_RED);Serial.println("ERR BMI270 configuration");return;}
 uint8_t mac[6];esp_read_mac(mac,ESP_MAC_EFUSE_FACTORY);
 const auto *part=esp_ota_get_running_partition();
 Serial.printf("BOOT %s factory=%02x:%02x:%02x:%02x:%02x:%02x app=0x%lx size=0x%lx imu=0x%02x accel_conf=0xa9 gyro_conf=0xe9 display=135x240 bar=13\n",version,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],(unsigned long)part->address,(unsigned long)part->size,imuAddress);
 M5.BtnA.setDebounceThresh(5);
 audioReady=startSound();
 if(!audioReady)Serial.println("NOTE audio unavailable; visual simulation remains active");
 ready=xTaskCreate(sensorTask,"gravity-sensor",8192,nullptr,3,nullptr)==pdPASS;
 if(!ready) Serial.println("ERR sensor task");
 reportAt=millis();
}
void loop() {
 if(!ready) {delay(100);return;}
 uint32_t now=micros();
 if(now-lastFrame>=framePeriod) {
  if(lastFrame) frameGapMax=std::max(frameGapMax,now-lastFrame);
  lastFrame=now;draw(snapshot());
 }
 // No command parsing backlog; '?' asks for the same bounded diagnostic report.
 bool requested=false;
 for(int i=0;i<16 && Serial.available();i++) if(Serial.read()=='?') requested=true;
 if(Serial && (millis()-reportAt>=2000 || requested)) report();
 delay(1);
}
