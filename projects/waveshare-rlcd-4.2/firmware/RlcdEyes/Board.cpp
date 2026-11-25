// SPDX-License-Identifier: MIT
#include "Board.h"
#include "Metadata.h"
#include "Battery.h"
#include <SPI.h>
#include <Wire.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
namespace rlcd_eyes {
uint32_t transfers=0,transferMaxUs=0;unsigned peripheralMask=0;
int batteryPinMv=-1;
uint32_t batterySamples=0,batteryErrors=0,batteryReadMaxUs=0,batteryUpdatedMs=0;
namespace {
SPIClass bus(FSPI);
adc_oneshot_unit_handle_t adc=nullptr;adc_cali_handle_t cal=nullptr;
class Panel {
 void _cmd(uint8_t value){digitalWrite(5,LOW);digitalWrite(40,LOW);bus.transfer(value);digitalWrite(40,HIGH);}
 void _cmd_data(uint8_t cmd,const uint8_t* data,size_t n){_cmd(cmd);digitalWrite(5,HIGH);digitalWrite(40,LOW);bus.writeBytes(data,n);digitalWrite(40,HIGH);}
 void reset(){digitalWrite(41,HIGH);delay(50);digitalWrite(41,LOW);delay(20);digitalWrite(41,HIGH);delay(50);}
public:
 void begin();
 void send(const uint8_t* data){
  const uint8_t columns[]={0x12,0x2A},rows[]={0,0xC7};
  _cmd_data(0x2A,columns,2);_cmd_data(0x2B,rows,2);_cmd(0x2C);
  digitalWrite(5,HIGH);digitalWrite(40,LOW);bus.writeBytes(data,frameBytes);digitalWrite(40,HIGH);
 }
};
#include "PanelInit.h"
Panel panel;
}
bool boardBegin(){
 pinMode(18,INPUT_PULLUP); // User KEY. BOOT/RESET are reserved for recovery.
 // Exact RLCD codec configuration assigns PA_CTRL to GPIO46. First release is silent.
 pinMode(46,OUTPUT);digitalWrite(46,LOW);
 pinMode(5,OUTPUT);pinMode(40,OUTPUT);pinMode(41,OUTPUT);digitalWrite(40,HIGH);
 bus.begin(11,-1,12,40);
 bus.beginTransaction(SPISettings(10000000,MSBFIRST,SPI_MODE0));panel.begin();bus.endTransaction();
 adc_oneshot_unit_init_cfg_t unit={};unit.unit_id=ADC_UNIT_1;
 if(adc_oneshot_new_unit(&unit,&adc)==ESP_OK){
  adc_oneshot_chan_cfg_t chan={};chan.atten=ADC_ATTEN_DB_12;chan.bitwidth=ADC_BITWIDTH_12;
  if(adc_oneshot_config_channel(adc,ADC_CHANNEL_3,&chan)==ESP_OK){
   adc_cali_curve_fitting_config_t cfg={};cfg.unit_id=ADC_UNIT_1;cfg.chan=ADC_CHANNEL_3;cfg.atten=ADC_ATTEN_DB_12;cfg.bitwidth=ADC_BITWIDTH_12;
   if(adc_cali_create_scheme_curve_fitting(&cfg,&cal)!=ESP_OK)cal=nullptr;
  }
 }
 Wire.begin(13,14);Wire.setTimeOut(15);
 const uint8_t addresses[]={0x18,0x40,0x51,0x70};
 for(unsigned i=0;i<4;i++){Wire.beginTransmission(addresses[i]);if(Wire.endTransmission()==0)peripheralMask|=1U<<i;}
 return true; // SPI has no panel readback; this does not prove visible pixels.
}
void present(const uint8_t* bytes){
 const uint32_t start=micros();bus.beginTransaction(SPISettings(10000000,MSBFIRST,SPI_MODE0));panel.send(bytes);bus.endTransaction();
 ++transfers;transferMaxUs=std::max(transferMaxUs,micros()-start);
}
bool batteryCalibrated(){return cal!=nullptr;}
int readBattery(){
 const uint32_t started=micros();++batterySamples;batteryPinMv=-1;
 int total=0;bool ok=adc&&cal;
 for(unsigned i=0;ok&&i<4;i++){
  int raw=0,mv=0;
  ok=adc_oneshot_read(adc,ADC_CHANNEL_3,&raw)==ESP_OK&&adc_cali_raw_to_voltage(cal,raw,&mv)==ESP_OK;
  if(ok)total+=mv;
 }
 if(ok)batteryPinMv=total/4;
 const int mv=batteryVoltage(total,4,ok);if(mv<0)++batteryErrors;
 batteryReadMaxUs=std::max(batteryReadMaxUs,micros()-started);batteryUpdatedMs=millis();
 return mv;
}
void drawHeader(Canvas& c,bool visible,int mv,unsigned personality){
 if(!visible)return;
 c.setTextWrap(false);c.setTextSize(2);c.setTextColor(1);
 c.setCursor(8,6);c.print(appName);c.print(" v");c.print(version);
 c.setCursor(8,26);c.print(created);
 char bat[12];formatBatteryHeader(mv,0,bat);
 c.setCursor(394-strlen(bat)*12,6);c.print(bat);
 const char* names[]={"","CURIOUS","DREAMY","CHEEKY"};const char* name=names[personality<=3?personality:0];
 c.setCursor(394-strlen(name)*12,26);c.print(name);c.drawFastHLine(8,46,386,1);
}
void drawMenu(Canvas& c,const Menu& menu,int mv){
 if(menu.page==Page::Play)return;
 // Rlcd.cpp clears the packed frame before drawing the menu and header.
 c.setTextSize(2);c.setTextColor(1);c.setTextWrap(false);
 const char* title="Menu";const char* lines[5]={};const char* items[7]={};unsigned start=180;
 switch(menu.page){
  case Page::Main:title="Menu";start=100;items[0]="Return to play";items[1]="Apps";items[2]="Personality";items[3]="Sound";items[4]="Display";items[5]="Controls";items[6]="About";break;
  case Page::Apps:title="Apps / EYES1";lines[0]="EYES1 is installed.";lines[1]="More apps require installation.";items[0]="Poke left";items[1]="Poke right";items[2]="Back";break;
  case Page::Personality:title="Personality";items[0]="Curious";items[1]="Dreamy";items[2]="Cheeky";items[3]="Back";break;
  case Page::Sound:title="Sound";lines[0]="Audio is off in this build.";lines[1]="Listening is not implemented.";items[0]="Back";break;
  case Page::Display:title="Display";items[0]="Performance";items[1]="Normal";items[2]="Back";break;
  case Page::Controls:title="Controls";start=264;lines[0]="KEY: short press = reaction";lines[1]="Hold KEY 0.8s = Menu";lines[2]="In menus: short = Next";lines[3]="Hold = Select; choose Back";lines[4]="BOOT / RESET: recovery only";items[0]="Back";break;
  case Page::About:title="About / EYES1";start=264;lines[0]="Waveshare RLCD 4.2";lines[1]="1-bit screen; no backlight";lines[2]="Battery: volts, not percent";lines[3]="Charging: unknown";items[0]="Back";break;
  case Page::Performance:title="Performance";lines[0]="Hides information for play.";lines[1]="To restore: hold KEY 0.8s.";items[0]="Enter performance";items[1]="Back";break;
  default:break;
 }
 c.setCursor(12,60);c.print(title);
 for(unsigned i=0;i<5;i++)if(lines[i]){c.setCursor(12,90+22*i);c.print(lines[i]);}
 if(menu.page==Page::Main){c.setTextSize(1);c.setCursor(12,84);c.print("KEY: short Next / hold Select");c.setTextSize(2);}
 if(menu.page==Page::About){char volts[8];formatBattery(mv,volts);c.setCursor(12,200);c.print("Battery node: ");c.print(volts);}
 for(unsigned i=0;i<menu.count();i++){
  const int y=start+25*i;const bool selected=i==menu.focus;
  if(selected)c.fillRect(8,y-3,384,23,1);
  c.setTextColor(selected?0:1);c.setCursor(14,y);c.print(items[i]);
 }
 c.setTextColor(1);
}
}
