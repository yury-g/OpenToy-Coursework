/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-FileCopyrightText: 2026 Battery Faces contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * ST7305 initialization/pixel mapping adapted from Waveshare's official driver:
 * https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/blob/
 * d8bf7813d3b53251577ff03c03596ad6ad3acae0/examples/arduino/examples/
 * 09_LVGL_V9_Test/display_bsp.cpp
 * Repository LICENSE identifies Copyright 2026 Waveshare, Apache License 2.0.
 * Changes: standalone synchronous Arduino SPI transport, no LVGL/U8g2/LUT;
 * uniform 200x150 to 400x300 landscape scaling and binary black-ink conversion.
 * https://www.apache.org/licenses/LICENSE-2.0
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace silver_rlcd {
constexpr size_t LOGICAL_WIDTH=200,LOGICAL_HEIGHT=150,LOGICAL_PIXELS=LOGICAL_WIDTH*LOGICAL_HEIGHT;
constexpr unsigned WIDTH=400,HEIGHT=300,NATIVE_WIDTH=300,NATIVE_HEIGHT=400;
constexpr size_t PACKED_BYTES=NATIVE_WIDTH*NATIVE_HEIGHT/8;
// ST7305 portrait bytes contain 4 adjacent columns x 2 adjacent rows.
// Vendor polarity: 1=white background, 0=black ink. Preserve the face shape:
// every nonzero RGB565 pixel becomes black, independent of its color.
inline bool encode(const uint16_t *logical,size_t pixels,uint8_t *packed,size_t capacity){
 if(!logical||!packed||pixels<LOGICAL_PIXELS||capacity<PACKED_BYTES)return false;
 memset(packed,0xff,PACKED_BYTES);
 // Uniform 2x landscape image, rotated into the proven portrait RAM mode.
 // Landscape coordinates X right/Y down map to native px=299-Y, py=X.
 for(unsigned y=0;y<HEIGHT;y++)for(unsigned x=0;x<WIDTH;x++){
  if(!logical[(y/2)*LOGICAL_WIDTH+x/2])continue;
  const unsigned px=NATIVE_WIDTH-1-y,py=x;
  const size_t index=(py/2)*(NATIVE_WIDTH/4)+px/4;
  const uint8_t mask=uint8_t(0x80u>>((px%4)*2+py%2));
  packed[index]&=uint8_t(~mask);
 }
 return true;
}
}

#ifndef SILVER_RLCD_HOST_TEST
#include <Arduino.h>
#include <SPI.h>
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error SilverRlcd requires the Waveshare ESP32-S3-RLCD-4.2 board profile
#endif

namespace silver_rlcd {
// Exact manufacturer LVGL sequence, including its 200 ms sleep-out delay.
struct InitCommand {uint8_t command,length;uint16_t waitMs;uint8_t data[10];};
constexpr InitCommand INIT[]={
 {0xd6,2,0,{0x17,0x02}},
 {0xd1,1,0,{0x01}},
 {0xc0,2,0,{0x11,0x04}},
 {0xc1,4,0,{0x69,0x69,0x69,0x69}},
 {0xc2,4,0,{0x19,0x19,0x19,0x19}},
 {0xc4,4,0,{0x4b,0x4b,0x4b,0x4b}},
 {0xc5,4,0,{0x19,0x19,0x19,0x19}},
 {0xd8,2,0,{0x80,0xe9}},
 {0xb2,1,0,{0x02}},
 {0xb3,10,0,{0xe5,0xf6,0x05,0x46,0x77,0x77,0x77,0x77,0x76,0x45}},
 {0xb4,8,0,{0x05,0x46,0x77,0x77,0x77,0x77,0x76,0x45}},
 {0x62,3,0,{0x32,0x03,0x1f}},
 {0xb7,1,0,{0x13}},
 {0xb0,1,0,{0x64}},
 {0x11,0,200,{0}},
 {0xc9,1,0,{0x00}},
 {0x36,1,0,{0x48}},
 {0x3a,1,0,{0x11}},
 {0xb9,1,0,{0x20}},
 {0xb8,1,0,{0x29}},
 {0x21,0,0,{0}},
 {0x2a,2,0,{0x12,0x2a}},
 {0x2b,2,0,{0x00,0xc7}},
 {0x35,1,0,{0x00}},
 {0xd0,1,0,{0xff}},
 {0x38,0,0,{0}},
 {0x29,0,0,{0}},
};
} // namespace silver_rlcd

// Dedicated SPI3_HOST (Arduino HSPI), matching Waveshare's example. This class
// configures only the five confirmed LCD pins. No software power-hold GPIO,
// backlight, audio, SD, RTC, sensor, charger or battery ADC setup occurs here.
class SilverRlcd {
 SPIClass spi_{HSPI};
 uint8_t packed_[silver_rlcd::PACKED_BYTES]={};
 bool ready_=false;uint32_t framesPresented_=0,lastPresentAt_=0,lastTransferMicros_=0;
 const char *stage_="not started";
 static constexpr uint32_t SPI_HZ=10000000; // official LVGL example rate
 void command(uint8_t cmd,const uint8_t *data=nullptr,size_t count=0){
  digitalWrite(DC,LOW);digitalWrite(CS,LOW);spi_.transfer(cmd);
  if(count){digitalWrite(DC,HIGH);spi_.writeBytes(data,count);}
  digitalWrite(CS,HIGH);
 }
 void flush(){
  static constexpr uint8_t columns[]={0x12,0x2a},pages[]={0x00,0xc7};
  const uint32_t started=micros();
  spi_.beginTransaction(SPISettings(SPI_HZ,MSBFIRST,SPI_MODE0));
  command(0x2a,columns,sizeof columns);command(0x2b,pages,sizeof pages);
  command(0x2c,packed_,sizeof packed_);
  spi_.endTransaction();
  lastTransferMicros_=uint32_t(micros()-started);lastPresentAt_=millis();++framesPresented_;
 }
 public:
 static constexpr int SCK=11,MOSI=12,CS=40,DC=5,RESET=41;
 static constexpr int WIDTH=400,HEIGHT=300,LOGICAL_WIDTH=200,LOGICAL_HEIGHT=150;
 static constexpr int NATIVE_WIDTH=300,NATIVE_HEIGHT=400;
 SilverRlcd()=default;SilverRlcd(const SilverRlcd&)=delete;SilverRlcd&operator=(const SilverRlcd&)=delete;
 bool begin(){
  if(ready_)return true;
  pinMode(DC,OUTPUT);pinMode(CS,OUTPUT);pinMode(RESET,OUTPUT);
  digitalWrite(CS,HIGH);digitalWrite(DC,HIGH);digitalWrite(RESET,HIGH);
  stage_="SPI begin";
  if(!spi_.begin(SCK,-1,MOSI,-1))return false;
  // Manufacturer reset: high 50 ms, low 20 ms, high 50 ms.
  digitalWrite(RESET,HIGH);delay(50);digitalWrite(RESET,LOW);delay(20);digitalWrite(RESET,HIGH);delay(50);
  stage_="display initialization";
  for(const auto &step:silver_rlcd::INIT){
   spi_.beginTransaction(SPISettings(SPI_HZ,MSBFIRST,SPI_MODE0));
   command(step.command,step.data,step.length);spi_.endTransaction();
   if(step.waitMs)delay(step.waitMs);
  }
  memset(packed_,0xff,sizeof packed_);ready_=true;stage_="ready";flush();return true;
 }
 // Caller owns the draw cadence: at most 5 presents/s for Battery Faces.
 // Source is row-major native RGB565 words, exactly the logical 200x150 landscape canvas.
 bool present(const uint16_t *logical,size_t pixels=silver_rlcd::LOGICAL_PIXELS){
  if(!ready_||!silver_rlcd::encode(logical,pixels,packed_,sizeof packed_))return false;
  flush();return true;
 }
 bool ready()const{return ready_;}
 const char *stage()const{return stage_;}
 uint32_t framesPresented()const{return framesPresented_;}
 uint32_t lastPresentAt()const{return lastPresentAt_;}
 uint32_t lastTransferMicros()const{return lastTransferMicros_;}
};
#endif // SILVER_RLCD_HOST_TEST
