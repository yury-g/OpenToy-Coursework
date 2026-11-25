// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "BatteryState.h"
#include "BatteryLed.h"
#ifndef FACE_SILVER
#define FACE_SILVER 0
#endif
#ifndef FACE_STICK
#define FACE_STICK 0
#endif
#ifndef FACE_CYD
#define FACE_CYD 0
#endif
#if FACE_SILVER
#define FACE_YELLOW 0
#define FACE_NESSO 0
#define FACE_TOUCH 1
#include "SilverRlcd.h"
#include "VoltageEstimate.h"
SilverRlcd silverDisplay;bool silverReady=false;uint32_t silverAdcMv=0;
#elif FACE_STICK
#define FACE_YELLOW 0
#define FACE_NESSO 0
#define FACE_TOUCH 1
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ST7789.h>
#include "VoltageEstimate.h"
Adafruit_ST7789 display(&SPI,41,45,21);
uint8_t pinkPowerBefore=0,pinkPowerAfter=0;bool pinkPmic=false;
bool pinkRead(uint8_t reg,uint8_t *data,size_t size){Wire.beginTransmission(0x6e);Wire.write(reg);if(Wire.endTransmission(false))return false;if(Wire.requestFrom((uint8_t)0x6e,(uint8_t)size)!=size)return false;for(size_t i=0;i<size;i++)data[i]=Wire.read();return true;}
bool pinkMask(uint8_t reg,uint8_t clear,uint8_t set){uint8_t v;if(!pinkRead(reg,&v,1))return false;Wire.beginTransmission(0x6e);Wire.write(reg);Wire.write((v&~clear)|set);return Wire.endTransmission()==0;}
#elif FACE_CYD
#define FACE_YELLOW 0
#define FACE_NESSO 0
#define FACE_TOUCH 1
#include <SPI.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
Adafruit_ST7789 display(&SPI,15,2,-1);
SPIClass cydTouchSPI(HSPI);XPT2046_Touchscreen cydTouch(33,36);
bool cydTouchReady=false;uint16_t cydPanelId=0;
#elif defined(FACE_WAVESHARE_169)
#define FACE_YELLOW 1
#define FACE_NESSO 0
#define FACE_TOUCH 1
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ST7789.h>
#include "VoltageEstimate.h"
Adafruit_ST7789 display(&SPI,5,4,8);
uint16_t *yellowBuffer=nullptr;
uint32_t yellowAdcMv=0;bool yellowTouch=false,yellowPaint=true;
#elif defined(ARDUINO_ARDUINO_NESSO_N1)
#define FACE_YELLOW 0
#define FACE_TOUCH 1
#define FACE_NESSO 1
#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_ST7789.hpp>
#include <lgfx/v1/touch/Touch_FT5x06.hpp>
// Confirmed Nesso hardware: bind its panel directly, avoiding GPIO-based
// autodetection that can fail after an interrupted I2C transaction/reset.
class NessoLight:public lgfx::ILight {
 public:bool init(uint8_t b)override{setBrightness(b);return true;}
 void setBrightness(uint8_t b)override{if(b)lgfx::i2c::bitOn(0,0x44,0x05,0x40,100000);else lgfx::i2c::bitOff(0,0x44,0x05,0x40,100000);}
};
class NessoDisplay:public lgfx::LGFX_Device {
 lgfx::Bus_SPI bus;lgfx::Panel_ST7789 panelDriver;lgfx::Touch_FT5x06 touchDriver;NessoLight light;
 public:NessoDisplay(){
  auto busCfg=bus.config();busCfg.pin_mosi=21;busCfg.pin_miso=22;busCfg.pin_sclk=20;busCfg.pin_dc=16;busCfg.freq_write=40000000;busCfg.freq_read=16000000;busCfg.spi_3wire=true;bus.config(busCfg);panelDriver.setBus(&bus);
  auto p=panelDriver.config();p.pin_cs=17;p.pin_rst=-1;p.panel_width=135;p.panel_height=240;p.offset_x=52;p.offset_y=40;p.invert=true;p.readable=true;p.bus_shared=true;panelDriver.config(p);
  auto t=touchDriver.config();t.x_min=0;t.x_max=134;t.y_min=0;t.y_max=239;t.bus_shared=true;t.i2c_port=0;t.i2c_addr=0x38;t.pin_rst=-1;t.pin_int=3;t.pin_sda=10;t.pin_scl=8;t.freq=400000;touchDriver.config(t);panelDriver.setTouch(&touchDriver);panelDriver.setLight(&light);setPanel(&panelDriver);
 }
};
NessoDisplay display;
bool panelReady=false;
uint8_t rawCharge=0;
bool gaugeRead16(uint8_t reg,uint16_t &value){
 uint8_t b[2];if(!lgfx::i2c::readRegister(0,0x55,reg,b,2,100000))return false;
 value=(uint16_t)b[0]|((uint16_t)b[1]<<8);return true;
}
#else
#define FACE_NESSO 0
#define FACE_YELLOW 0
#define FACE_TOUCH 0
#include <Adafruit_ST7789.h>
#include <Adafruit_MAX1704X.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "esp32-hal-tinyusb.h"
Adafruit_ST7789 display(TFT_CS,TFT_DC,TFT_RST);
Adafruit_MAX17048 fuel;
Adafruit_NeoPixel statusPixel(1,PIN_NEOPIXEL,NEO_GRB+NEO_KHZ800);
#endif
bool isRetiredGreenDevice(){return ESP.getEfuseMac()==0x1480d562f8b8ULL;}
unsigned localFace(){return FACE_SILVER?8:FACE_STICK?6:FACE_CYD?5:FACE_YELLOW?(ESP.getEfuseMac()==0xf8fc939006e8ULL?4:3):FACE_NESSO?2:ESP.getEfuseMac()==0xb0cbb16e27acULL?1:0;}
struct HardwareBattery {
 float volts=NAN,percent=NAN,rate=NAN,current=0;bool currentValid=false;uint32_t chargerState=0;
};
void initFaceHardware(){
#if FACE_SILVER
 silverReady=silverDisplay.begin();pinMode(0,INPUT_PULLUP);pinMode(18,INPUT_PULLUP);
 pinMode(4,INPUT);analogReadResolution(12);analogSetPinAttenuation(4,ADC_11db);
#elif FACE_STICK
 Wire.begin(47,48);Wire.setTimeOut(25);uint8_t id[2]={};pinkPmic=pinkRead(0,id,2)&&id[0]==0x50&&id[1]==0x20;
 pinkRead(6,&pinkPowerBefore,1);
 // Only LCD rail GPIO2; preserve charge enable, regulators, boost and charger settings.
 if(pinkPmic){pinkMask(0x16,4,0);pinkMask(0x10,0,4);pinkMask(0x13,4,0);pinkMask(0x11,0,4);}
 delay(100);pinMode(38,OUTPUT);digitalWrite(38,LOW);SPI.begin(40,-1,39,41);display.init(135,240);display.setRotation(2);display.invertDisplay(true);display.fillScreen(0);digitalWrite(38,HIGH);
 pinMode(11,INPUT_PULLUP);pinMode(12,INPUT_PULLUP);pinkRead(6,&pinkPowerAfter,1);
#elif FACE_CYD
 pinMode(0,INPUT_PULLUP);pinMode(21,OUTPUT);digitalWrite(21,LOW);
 for(int pin:{4,16,17}){pinMode(pin,OUTPUT);digitalWrite(pin,HIGH);} // active-low RGB off
 pinMode(26,OUTPUT);digitalWrite(26,LOW); // speaker unused
 // Owner's micro-USB + USB-C model matches the CYD2USB ST7789 variant.
 SPI.begin(14,12,13,15);display.init(240,320);display.setSPISpeed(40000000);display.setRotation(0);
 display.invertDisplay(false);uint8_t madctl=0x88;display.sendCommand(ST77XX_MADCTL,&madctl,1); // BGR; clear MX after owner confirmed horizontal mirroring
 display.fillScreen(0);cydPanelId=display.readcommand8(0x04,2);
 digitalWrite(21,HIGH);cydTouchSPI.begin(25,39,32,33);cydTouchReady=cydTouch.begin(cydTouchSPI);cydTouch.setRotation(0);
#elif FACE_YELLOW
 // Confirmed printed-model revision: SYS_EN41, SYS_OUT40, buzzer42.
 pinMode(41,OUTPUT);digitalWrite(41,HIGH);pinMode(40,INPUT_PULLUP);
 pinMode(42,OUTPUT);digitalWrite(42,LOW);pinMode(0,INPUT_PULLUP);
 pinMode(15,OUTPUT);digitalWrite(15,LOW);SPI.begin(6,-1,7,5);
 display.init(240,280);display.setRotation(0);display.fillScreen(0);digitalWrite(15,HIGH);
 yellowBuffer=(uint16_t*)ps_malloc(240*280*2);
 Wire.begin(11,10);Wire.setTimeOut(20);pinMode(13,OUTPUT);digitalWrite(13,LOW);delay(10);digitalWrite(13,HIGH);delay(100);
 Wire.beginTransmission(0x15);yellowTouch=Wire.endTransmission()==0;
 Wire.beginTransmission(0x15);Wire.write(0xfe);Wire.write(1);Wire.endTransmission(); // disable auto sleep
 pinMode(1,INPUT);analogReadResolution(12);analogSetPinAttenuation(1,ADC_11db);
#elif FACE_NESSO
 // Recover a slave left mid-byte by a USB reset. Release open-drain lines;
 // nine clocks plus STOP never program charger/gauge registers.
 pinMode(10,INPUT_PULLUP);pinMode(8,INPUT_PULLUP);delay(2);
 if(!digitalRead(10) && digitalRead(8)){
  pinMode(8,OUTPUT_OPEN_DRAIN);digitalWrite(8,HIGH);
  for(unsigned n=0;n<9;n++){digitalWrite(8,LOW);delayMicroseconds(5);digitalWrite(8,HIGH);delayMicroseconds(5);}
  pinMode(10,OUTPUT_OPEN_DRAIN);digitalWrite(10,LOW);delayMicroseconds(5);digitalWrite(8,HIGH);delayMicroseconds(5);digitalWrite(10,HIGH);delayMicroseconds(5);
  pinMode(10,INPUT_PULLUP);pinMode(8,INPUT_PULLUP);
 }
 lgfx::i2c::init(0,10,8);
 // User buttons A/B are active-low inputs with pull-ups, including on POR.
 lgfx::i2c::bitOff(0,0x43,0x03,0x03,100000);
 lgfx::i2c::bitOn(0,0x43,0x0d,0x03,100000);lgfx::i2c::bitOn(0,0x43,0x0b,0x03,100000);
 // Touch only LCD reset/backlight direction and latch bits, preserving all
 // charger/gauge state and every other expander output.
 lgfx::i2c::bitOff(0,0x44,0x07,0x42,100000); // enable LCD outputs after a cold power-on
 lgfx::i2c::bitOn(0,0x44,0x03,0x42,100000);
 lgfx::i2c::bitOff(0,0x44,0x05,0x02,100000);delay(10);
 lgfx::i2c::bitOn(0,0x44,0x05,0x02,100000);delay(120);
 panelReady=display.init();display.setRotation(0);display.setBrightness(96);display.fillScreen(0);
#else
 pinMode(0,INPUT_PULLUP);pinMode(1,INPUT_PULLDOWN);pinMode(2,INPUT_PULLDOWN);
 pinMode(TFT_I2C_POWER,OUTPUT);pinMode(TFT_BACKLITE,OUTPUT);digitalWrite(TFT_BACKLITE,LOW);
 if(isRetiredGreenDevice()){
  pinMode(TFT_RST,OUTPUT);digitalWrite(TFT_RST,LOW);pinMode(TFT_CS,OUTPUT);digitalWrite(TFT_CS,HIGH);
  digitalWrite(TFT_I2C_POWER,HIGH);delay(20);
  pinMode(NEOPIXEL_POWER,OUTPUT);digitalWrite(NEOPIXEL_POWER,HIGH);
  statusPixel.begin();statusPixel.clear();statusPixel.show();pinMode(LED_BUILTIN,OUTPUT);digitalWrite(LED_BUILTIN,LOW);
 }else{
  digitalWrite(TFT_I2C_POWER,HIGH);delay(20);display.init(135,240);display.setRotation(0);display.fillScreen(0);digitalWrite(TFT_BACKLITE,HIGH);
 }
 Wire.begin();Wire.setTimeOut(30);fuel.begin(&Wire); // once only; library failed-reinit limitation
#endif
}
HardwareBattery readFaceBattery(){
 HardwareBattery out;
#if FACE_SILVER
 uint32_t sum=0;for(unsigned i=0;i<32;i++)sum+=analogReadMilliVolts(4);silverAdcMv=(sum+16)/32;out.volts=silverAdcMv*.003f;out.percent=voltagePercent(out.volts);
#elif FACE_STICK
 uint8_t mv[2];if(pinkPmic&&pinkRead(0x22,mv,2)){out.volts=(mv[0]|(uint16_t(mv[1])<<8))*.001f;out.percent=voltagePercent(out.volts);}
 // M5Unified M5PM1 isCharging() is a stub in the installed release. Unknown, never false proof.
#elif FACE_CYD
 // USB-powered viewer: no battery/gauge exists, never sample an unrelated ADC.
#elif FACE_YELLOW
 uint32_t sum=0;for(unsigned i=0;i<32;i++)sum+=analogReadMilliVolts(1);
 yellowAdcMv=(sum+16)/32;out.volts=yellowAdcMv*.003f;out.percent=voltagePercent(out.volts);
#elif FACE_NESSO
 uint16_t mv=0,soc=0,ma=0;
 bool v=gaugeRead16(0x08,mv),p=gaugeRead16(0x2c,soc),c=gaugeRead16(0x0c,ma);
 if(v && p && soc<=100){out.volts=mv*.001f;out.percent=soc;}
 if(c){out.current=(int16_t)ma;out.currentValid=true;}
 uint8_t status=0;
 if(lgfx::i2c::readRegister(0,0x49,0x08,&status,1,100000)){
  rawCharge=status;unsigned state=(status>>3)&3;
  out.chargerState=state==3?3:state==1||state==2?1:(c && out.current < -2?2:4);
 }
#else
 if(fuel.isDeviceReady()){out.volts=fuel.cellVoltage();out.percent=fuel.cellPercent();out.rate=fuel.chargeRate();}
#endif
 return out;
}
void showCanvas(uint16_t *buffer){
#if FACE_SILVER
 static uint32_t last=millis()-200;if(silverReady&&uint32_t(millis()-last)>=200){last=millis();silverDisplay.present(buffer);}
#elif FACE_CYD
 // Uniform 4/3 scaling in both axes. Small logical buffer leaves RAM for Wi-Fi.
 static uint16_t row[180];display.startWrite();display.setAddrWindow(30,0,180,320);
 for(unsigned y=0;y<320;y++){for(unsigned x=0;x<180;x++)row[x]=buffer[(y*3/4)*135+x*3/4];display.writePixels(row,180);}
 display.endWrite();
#elif FACE_YELLOW
 if(!yellowBuffer || !yellowPaint)return;
 // Inset protects the text from the watch display's rounded corners.
 memset(yellowBuffer,0,240*280*2);
 memcpy(yellowBuffer,buffer,240*280*2);
 display.drawRGBBitmap(0,0,yellowBuffer,240,280);
#elif FACE_NESSO
 display.pushImage(0,0,135,240,(lgfx::rgb565_t*)buffer);
#else
 display.drawRGBBitmap(0,0,buffer,135,240);
#endif
}
bool readTouchPoint(int &x,int &y){
#if FACE_CYD
 if(!cydTouchReady||!cydTouch.touched())return false;auto p=cydTouch.getPoint();
 x=constrain(map(p.x,200,3700,0,239),0,239);y=constrain(map(p.y,240,3800,0,319),0,319);return p.z>=350;
#elif FACE_YELLOW
 Wire.beginTransmission(0x15);Wire.write(0x02);
 if(Wire.endTransmission(false)!=0 || Wire.requestFrom((uint8_t)0x15,(uint8_t)5)!=5)return false;
 uint8_t b[5];for(auto &v:b)v=Wire.read();x=((b[1]&15)<<8)|b[2];y=((b[3]&15)<<8)|b[4];return (b[0]&15)>0;
#elif FACE_NESSO
 uint16_t tx=0,ty=0;bool down=display.getTouch(&tx,&ty)>0;x=tx;y=ty;return down;
#else
 x=y=0;return false;
#endif
}
void readFaceButtons(bool *pressed){
#if FACE_SILVER
 pressed[0]=!digitalRead(0);pressed[1]=!digitalRead(18);pressed[2]=false;
#elif FACE_STICK
 pressed[0]=!digitalRead(11);pressed[1]=!digitalRead(12);pressed[2]=false;
#elif FACE_CYD
 pressed[0]=!digitalRead(0);pressed[1]=pressed[2]=false;
#elif FACE_YELLOW
 pressed[0]=!digitalRead(0);pressed[1]=pressed[2]=false;
#elif FACE_NESSO
 uint8_t value=0xff;bool ok=bool(lgfx::i2c::readRegister(0,0x43,0x0f,&value,1,100000));
 pressed[0]=ok && !(value&1);pressed[1]=ok && !(value&2);pressed[2]=false;
#else
 pressed[0]=!digitalRead(0);pressed[1]=digitalRead(1);pressed[2]=digitalRead(2);
#endif
}
bool usbPowered(){
#if FACE_TOUCH
 return true; // Orange's UI never offers runtime tests; conservatively block any attempt.
#else
 return tud_mounted();
#endif
}
void showIndicator(BatteryLedColor color,uint32_t now){
#if !FACE_TOUCH
 statusPixel.setPixelColor(0,statusPixel.Color(color.r,color.g,color.b));statusPixel.show();digitalWrite(LED_BUILTIN,now%2000<80?HIGH:LOW);
#endif
}
void reportFaceHardware(){
#if FACE_SILVER
 Serial.printf("HW board=waveshare-rlcd42 ready=%d width=400 height=300 logical=200x150 monochrome=1 backlight=none adc_mv=%lu estimator=voltage buttons=0,18 presents=%lu transfer_us=%lu\n",silverReady,(unsigned long)silverAdcMv,(unsigned long)silverDisplay.framesPresented(),(unsigned long)silverDisplay.lastTransferMicros());
#elif FACE_STICK
 Serial.printf("HW board=sticks3 width=%d height=%d pmic=%d cfg_before=%02x cfg_after=%02x estimator=voltage buttons=11,12\n",display.width(),display.height(),pinkPmic,pinkPowerBefore,pinkPowerAfter);
#elif FACE_CYD
 Serial.printf("HW board=cyd width=%d height=%d touch=%d driver=st7789-dualusb panel_id=%04x uart=460800 estimator=none power=usb logical=135x240 scale=uniform4over3 heap=%lu\n",display.width(),display.height(),cydTouchReady,cydPanelId,(unsigned long)ESP.getFreeHeap());
#elif FACE_YELLOW
 Serial.printf("HW board=waveshare169 width=%d height=%d touch=%d adc_mv=%lu estimator=voltage framebuffer=%d\n",display.width(),display.height(),yellowTouch,(unsigned long)yellowAdcMv,yellowBuffer!=nullptr);
#elif FACE_NESSO
 Serial.printf("HW board=nesso panel=%d width=%d height=%d touch=%d charger_raw=%02x\n",panelReady,display.width(),display.height(),display.touch()!=nullptr,rawCharge);
#else
 Serial.printf("HW board=feather power=%d backlight=%d panel_reset=%d\n",digitalRead(TFT_I2C_POWER),digitalRead(TFT_BACKLITE),digitalRead(TFT_RST));
#endif
}
