// SPDX-License-Identifier: MIT
#include "Board.h"
#include "Menu.h"
#include "Metadata.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include "Companion.h"
#pragma GCC diagnostic pop
#include "Smooth.h"
#include "Battery.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>
namespace {
using namespace crowpanel;
LayoutState layoutState;Presentation& view=layoutState.wanted;MenuInput input;MenuModel menu,shownMenu;bool uiDirty=true;
feather_face::Companion companion{1};feather_face::CompanionPose pose;
unsigned persona=1,syntheticMask=0;Deadline synthetic,skip;
uint32_t frames=0,samples=0,errors=0,pokes=0,touchAt=0,drawMax=0,hash=0;
uint32_t modelUs=0,waitUs=0,pixelUs=0,submitUs=0;
int batteryCandidateMv=-1;const int batteryMv=-1;esp_err_t batteryResult=ESP_ERR_NOT_FOUND;uint8_t batteryRaw[12]={};
bool displayReady=false,touchReady=false,contactDown=false;int touchX=-1,touchY=-1;
uint32_t clockMs(){return uint32_t(esp_timer_get_time()/1000);}
lv_obj_t *face[2],*header,*identity,*battery,*summary,*menuButton,*menuLabel,*panel,*title,*help,*buttons[6],*labels[6];
lv_color_t* pixels[2];
lv_img_dsc_t image[2];
char tx[2048];size_t txSize=0,txAt=0;
void pump(){if(txAt<txSize){int n=uart_tx_chars(UART_NUM_0,tx+txAt,std::min<size_t>(128,txSize-txAt));if(n>0)txAt+=n;}}
void report(){
 if(txAt<txSize)return;
 auto now=clockMs();const auto* slot=esp_ota_get_running_partition();
 int n=snprintf(tx,sizeof(tx),"EYES app=%s version=%s created=%s source=%s ms=%lu slot=0x%lx ready=%d touch_ready=%d\nDISPLAY frames=%lu hash=%08lx max_us=%lu heap=%lu header=%d eco=%d menu=%u performance=%d battery_mv=%d candidate_mv=%d battery_result=%d charge=unknown raw=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\nTOUCH samples=%lu errors=%lu fresh=%d contacts=%d xy=%d,%d mask=%u pokes=%lu\nPOSE persona=%u mood=%d,%d lids=%u,%u synthetic=%d blinks=%lu,%lu\nTIMING model_us=%lu wait_us=%lu pixel_us=%lu submit_us=%lu\n",
  appName,version,created,source,(unsigned long)now,(unsigned long)(slot?slot->address:0),displayReady,touchReady,
  (unsigned long)frames,(unsigned long)hash,(unsigned long)drawMax,(unsigned long)esp_get_free_heap_size(),view.header,view.eco,unsigned(menu.page),menu.performance,batteryMv,batteryCandidateMv,batteryResult,batteryRaw[0],batteryRaw[1],batteryRaw[2],batteryRaw[3],batteryRaw[4],batteryRaw[5],batteryRaw[6],batteryRaw[7],batteryRaw[8],batteryRaw[9],batteryRaw[10],batteryRaw[11],
  (unsigned long)samples,(unsigned long)errors,uint32_t(now-touchAt)<100,contactDown,touchX,touchY,input.mask(now),(unsigned long)pokes,
  persona,pose.mood[0],pose.mood[1],pose.eye[0].left,pose.eye[1].right,synthetic.active(now),(unsigned long)companion.idle.blinks[0],(unsigned long)companion.idle.blinks[1],
  (unsigned long)modelUs,(unsigned long)waitUs,(unsigned long)pixelUs,(unsigned long)submitUs);
 txSize=n>0?std::min<size_t>(n,sizeof(tx)-1):0;txAt=0;
}
void styleBox(lv_obj_t* obj,uint32_t color){
 lv_obj_remove_style_all(obj);lv_obj_set_style_bg_color(obj,lv_color_hex(color),0);lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
 lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
}
void visible(lv_obj_t* obj,bool show){if(show)lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);}
void layout(){
 visible(header,!menu.performance);visible(menuButton,menu.page==Page::Play);
 visible(panel,menu.page!=Page::Play);
 const char* names[]={"Curious","Dreamy","Cheeky"};
 lv_label_set_text_fmt(summary,"%s   ECO %s",names[persona-1],view.eco?"ON":"OFF");
 lv_obj_set_style_bg_color(menuButton,lv_color_hex(input.pressed()==Command::Open?0x365c66:0x263740),0);
 lv_obj_set_style_border_width(menuButton,input.pressed()==Command::Open?3:1,0);
 for(unsigned eye=0;eye<2;eye++){
  visible(face[eye],menu.page==Page::Play);
  unsigned w=28*view.scale(),h=32*view.scale();
  if(image[eye].header.w!=w||image[eye].header.h!=h){
   image[eye].header.w=w;image[eye].header.h=h;image[eye].data_size=w*h*sizeof(lv_color_t);
   lv_img_cache_invalidate_src(&image[eye]);lv_img_set_src(face[eye],&image[eye]);
  }
 }
 Item choices[6];unsigned n=menu.items(choices);
 lv_label_set_text(title,menu.title());
 for(unsigned k=0;k<6;k++){
  bool show=menu.page!=Page::Play&&k<n;visible(buttons[k],show);if(!show)continue;
  auto box=choices[k].bounds;lv_obj_set_pos(buttons[k],box.x,box.y-88);lv_obj_set_size(buttons[k],box.w,box.h);
  bool pressed=input.pressed()==choices[k].command;
  lv_obj_set_style_bg_color(buttons[k],lv_color_hex(pressed?0x365c66:0x263740),0);
  lv_obj_set_style_border_width(buttons[k],pressed?3:1,0);
  lv_label_set_text(labels[k],choices[k].label);lv_obj_center(labels[k]);
 }
 const char* message="";
 switch(menu.page){
  case Page::Root:message="Choose a setting, or Return to play.";break;
  case Page::Apps:message="Eyes 1 is the current app.\nMore apps require installation.";break;
  case Page::Personality:message="Each personality has its own look and blink rhythm.";break;
  case Page::Display:message="ECO lowers backlight and target animation cadence.\nPerformance hides all bars; it does not guarantee higher speed.";break;
  case Page::PerformanceConfirm:message="All information and controls will hide.\nTo restore Menu, hold the top-center edge for 0.8 seconds.\nRestart also restores normal information.";break;
  case Page::Controls:message="Play: touch either eye, or both.\nMenu: tap the visible Menu button.\nPerformance: hold the top-center edge for 0.8 seconds to restore.\nBOOT and RESET are reserved for recovery.";break;
  case Page::About:break;
  default:break;
 }
 lv_obj_set_pos(help,32,164+int((n+1)/2)*88+24-88);
 if(menu.page==Page::About)lv_label_set_text_fmt(help,"%s v%s  |  %s\nCrowPanel 9-inch V1.0 / ESP32-P4\nBattery: unknown  |  Charging: unknown\nSound is not available in this build.",appName,version,created);
 else lv_label_set_text(help,message);
 shownMenu=menu;layoutState.commit();uiDirty=false;
}
lv_obj_t* textLabel(lv_obj_t* parent,const lv_font_t* font){
 auto* label=lv_label_create(parent);lv_obj_set_style_text_font(label,font,0);lv_obj_set_style_text_color(label,lv_color_white(),0);return label;
}
void makeUI(){
 lv_obj_t* screen=lv_scr_act();styleBox(screen,0x000000);
 for(unsigned eye=0;eye<2;eye++){
  image[eye].header.cf=LV_IMG_CF_TRUE_COLOR;image[eye].header.w=196;image[eye].header.h=224;
  image[eye].data_size=196*224*sizeof(lv_color_t);image[eye].data=reinterpret_cast<uint8_t*>(pixels[eye]);
  face[eye]=lv_img_create(screen);lv_img_set_src(face[eye],&image[eye]);
 }
 header=lv_obj_create(screen);styleBox(header,0x151d22);lv_obj_set_size(header,1024,88);lv_obj_set_pos(header,0,0);
 identity=textLabel(header,&lv_font_montserrat_24);lv_label_set_text_fmt(identity,"%s v%s  %s",appName,version,created);lv_obj_set_pos(identity,16,10);
 summary=textLabel(header,&lv_font_montserrat_20);lv_obj_set_pos(summary,16,48);
 battery=textLabel(header,&lv_font_montserrat_24);lv_label_set_text(battery,"BAT --");lv_obj_align(battery,LV_ALIGN_RIGHT_MID,-16,0);
 menuButton=lv_obj_create(header);styleBox(menuButton,0x263740);auto box=MenuModel::menuBox();lv_obj_set_pos(menuButton,box.x,box.y);lv_obj_set_size(menuButton,box.w,box.h);
 lv_obj_set_style_border_color(menuButton,lv_color_white(),0);menuLabel=textLabel(menuButton,&lv_font_montserrat_24);lv_label_set_text(menuLabel,"Menu");lv_obj_center(menuLabel);
 panel=lv_obj_create(screen);styleBox(panel,0x0e171c);lv_obj_set_pos(panel,0,88);lv_obj_set_size(panel,1024,512);
 title=textLabel(panel,&lv_font_montserrat_24);lv_obj_set_pos(title,32,16);
 for(unsigned k=0;k<6;k++){
  buttons[k]=lv_obj_create(panel);styleBox(buttons[k],0x263740);lv_obj_set_style_border_color(buttons[k],lv_color_white(),0);
  labels[k]=textLabel(buttons[k],&lv_font_montserrat_24);
 }
 help=textLabel(panel,&lv_font_montserrat_20);lv_obj_set_width(help,960);lv_label_set_long_mode(help,LV_LABEL_LONG_WRAP);
 layout();
}
void syncMenu(uint32_t now,bool waitForRelease){
 if(persona!=menu.persona){persona=menu.persona;companion=feather_face::Companion(persona);}
 if(view.eco!=menu.eco)board_brightness(menu.eco?35:65);
 view=menu.presentation();input.cancel(waitForRelease);synthetic.start(now,0);
 contactDown=false;layoutState.dirty=true;
}
void sample(uint32_t now){
 Command prior=input.pressed();input.expire(now);if(prior!=input.pressed())uiDirty=true;
 if(skip.active(now))return;
 uint16_t x[5]={},y[5]={};uint8_t count=0;
 esp_err_t result=touchReady?board_touch_read(x,y,&count):ESP_ERR_INVALID_STATE;
 now=clockMs();++samples;
 if(result==ESP_ERR_NOT_FINISHED){if(!input.mask(now))contactDown=false;return;}
 bool ok=result==ESP_OK;if(!ok)++errors;touchAt=now;
 Contact points[5];for(unsigned k=0;k<std::min<unsigned>(count,5);k++)points[k]={int(x[k]),int(y[k])};
 bool down=ok&&count>0&&count<=5;
 if(down){touchX=x[0];touchY=y[0];}else touchX=touchY=-1;
 unsigned oldMask=input.mask(now);prior=input.pressed();Command action=Command::None;
 if(layoutState.dirty)input.cancel(down||!ok);
 else action=input.sample(ok,points,count,now,shownMenu);
 if(prior!=input.pressed())uiDirty=true;
 if(menu.apply(action))syncMenu(now,down);
 if(input.mask(now)&&!oldMask)++pokes;
 contactDown=down;
}
void draw(uint32_t now){
 uint32_t start=esp_timer_get_time();unsigned mask=menu.page==Page::Play?input.mask(now):0;
 if(menu.page==Page::Play&&!layoutState.dirty&&synthetic.active(now))mask=syntheticMask;
 pose=companion.update(512,512,0,false,now,mask);
 uint32_t modeled=esp_timer_get_time();modelUs=modeled-start;
 if(lvgl_port_lock(10)){
  uint32_t locked=esp_timer_get_time();waitUs=locked-modeled;
  if(layoutState.dirty||uiDirty)layout();
  if(batteryMv<0)lv_label_set_text(battery,"BAT --");
  else lv_label_set_text_fmt(battery,"BAT %d.%02d V",batteryMv/1000,(batteryMv%1000)/10);
  hash=2166136261U;
  if(menu.page==Page::Play)for(unsigned eye=0;eye<2;eye++){
   auto shake=nesso_eyes::jitter(now,pose.mood[eye],eye);
   int x=view.left()+((eye?78:22)+shake.x)*view.scale();
   int y=view.top()+(eyeWhiteY(pose.eye[eye])+shake.y+feather_face::personality(persona).eyeOffset[eye])*view.scale();
   renderSmoothEye(pose,eye,view.scale(),feather_face::personality(persona).ink,reinterpret_cast<uint16_t*>(pixels[eye]));
   for(unsigned i=0;i<image[eye].data_size/2;i++){hash^=pixels[eye][i].full;hash*=16777619U;}
   lv_obj_set_pos(face[eye],x,y);lv_obj_invalidate(face[eye]);
  }
  uint32_t expanded=esp_timer_get_time();pixelUs=expanded-locked;
  lvgl_port_unlock();++frames;
  submitUs=uint32_t(esp_timer_get_time())-expanded;
 }
 drawMax=std::max(drawMax,uint32_t(esp_timer_get_time())-start);
}
}
extern "C" void app_main(){
 uart_config_t uart={};uart.baud_rate=115200;uart.data_bits=UART_DATA_8_BITS;uart.parity=UART_PARITY_DISABLE;uart.stop_bits=UART_STOP_BITS_1;uart.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;uart.source_clk=UART_SCLK_DEFAULT;
 ESP_ERROR_CHECK(uart_param_config(UART_NUM_0,&uart));ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0,2048,0,0,nullptr,0));
 displayReady=board_display_begin()==ESP_OK;
 touchReady=board_touch_begin()==ESP_OK;
 for(unsigned eye=0;eye<2;eye++){
  pixels[eye]=static_cast<lv_color_t*>(heap_caps_calloc(224*256,sizeof(lv_color_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!pixels[eye])displayReady=false;
 }
 if(displayReady&&lvgl_port_lock(1000)){makeUI();lvgl_port_unlock();board_brightness(35);}else displayReady=false;
 esp_log_level_set("*",ESP_LOG_NONE);report();
 uint32_t sampleAt=0,frameAt=0,batteryAt=clockMs()-5000;
 for(;;){
  pump();uint32_t now=clockMs();bool requested=false;uint8_t commands[16];int n=uart_read_bytes(UART_NUM_0,commands,sizeof(commands),0);
  for(int i=0;i<n;i++){
   int c=commands[i];if(c=='?')requested=true;
   if(c=='H'){menu.performance=!menu.performance;menu.page=menu.performance?Page::Play:Page::Root;syncMenu(now,true);requested=true;}
   if(c=='M'||c=='Q'){menu.performance=false;menu.page=c=='M'?Page::Root:Page::Play;syncMenu(now,true);requested=true;}
   if(c=='E'){menu.eco=!menu.eco;syncMenu(now,true);requested=true;}
   if(c>='1'&&c<='3'){menu.persona=c-'0';syncMenu(now,true);requested=true;}
   if(c=='L'||c=='R'||c=='B'){syntheticMask=c=='L'?1:c=='R'?2:3;synthetic.start(now,300);requested=true;}
   if(c=='!'){skip.start(now,1500);input.cancel();contactDown=false;uiDirty=true;requested=true;}
  }
  if(uint32_t(now-sampleAt)>=10){sampleAt=now;sample(now);now=clockMs();}
  if(uint32_t(now-batteryAt)>=5000){batteryAt=now;batteryResult=board_battery_read(batteryRaw);batteryCandidateMv=batteryMillivolts(batteryRaw,batteryResult==ESP_OK);now=clockMs();}
  if(displayReady&&uint32_t(now-frameAt)>=(view.eco?66U:33U)){frameAt=now;draw(now);}
  if(requested)report();
  vTaskDelay(1); // At least one real tick, including a 100 Hz FreeRTOS build.
 }
}
