/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Battery Faces contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sensor register table copied from M5Stack's esp_cam_sensor SC202CS port:
 * https://github.com/m5stack/M5Tab5-UserDemo/blob/
 * b4e356bc491ca070d54004718dad789c07d5fc93/platforms/tab5/components/
 * esp_cam_sensor/sensors/sc202cs/private_include/sc202cs_settings.h
 * The original table is explicitly described there as the SC2356 720p mode.
 * Board pin mappings also checked against M5Stack's MIT-licensed Tab5 BSP.
 * Changes: isolated Arduino integration, fixed 720p mode, native CSI/ISP,
 * bounded fresh capture, RGB332 letterbox; no BSP/charger/DSI initialization.
 * License: https://www.apache.org/licenses/LICENSE-2.0
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace tabcamera_detail {
constexpr size_t SENSOR_WIDTH=1280,SENSOR_HEIGHT=720,OUTPUT_BYTES=4800;
// Native P4 ISP RGB565 words (not the XIAO camera's high-byte-first byte stream).
inline bool letterbox(const uint16_t *source,size_t pixels,uint8_t *out,size_t capacity){
 if(!source||!out||pixels<SENSOR_WIDTH*SENSOR_HEIGHT||capacity<OUTPUT_BYTES)return false;
 memset(out,0,OUTPUT_BYTES);
 for(size_t y=0;y<45;y++)for(size_t x=0;x<80;x++){
  const uint16_t v=source[(y*16+8)*SENSOR_WIDTH+x*16+8];
  out[(y+7)*80+x]=uint8_t(((v>>8)&0xe0)|((v>>6)&0x1c)|((v>>3)&3));
 }
 return true;
}
} // namespace tabcamera_detail

#ifndef TAB_CAMERA_HOST_TEST
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
#error TabCamera requires the ESP32-P4 Tab5 profile
#endif
#include <Arduino.h>
#include <M5GFX.h>
#include <esp_cam_ctlr.h>
#include <esp_cam_ctlr_csi.h>
#include <driver/isp_core.h>
#include <esp_clock_output.h>
#include <esp_clk_tree.h>
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <esp_cache.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace tabcamera_detail {
struct Register {uint16_t address;uint8_t value;};
static const Register mode720p[] = {
    {0x0103, 0x01},          {0x0100, 0x00},
    {0x36e9, 0x80},          {0x36ea, 0x06},
    {0x36eb, 0x0a},          {0x36ec, 0x01},
    {0x36ed, 0x18},          {0x36e9, 0x24},
    {0x301f, 0x18},          {0x3031, 0x08},
    {0x3037, 0x00},          {0x3200, 0x00},
    {0x3201, 0xa0},          {0x3202, 0x00},
    {0x3203, 0xf0},          {0x3204, 0x05},
    {0x3205, 0xa7},          {0x3206, 0x03},
    {0x3207, 0xc7},          {0x3208, 0x05},
    {0x3209, 0x00},          {0x320a, 0x02},
    {0x320b, 0xd0},          {0x3210, 0x00},
    {0x3211, 0x04},          {0x3212, 0x00},
    {0x3213, 0x04},          {0x3301, 0xff},
    {0x3304, 0x68},          {0x3306, 0x40},
    {0x3308, 0x08},          {0x3309, 0xa8},
    {0x330b, 0xd0},          {0x330c, 0x18},
    {0x330d, 0xff},          {0x330e, 0x20},
    {0x331e, 0x59},          {0x331f, 0x99},
    {0x3333, 0x10},          {0x335e, 0x06},
    {0x335f, 0x08},          {0x3364, 0x1f},
    {0x337c, 0x02},          {0x337d, 0x0a},
    {0x338f, 0xa0},          {0x3390, 0x01},
    {0x3391, 0x03},          {0x3392, 0x1f},
    {0x3393, 0xff},          {0x3394, 0xff},
    {0x3395, 0xff},          {0x33a2, 0x04},
    {0x33ad, 0x0c},          {0x33b1, 0x20},
    {0x33b3, 0x38},          {0x33f9, 0x40},
    {0x33fb, 0x48},          {0x33fc, 0x0f},
    {0x33fd, 0x1f},          {0x349f, 0x03},
    {0x34a6, 0x03},          {0x34a7, 0x1f},
    {0x34a8, 0x38},          {0x34a9, 0x30},
    {0x34ab, 0xd0},          {0x34ad, 0xd8},
    {0x34f8, 0x1f},          {0x34f9, 0x20},
    {0x3630, 0xa0},          {0x3631, 0x92},
    {0x3632, 0x64},          {0x3633, 0x43},
    {0x3637, 0x49},          {0x363a, 0x85},
    {0x363c, 0x0f},          {0x3650, 0x31},
    {0x3670, 0x0d},          {0x3674, 0xc0},
    {0x3675, 0xa0},          {0x3676, 0xa0},
    {0x3677, 0x92},          {0x3678, 0x96},
    {0x3679, 0x9a},          {0x367c, 0x03},
    {0x367d, 0x0f},          {0x367e, 0x01},
    {0x367f, 0x0f},          {0x3698, 0x83},
    {0x3699, 0x86},          {0x369a, 0x8c},
    {0x369b, 0x94},          {0x36a2, 0x01},
    {0x36a3, 0x03},          {0x36a4, 0x07},
    {0x36ae, 0x0f},          {0x36af, 0x1f},
    {0x36bd, 0x22},          {0x36be, 0x22},
    {0x36bf, 0x22},          {0x36d0, 0x01},
    {0x370f, 0x02},          {0x3721, 0x6c},
    {0x3722, 0x8d},          {0x3725, 0xc5},
    {0x3727, 0x14},          {0x3728, 0x04},
    {0x37b7, 0x04},          {0x37b8, 0x04},
    {0x37b9, 0x06},          {0x37bd, 0x07},
    {0x37be, 0x0f},          {0x3901, 0x02},
    {0x3903, 0x40},          {0x3905, 0x8d},
    {0x3907, 0x00},          {0x3908, 0x41},
    {0x391f, 0x41},          {0x3933, 0x80},
    {0x3934, 0x02},          {0x3937, 0x6f},
    {0x393a, 0x01},          {0x393d, 0x01},
    {0x393e, 0xc0},          {0x39dd, 0x41},
    {0x3e00, 0x00},          {0x3e01, 0x4d},
    {0x3e02, 0xc0},          {0x3e09, 0x00},
    {0x4509, 0x28},          {0x450d, 0x61},
    {0xffff, 0x00},
};
} // namespace tabcamera_detail

// One owner, used from one task. Call begin only after M5GFX display.init().
// The display already owns/enables the shared MIPI PHY LDO; this class never
// acquires, changes or releases that LDO and never accesses expander 0x44.
class TabCamera {
 static constexpr size_t FRAME_PIXELS=1280u*720u,FRAME_BYTES=FRAME_PIXELS*2;
 // Manufacturer sc202cs.h defines SC202CS_SCCB_ADDR as the 7-bit address 0x36.
 static constexpr uint8_t SENSOR_ADDRESS=0x36,CAMERA_EXPANDER=0x43,CAMERA_BIT=0x40;
 esp_cam_ctlr_handle_t controller_=nullptr;isp_proc_handle_t isp_=nullptr;
 uint16_t *frame_=nullptr;SemaphoreHandle_t finished_=nullptr;
 StaticSemaphore_t finishedStorage_{};
 bool controllerEnabled_=false,controllerStarted_=false,ispEnabled_=false,ready_=false,sensorStreaming_=false,clockHeld_=false;
 esp_clock_output_mapping_handle_t xclk_=nullptr;uint32_t clockHz_=0;
 uint16_t chipId_=0;esp_err_t error_=ESP_OK;const char *stage_="not started";
 uint32_t framesCaptured_=0,captureErrors_=0,lastCaptureAt_=0;
 volatile uint32_t finishedAt_=0,finishedBytes_=0;

 static bool readSensor(uint16_t reg,uint8_t &value){
  const uint8_t address[]={uint8_t(reg>>8),uint8_t(reg)};
  return lgfx::i2c::transactionWriteRead(1,SENSOR_ADDRESS,address,2,&value,1,100000).has_value();
 }
 static bool writeSensor(uint16_t reg,uint8_t value){
  const uint8_t data[]={uint8_t(reg>>8),uint8_t(reg),value};
  return lgfx::i2c::transactionWrite(1,SENSOR_ADDRESS,data,3,100000).has_value();
 }
 // Modify only manufacturer's camera reset/enable bit, preserving antenna,
 // audio, display and touch state on the same expander. Skip redundant writes.
 static bool cameraBit(uint8_t reg,bool set){
  auto old=lgfx::i2c::readRegister8(1,CAMERA_EXPANDER,reg,100000);
  if(!old.has_value())return false;
  const uint8_t value=uint8_t((old.value()&~CAMERA_BIT)|(set?CAMERA_BIT:0));
  if(value==old.value())return true;
  if(!lgfx::i2c::writeRegister8(1,CAMERA_EXPANDER,reg,value,0,100000).has_value())return false;
  auto checked=lgfx::i2c::readRegister8(1,CAMERA_EXPANDER,reg,100000);
  return checked.has_value()&&checked.value()==value;
 }
 static bool IRAM_ATTR onFinished(esp_cam_ctlr_handle_t,esp_cam_ctlr_trans_t *trans,void *ctx){
  auto *self=static_cast<TabCamera*>(ctx);
  if(trans->buffer!=self->frame_)return false;
  self->finishedBytes_=uint32_t(trans->received_size);
  self->finishedAt_=uint32_t(esp_timer_get_time()/1000);
  BaseType_t woken=pdFALSE;xSemaphoreGiveFromISR(self->finished_,&woken);return woken==pdTRUE;
 }
 bool fail(esp_err_t error,const char *stage){
  error_=error;stage_=stage;releaseResources();return false;
 }
 void releaseResources(){
  ready_=false;
  if(sensorStreaming_){writeSensor(0x0100,0);sensorStreaming_=false;}
  if(controllerStarted_){esp_cam_ctlr_stop(controller_);controllerStarted_=false;}
  if(ispEnabled_){esp_isp_disable(isp_);ispEnabled_=false;}
  if(isp_){esp_isp_del_processor(isp_);isp_=nullptr;}
  if(controllerEnabled_){esp_cam_ctlr_disable(controller_);controllerEnabled_=false;}
  if(controller_){esp_cam_ctlr_del(controller_);controller_=nullptr;}
  if(frame_){heap_caps_free(frame_);frame_=nullptr;}
  if(finished_){vSemaphoreDelete(finished_);finished_=nullptr;}
  if(xclk_){esp_clock_output_stop(xclk_);xclk_=nullptr;gpio_set_direction(GPIO_NUM_36,GPIO_MODE_INPUT);}
  if(clockHeld_){gpio_set_direction(GPIO_NUM_36,GPIO_MODE_INPUT);gpio_hold_dis(GPIO_NUM_36);clockHeld_=false;}
  clockHz_=0;
  // No expander/rail writes here: preserve unrelated board state even on error.
 }
 public:
 TabCamera()=default;TabCamera(const TabCamera&)=delete;TabCamera&operator=(const TabCamera&)=delete;
 ~TabCamera(){releaseResources();}
 // Read-only identification on the existing M5GFX bus. No bus initialization,
 // reset, clock generation, sensor configuration, power or charger writes.
 // Sensor must already be clocked/released for a reliable response.
 static bool readChipId(uint16_t &id){
  uint8_t high=0,low=0;if(!readSensor(0x3107,high)||!readSensor(0x3108,low))return false;
  id=uint16_t((uint16_t(high)<<8)|low);return true;
 }
 bool begin(){
  if(ready_)return true;
  releaseResources();error_=ESP_OK;stage_="clock";
  // The P4 LEDC source is GLOBAL; Arduino's active backlight normally owns
  // XTAL40, which cannot generate 24 MHz. Use a separate clock-output channel
  // instead, tapping existing SPLL480 without changing its source or frequency.
  uint32_t sourceHz=0;
  esp_err_t err=esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_SPLL,ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,&sourceHz);
  if(err!=ESP_OK||sourceHz%24000000u||sourceHz/24000000u>256||sourceHz<24000000u)
   return fail(err==ESP_OK?ESP_ERR_NOT_SUPPORTED:err,"clock source");
  // Hold the camera clock pad as INPUT while mapping/divider are configured;
  // only release it after the divider is ready, avoiding a raw PLL transient.
  err=gpio_set_direction(GPIO_NUM_36,GPIO_MODE_INPUT);if(err!=ESP_OK)return fail(err,"clock pin");
  err=gpio_hold_en(GPIO_NUM_36);if(err!=ESP_OK)return fail(err,"clock pin hold");
  clockHeld_=true;
  // gpio_set_direction(OUTPUT) resets the pin's matrix route to ordinary GPIO.
  // Do it BEFORE routing the clock; the held pad still stays electrically INPUT.
  err=gpio_set_direction(GPIO_NUM_36,GPIO_MODE_OUTPUT);if(err!=ESP_OK)return fail(err,"clock output");
  err=esp_clock_output_start(CLKOUT_SIG_SPLL,GPIO_NUM_36,&xclk_);
  if(err!=ESP_OK||!xclk_)return fail(err==ESP_OK?ESP_ERR_NO_MEM:err,"clock route");
  err=esp_clock_output_set_divider(xclk_,sourceHz/24000000u);
  if(err!=ESP_OK)return fail(err,"clock divider");
  err=gpio_hold_dis(GPIO_NUM_36);if(err!=ESP_OK)return fail(err,"clock release");
  clockHeld_=false;clockHz_=24000000;
  // GPIO 36 supplies the manufacturer's 24 MHz camera reference clock.
  if(!cameraBit(0x05,true)||!cameraBit(0x03,true)||!cameraBit(0x07,false))
   return fail(ESP_FAIL,"camera enable bit");
  delay(20);
  stage_="sensor probe";
  if(!readChipId(chipId_))return fail(ESP_ERR_NOT_FOUND,"sensor probe");
  if(chipId_!=0xeb52)return fail(ESP_ERR_NOT_SUPPORTED,"unknown sensor ID");
  stage_="sensor mode";
  for(const auto &reg:tabcamera_detail::mode720p){
   if(reg.address==0xffff)break;
   if(!writeSensor(reg.address,reg.value))return fail(ESP_FAIL,"sensor mode write");
   if(reg.address==0x0103)delay(5);
  }
  // The mode table leaves the sensor in standby until CSI/ISP are ready.
  esp_cam_ctlr_csi_config_t c={};
  c.ctlr_id=0;c.h_res=1280;c.v_res=720;c.data_lane_num=1;c.lane_bit_rate_mbps=576;
  c.input_data_color_type=CAM_CTLR_COLOR_RAW8;c.output_data_color_type=CAM_CTLR_COLOR_RGB565;
  c.queue_items=1;c.byte_swap_en=false;c.bk_buffer_dis=false;
  err=esp_cam_new_csi_ctlr(&c,&controller_);
  if(err!=ESP_OK)return fail(err,"CSI create");
  // P4 profile uses a 128-byte L2 cache line; match the configured line size
  // without depending on esp_cache_private.h, a private IDF ABI.
  constexpr size_t alignment=CONFIG_CACHE_L2_CACHE_LINE_SIZE;
  frame_=static_cast<uint16_t*>(heap_caps_aligned_alloc(alignment,FRAME_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_DMA));
  if(!frame_)return fail(ESP_ERR_NO_MEM,"frame allocation");
  finished_=xSemaphoreCreateBinaryStatic(&finishedStorage_);
  if(!finished_)return fail(ESP_ERR_NO_MEM,"capture semaphore");
  esp_cam_ctlr_evt_cbs_t callbacks={};callbacks.on_trans_finished=onFinished;
  err=esp_cam_ctlr_register_event_callbacks(controller_,&callbacks,this);
  if(err!=ESP_OK)return fail(err,"CSI callbacks");
  err=esp_cam_ctlr_enable(controller_);if(err!=ESP_OK)return fail(err,"CSI enable");
  controllerEnabled_=true;
  esp_isp_processor_cfg_t processing={};
  processing.clk_hz=80000000;processing.input_data_source=ISP_INPUT_DATA_SOURCE_CSI;
  processing.input_data_color_type=ISP_COLOR_RAW8;processing.output_data_color_type=ISP_COLOR_RGB565;
  processing.h_res=1280;processing.v_res=720;processing.bayer_order=COLOR_RAW_ELEMENT_ORDER_BGGR;
  processing.has_line_start_packet=false;processing.has_line_end_packet=false;
  processing.flags.byte_swap_en=false;
  err=esp_isp_new_processor(&processing,&isp_);if(err!=ESP_OK)return fail(err,"ISP create");
  err=esp_isp_enable(isp_);if(err!=ESP_OK)return fail(err,"ISP enable");
  ispEnabled_=true;
  // Remain in sensor standby with our CSI stopped until a still is requested.
  ready_=true;stage_="ready";return true;
 }
 bool capture(uint8_t *out,size_t capacity=4800,uint32_t timeoutMs=450){
  if(!out||capacity<4800||timeoutMs==0){error_=ESP_ERR_INVALID_ARG;return false;}
  if(!ready_){error_=ESP_ERR_INVALID_STATE;return false;}
  if(timeoutMs>450)timeoutMs=450;
  while(xSemaphoreTake(finished_,0)==pdTRUE){}
  finishedBytes_=0;
  esp_err_t err=esp_cam_ctlr_start(controller_);
  if(err!=ESP_OK){++captureErrors_;return fail(err,"CSI capture start");}
  controllerStarted_=true;
  esp_cam_ctlr_trans_t transaction={};transaction.buffer=frame_;transaction.buflen=FRAME_BYTES;
  err=esp_cam_ctlr_receive(controller_,&transaction,20);
  if(err!=ESP_OK){++captureErrors_;return fail(err,"capture queue");}
  if(!writeSensor(0x0100,1)){++captureErrors_;return fail(ESP_FAIL,"sensor stream start");}
  sensorStreaming_=true;
  if(xSemaphoreTake(finished_,pdMS_TO_TICKS(timeoutMs))!=pdTRUE){
   ++captureErrors_;return fail(ESP_ERR_TIMEOUT,"capture timeout");
  }
  // The first complete sensor frame goes to the CSI backup buffer. Only the
  // following app buffer, queued by THIS capture, can signal the semaphore.
  // Put just the sensor/our CSI back in standby; leave display and PHY rail on.
  if(!writeSensor(0x0100,0)){++captureErrors_;return fail(ESP_FAIL,"sensor stream stop");}
  sensorStreaming_=false;
  err=esp_cam_ctlr_stop(controller_);
  if(err!=ESP_OK){++captureErrors_;return fail(err,"CSI capture stop");}
  controllerStarted_=false;
  err=esp_cache_msync(frame_,FRAME_BYTES,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(err!=ESP_OK){++captureErrors_;return fail(err,"frame cache sync");}
  const uint32_t at=finishedAt_,now=uint32_t(esp_timer_get_time()/1000);
  if(finishedBytes_!=FRAME_BYTES||uint32_t(now-at)>=500){
   ++captureErrors_;return fail(ESP_ERR_INVALID_SIZE,"capture stale/short");
  }
  if(!tabcamera_detail::letterbox(frame_,FRAME_PIXELS,out,capacity)){
   ++captureErrors_;return fail(ESP_ERR_INVALID_SIZE,"frame conversion");
  }
  lastCaptureAt_=at;++framesCaptured_;error_=ESP_OK;stage_="ready";return true;
 }
 void end(){releaseResources();stage_="stopped";}
 bool ready()const{return ready_;}
 const char *model()const{return chipId_==0xeb52?"SC2356/SC202CS":"unidentified";}
 uint16_t chipId()const{return chipId_;}
 esp_err_t error()const{return error_;}
 const char *stage()const{return stage_;}
 uint32_t clockHz()const{return clockHz_;}
 uint32_t framesCaptured()const{return framesCaptured_;}
 uint32_t captureErrors()const{return captureErrors_;}
 uint32_t lastCaptureAt()const{return lastCaptureAt_;}
};
#endif // TAB_CAMERA_HOST_TEST
