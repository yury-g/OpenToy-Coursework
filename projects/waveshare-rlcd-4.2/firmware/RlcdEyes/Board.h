// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "Display.h"
#include "Menu.h"
namespace rlcd_eyes {
class Canvas:public Adafruit_GFX {
public:
 uint8_t bytes[frameBytes];
 Canvas():Adafruit_GFX(rlcd_eyes::width,rlcd_eyes::height){}
 void drawPixel(int16_t x,int16_t y,uint16_t color)override{pixel(bytes,x,y,color!=0);}
};
bool boardBegin();
void present(const uint8_t* bytes);
int readBattery();
bool batteryCalibrated();
extern int batteryPinMv;
extern uint32_t batterySamples,batteryErrors,batteryReadMaxUs,batteryUpdatedMs;
extern uint32_t transfers,transferMaxUs;
extern unsigned peripheralMask;
void drawMenu(Canvas& canvas,const Menu& menu,int batteryMv);
void drawHeader(Canvas& canvas,bool visible,int batteryMv,unsigned personality);
}
