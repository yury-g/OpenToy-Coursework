// SPDX-License-Identifier: MIT
// Local camera developer preview. Explicit simulation is available from Display.
#include "Board.h"
#include "Metadata.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include "Scene.h"
#pragma GCC diagnostic pop
#include "DeveloperView.h"
#include "Ui.h"
#if __has_include("../core/BatteryStatus.h")
#include "../core/BatteryStatus.h"
#else
#include "BatteryStatus.h"
#endif
#include "Camera.h"
#include <cstring>
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
namespace {
using namespace copy_face;
Tracker tracker;Ui ui;TouchMenu touchMenu;
feather_face::Companion companion{1};Pose pose;
bool displayReady=false,touchReady=false,havePixels=false;
CameraStatus cameraStatus;
uint32_t frames=0,maxDrawUs=0,captured=0;
int lastX=0,lastY=0;
lv_obj_t *eyes[2],*mouth,*header,*footer,*watermark,*developer,*camera,*shapeText,*eyeText,*selectionText,*previewTitle,*previewNote,*sourceLabel;
lv_obj_t *menu,*menuTitle,*menuInfo,*rows[7],*rowText[7];
lv_color_t *eyePixels[2],*mouthPixels,*cameraPixels;
uint16_t* sourcePixels;
uint16_t* exportPixels;size_t exportAt=0,exportSize=0;
lv_img_dsc_t eyeImage[2],mouthImage,cameraImage;
char tx[1200];size_t txSize=0,txAt=0;
uint32_t clockMs(){return uint32_t(esp_timer_get_time()/1000);}
PreviewImage image(){return {havePixels?sourcePixels:nullptr,previewWidth,previewHeight,previewWidth,previewWidth*previewHeight,captured};}
void report() {
 if(txAt<txSize||exportAt<exportSize)return;
 Detection selected;uint32_t at=0;bool has=tracker.selected(selected,at,clockMs());
 int n=snprintf(tx,sizeof(tx),"COPY_FACE app=%s version=%s created=%s source=%s\nSTATE camera=%d detector=%d sensor=%04x error=%d stage=%u camera_frames=%lu capture_ms=%lu infer_ms=%lu lips=unavailable-unless-simulated demo=%d tracking=%d mirror=%d header=%d developer=%d menu=%d battery=--\nDISPLAY ready=%d touch_ready=%d frames=%lu max_draw_us=%lu heap=%lu\nFACE valid=%d captured=%lu age=%lu box=%.4f,%.4f,%.4f,%.4f score=%.4f points=%d gaze=%.3f,%.3f\n",
 appName,version,created,source,cameraStatus.ready,cameraStatus.detector_ready,cameraStatus.sensor_id,cameraStatus.error,cameraStatus.stage,(unsigned long)cameraStatus.capture_count,(unsigned long)cameraStatus.captured_ms,(unsigned long)cameraStatus.inference_ms,ui.demo,pose.tracking,ui.mirror,!ui.performance,ui.showDeveloper(),int(ui.page),displayReady,touchReady,
 (unsigned long)frames,(unsigned long)maxDrawUs,(unsigned long)esp_get_free_heap_size(),has,(unsigned long)at,(unsigned long)(clockMs()-at),
 double(selected.x),double(selected.y),double(selected.w),double(selected.h),double(selected.confidence),selected.keypointsValid,double(pose.x),double(pose.y));
 txSize=n>0?std::min<size_t>(n,sizeof(tx)-1):0;txAt=0;
}
void box(lv_obj_t* obj,uint32_t color) {
 lv_obj_remove_style_all(obj);lv_obj_set_style_bg_color(obj,lv_color_hex(color),0);
 lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
}
void visible(lv_obj_t* obj,bool show){if(show)lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);}
lv_obj_t* label(lv_obj_t* parent,const char* text,int x,int y,bool small=false) {
 auto* l=lv_label_create(parent);lv_obj_set_style_text_font(l,small?&lv_font_montserrat_20:&lv_font_montserrat_24,0);
 lv_obj_set_style_text_color(l,lv_color_white(),0);lv_label_set_text(l,text);lv_obj_set_pos(l,x,y);return l;
}
void layout() {
 Layout l(!ui.performance,ui.showDeveloper());
 visible(header,!ui.performance);visible(footer,!ui.performance);visible(watermark,ui.performance&&ui.demo);
 visible(developer,ui.showDeveloper());visible(menu,ui.page!=Page::Closed);
 for(unsigned e=0;e<2;e++)lv_obj_set_pos(eyes[e],l.eyeX(e),l.eyeY());
 lv_obj_set_pos(mouth,l.mouthX(),l.mouthY());
 const char* title="Menu";const char* info="";
 switch(ui.page) {
 case Page::Apps:title="Apps";info="Copy-Face is the current app.\nMore apps require installation.";break;
 case Page::Personality:title="Personality";info="Curious blinking. Camera follows\nthe face. Live mouth stays neutral\nuntil a lip model is available.";break;
 case Page::Sound:title="Sound";info="Unavailable in this prototype.";break;
 case Page::Display:title="Display";info="Performance: hold the top-center\nedge for 0.8 seconds to restore.";break;
 case Page::Controls:title="Controls";info="Menu: tap and release here.\nLive green points: face landmarks.\nSimulation: estimated lip contact.\nDisplay: preview, mirror, simulation.\nPerformance restore: top-center\nedge hold for 0.8 seconds.\nBOOT/RESET are recovery only.";break;
 case Page::About:title="About";break;
 default:break;
 }
 lv_label_set_text(menuTitle,title);
 for(unsigned i=0;i<7;i++){visible(rows[i],i<ui.rows());if(i<ui.rows())lv_label_set_text(rowText[i],ui.rowLabel(i));}
 lv_obj_set_pos(menuInfo,16,ui.page==Page::Display?340:128);
 if(ui.page==Page::About)lv_label_set_text_fmt(menuInfo,"%s v%s\n%s\nCrowPanel DHE04209D V1.0\nBattery: -- (unverified)\nLocal camera; debug view in Display",appName,version,created);
 else lv_label_set_text(menuInfo,info);
 lv_label_set_text(previewTitle,ui.demo?"Camera preview (simulated)":"Camera preview (local)");
 lv_label_set_text(sourceLabel,ui.demo?"SIMULATION / scripted lip contours":"LOCAL CAMERA  |  Lip contour model not yet available");
 ui.dirty=false;
}
void makeUI() {
 auto* screen=lv_scr_act();box(screen,0);
 for(unsigned e=0;e<2;e++) {
  eyeImage[e].header.cf=LV_IMG_CF_TRUE_COLOR;eyeImage[e].header.w=224;eyeImage[e].header.h=256;
  eyeImage[e].data_size=224*256*2;eyeImage[e].data=reinterpret_cast<uint8_t*>(eyePixels[e]);
  eyes[e]=lv_img_create(screen);lv_img_set_src(eyes[e],&eyeImage[e]);
 }
 mouthImage.header.cf=LV_IMG_CF_TRUE_COLOR;mouthImage.header.w=mouthWidth;mouthImage.header.h=mouthHeight;
 mouthImage.data_size=mouthWidth*mouthHeight*2;mouthImage.data=reinterpret_cast<uint8_t*>(mouthPixels);
 mouth=lv_img_create(screen);lv_img_set_src(mouth,&mouthImage);
 developer=lv_obj_create(screen);box(developer,0);lv_obj_set_pos(developer,16,60);lv_obj_set_size(developer,340,464);
 previewTitle=label(developer,"Camera preview",0,0,true);
 cameraImage.header.cf=LV_IMG_CF_TRUE_COLOR;cameraImage.header.w=previewWidth;cameraImage.header.h=previewHeight;
 cameraImage.data_size=previewWidth*previewHeight*2;cameraImage.data=reinterpret_cast<uint8_t*>(cameraPixels);
 camera=lv_img_create(developer);lv_img_set_src(camera,&cameraImage);lv_obj_set_pos(camera,0,36);
 shapeText=label(developer,"Shape guess: unavailable",0,232,true);
 eyeText=label(developer,"Eyes: unavailable",0,266,true);
 selectionText=label(developer,"Acquiring primary face",0,308,true);
 previewNote=label(developer,"Green edges: eye / lip outlines\nGreen dots: estimated contact\n\nLandmarks are scripted.\nNot a live camera feed.",0,344,true);lv_obj_set_style_text_color(previewNote,lv_color_hex(0xa7c2ae),0);
 header=lv_obj_create(screen);box(header,0x151d22);lv_obj_set_size(header,1024,48);
 auto* identity=label(header,"",16,10);lv_label_set_text_fmt(identity,"%s v%s  %s",appName,version,created);
 auto* menuButton=lv_obj_create(header);box(menuButton,0x254d40);lv_obj_set_pos(menuButton,720,0);lv_obj_set_size(menuButton,144,48);label(menuButton,"Menu",34,10);
 char batText[16]={};formatBatteryBadge(-1,0,false,batText);
 auto* battery=label(header,batText,0,10);lv_obj_align(battery,LV_ALIGN_RIGHT_MID,-16,0);
 footer=lv_obj_create(screen);box(footer,0x151d22);lv_obj_set_pos(footer,0,544);lv_obj_set_size(footer,1024,56);
 sourceLabel=label(footer,"SIMULATION / NO CAMERA   |   Mouth follows selected lip contours",16,14,true);lv_obj_set_style_text_color(sourceLabel,lv_color_hex(0xffd577),0);
 watermark=label(screen,"SIMULATION",16,572,true);lv_obj_set_style_text_color(watermark,lv_color_hex(0xffd577),0);
 menu=lv_obj_create(screen);box(menu,0x182b25);lv_obj_set_pos(menu,296,60);lv_obj_set_size(menu,432,472);
 menuTitle=label(menu,"Menu",16,10);menuInfo=label(menu,"",16,128,true);lv_obj_set_width(menuInfo,400);
 for(unsigned i=0;i<7;i++) {
  rows[i]=lv_obj_create(menu);box(rows[i],0x27483b);lv_obj_set_pos(rows[i],16,52+i*52);lv_obj_set_size(rows[i],400,48);
  rowText[i]=label(rows[i],"",12,10);
 }
 layout();
}
void command(char c) {
 if(c=='H'){ui.apply(ui.performance?Action::Menu:Action::Performance);touchMenu.cancel();}
 if(c=='M')ui.apply(Action::Mirror);
 if(c=='D')ui.apply(Action::Demo);
}
void sampleTouch(uint32_t now) {
 touchMenu.expire(now);uint16_t x[5]={},y[5]={};uint8_t count=0;
 auto result=board_touch_read(x,y,&count);
 if(result==ESP_ERR_NOT_FINISHED)return;
 if(result!=ESP_OK||count>5){touchMenu.cancel();return;}
 if(count){lastX=x[0];lastY=y[0];}
 touchMenu.sample(ui,lastX,lastY,count,now);
}
void draw(uint32_t now) {
 uint32_t start=esp_timer_get_time();pose=tracker.step(now,ui.mirror);
 auto eyesPose=companion.update(512,512,0,false,now,0);applyGaze(pose,eyesPose);
 if(!lvgl_port_lock(10))return;
 if(ui.dirty)layout();
 if(ui.page!=Page::Closed)for(unsigned i=0;i<ui.rows();i++)
  lv_obj_set_style_bg_color(rows[i],lv_color_hex(touchMenu.pressed()==ui.rowAction(i)?0x497f63:0x27483b),0);
 for(unsigned e=0;e<2;e++) {
  crowpanel::renderSmoothEye(eyesPose,e,eyeScale,0xffff,reinterpret_cast<uint16_t*>(eyePixels[e]));lv_obj_invalidate(eyes[e]);
 }
 renderMouth(pose,reinterpret_cast<uint16_t*>(mouthPixels));lv_obj_invalidate(mouth);
 if(ui.showDeveloper()) {
  auto im=image();renderDeveloper(im,tracker,now,ui.mirror,reinterpret_cast<uint16_t*>(cameraPixels));lv_obj_invalidate(camera);
  auto selected=overlaySelection(im,tracker,now);
  auto shape=selected.tracked?normalizeLips(selected.face.lips):LipShape{};
  lv_label_set_text_fmt(shapeText,"Shape guess: %s",expressionName(expression(shape)));
  lv_label_set_text_fmt(eyeText,"Eyes L / R: %s / %s",selected.tracked?eyeActivity(selected.face.eyes[0]):"--",selected.tracked?eyeActivity(selected.face.eyes[1]):"--");
  if(selected.tracked)lv_label_set_text_fmt(selectionText,"Face score: %.2f / age: %lu ms",double(selected.face.confidence),(unsigned long)(now-im.captured));
  else lv_label_set_text(selectionText,"No current tracked face");
  if(ui.demo)lv_label_set_text(previewNote,"Green edges: eye / lip outlines\nGreen dots: estimated contact\n\nLandmarks are scripted.\nNot a live camera feed.");
  else if(cameraStatus.error)lv_label_set_text_fmt(previewNote,"Camera unavailable\nStartup stage: %u / error: %d\nNo camera image received.\nLip / eyelid model: unavailable",cameraStatus.stage,cameraStatus.error);
  else lv_label_set_text_fmt(previewNote,"Green box / points: face detector\nLip contour model: unavailable\nEye open/closed: unavailable\n\nFrames: %lu / infer: %lu ms",(unsigned long)cameraStatus.capture_count,(unsigned long)cameraStatus.inference_ms);
 }
 lvgl_port_unlock();++frames;maxDrawUs=std::max(maxDrawUs,uint32_t(esp_timer_get_time())-start);
}
}
extern "C" void app_main() {
 uart_config_t uart={};uart.baud_rate=115200;uart.data_bits=UART_DATA_8_BITS;uart.parity=UART_PARITY_DISABLE;
 uart.stop_bits=UART_STOP_BITS_1;uart.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;uart.source_clk=UART_SCLK_DEFAULT;
 ESP_ERROR_CHECK(uart_param_config(UART_NUM_0,&uart));ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0,2048,0,0,nullptr,0));
 displayReady=board_display_begin()==ESP_OK;touchReady=board_touch_begin()==ESP_OK;
 auto allocate=[](size_t count){return static_cast<lv_color_t*>(heap_caps_calloc(count,2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));};
 for(auto& pixels:eyePixels){pixels=allocate(224*256);if(!pixels)displayReady=false;}
 mouthPixels=allocate(mouthWidth*mouthHeight);cameraPixels=allocate(previewWidth*previewHeight);
 sourcePixels=reinterpret_cast<uint16_t*>(allocate(previewWidth*previewHeight));
 exportPixels=reinterpret_cast<uint16_t*>(allocate(previewWidth*previewHeight));
 if(!mouthPixels||!cameraPixels||!sourcePixels||!exportPixels)displayReady=false;
 if(displayReady&&lvgl_port_lock(1000)){makeUI();lvgl_port_unlock();board_brightness(35);}else displayReady=false;
 camera_begin();
 esp_log_level_set("*",ESP_LOG_NONE);report();uint32_t observed=0,rendered=0,touched=0;bool previousDemo=ui.demo;
 for(;;) {
  uint32_t now=clockMs();
  if(txAt<txSize){int n=uart_tx_chars(UART_NUM_0,tx+txAt,std::min<size_t>(128,txSize-txAt));if(n>0)txAt+=n;}
  else if(exportAt<exportSize){int n=uart_tx_chars(UART_NUM_0,reinterpret_cast<char*>(exportPixels)+exportAt,std::min<size_t>(128,exportSize-exportAt));if(n>0)exportAt+=n;}
  uint8_t input[16];int n=uart_read_bytes(UART_NUM_0,input,sizeof(input),0);
  for(int i=0;i<n;i++){
   command(char(input[i]));if(input[i]=='?')report();
   if(input[i]=='C'&&havePixels&&exportPixels&&exportAt>=exportSize&&txAt>=txSize) {
    memcpy(exportPixels,sourcePixels,previewWidth*previewHeight*2);exportAt=0;exportSize=previewWidth*previewHeight*2;
    txSize=snprintf(tx,sizeof(tx),"CF_FRAME %d %d %u %lu %d\n",previewWidth,previewHeight,unsigned(exportSize),(unsigned long)captured,ui.demo);txAt=0;
   }
  }
  if(touchReady&&uint32_t(now-touched)>=20){touched=now;sampleTouch(now);}
  if(previousDemo!=ui.demo){previousDemo=ui.demo;tracker=Tracker();havePixels=false;captured=0;}
  // Pixels and detection geometry come from the same capture, copied from a bounded mailbox.
  if(uint32_t(now-observed)>=100) {
   observed=now;copy_face::Frame f;
   if(ui.demo){f=syntheticFrame(now);tracker.observe(f,now);captured=now;if(sourcePixels){syntheticCamera(f,sourcePixels);havePixels=true;}}
   else if(sourcePixels&&camera_copy_latest(sourcePixels,previewWidth*previewHeight,&f,&cameraStatus)) {
    tracker.observe(f,clockMs());captured=f.captured;havePixels=true;
   }
  }
  now=clockMs(); // Snapshot copy and touch reads may advance the clock.
  if(displayReady&&uint32_t(now-rendered)>=33){rendered=now;draw(now);}
  vTaskDelay(1);
 }
}
