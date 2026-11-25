// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <Wire.h>
#include <driver/spi_master.h>
#include <driver/i2s_std.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_io.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include "Board.h"
#include "PanelInit.h"
#include "Sound.h"
#include "es8311.h"
namespace waveshare_eyes {
int audioStage=0,audioResult=0,codecPower=-1,batteryRaw=-1;
uint32_t displayTransfers=0,displayErrors=0,audioPlays=0,audioErrors=0;
namespace {
esp_lcd_panel_handle_t panel=nullptr;
esp_lcd_panel_io_handle_t panelIO=nullptr;
SemaphoreHandle_t completed=nullptr;
uint16_t* pixels=nullptr;
bool displayOK=false;
i2s_chan_handle_t tx=nullptr;
esp_timer_handle_t ampTimer=nullptr;
bool audioOK=false,playing=false;
uint32_t soundAt=0;
int16_t pcm[(soundSamples+480)*2];
bool readReg(uint8_t address,uint8_t reg,uint8_t* data,size_t size){
 Wire.beginTransmission(address);Wire.write(reg);
 if(Wire.endTransmission(true)!=0)return false;
 if(Wire.requestFrom(address,size)!=size){while(Wire.available())Wire.read();return false;}
 for(size_t i=0;i<size;i++)data[i]=Wire.read();return true;
}
bool writeReg(uint8_t address,uint8_t reg,uint8_t value){
 Wire.beginTransmission(address);Wire.write(reg);Wire.write(value);return Wire.endTransmission(true)==0;
}
bool IRAM_ATTR transferDone(esp_lcd_panel_io_handle_t,esp_lcd_panel_io_event_data_t*,void*){
 BaseType_t wake=pdFALSE;xSemaphoreGiveFromISR(completed,&wake);return wake==pdTRUE;
}
void amplifierOff(void*){digitalWrite(15,LOW);}
}
bool boardBegin(){
 pinMode(15,OUTPUT);digitalWrite(15,LOW);
 pinMode(5,OUTPUT);digitalWrite(5,LOW);
 Wire.begin(11,10,400000);Wire.setTimeOut(12);
 uint8_t config=0,output=0;
 if(!readReg(0x20,3,&config,1)||!readReg(0x20,1,&output,1))return false;
 // Preserve unrelated expander outputs; pulse only touch/display reset bits.
 if(!writeReg(0x20,1,output&~3)||!writeReg(0x20,3,config&~3))return false;
 delay(10);if(!writeReg(0x20,1,output|3))return false;delay(120);
 pinMode(4,INPUT_PULLUP);
 if(!writeReg(0x15,0xfe,0xff))return false; // disable CST816 auto-sleep for bounded polling
 spi_bus_config_t bus={};bus.data0_io_num=46;bus.data1_io_num=45;bus.sclk_io_num=40;
 bus.data2_io_num=42;bus.data3_io_num=41;bus.data4_io_num=bus.data5_io_num=bus.data6_io_num=bus.data7_io_num=-1;
 bus.max_transfer_sz=360*stripRows*2;bus.flags=SPICOMMON_BUSFLAG_MASTER;
 if(spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO)!=ESP_OK)return false;
 esp_lcd_panel_io_spi_config_t io={};io.cs_gpio_num=21;io.dc_gpio_num=-1;io.spi_mode=0;
 io.pclk_hz=5000000;io.trans_queue_depth=1;io.lcd_cmd_bits=32;io.lcd_param_bits=8;io.flags.quad_mode=1;
 if(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,&io,&panelIO)!=ESP_OK)return false;
 uint8_t id[4]={};auto err=esp_lcd_panel_io_rx_param(panelIO,0x0b000400,id,sizeof id);
 esp_lcd_panel_io_del(panelIO);panelIO=nullptr; // the low-speed read object must not leak
 if(err!=ESP_OK||id[0]!=0||id[1]!=2||id[2]!=0x7f||id[3]!=0x7f)return false;
 completed=xSemaphoreCreateBinary();pixels=(uint16_t*)heap_caps_malloc(bus.max_transfer_sz,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
 if(!completed||!pixels)return false;
 io.pclk_hz=40000000;io.on_color_trans_done=transferDone;
 if(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,&io,&panelIO)!=ESP_OK)return false;
 st77916_vendor_config_t vendor={};vendor.flags.use_qspi_interface=1;
 vendor.init_cmds=vendor_specific_init_new;vendor.init_cmds_size=sizeof vendor_specific_init_new/sizeof vendor_specific_init_new[0];
 esp_lcd_panel_dev_config_t pc={};pc.reset_gpio_num=-1;pc.rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB;
 pc.bits_per_pixel=16;pc.data_endian=LCD_RGB_DATA_ENDIAN_BIG;pc.vendor_config=&vendor;
 if(esp_lcd_new_panel_st77916(panelIO,&pc,&panel)!=ESP_OK||esp_lcd_panel_init(panel)!=ESP_OK||esp_lcd_panel_disp_on_off(panel,true)!=ESP_OK)return false;
 displayOK=true;
 // Start from black; the strip is reused only after completion.
 memset(pixels,0,bus.max_transfer_sz);
 for(int y=0;y<360;y+=stripRows)if(!displayStrip(y,pixels))return false;
 if(!ledcAttach(5,20000,10))return false;ledcWrite(5,400);
 analogReadResolution(12);analogSetPinAttenuation(8,ADC_11db);
 return true;
}
bool readTouch(uint8_t* data){return readReg(0x15,1,data,6);}
int readBattery(){uint32_t mv=0;for(int i=0;i<4;i++)mv+=analogReadMilliVolts(8);mv=mv*3/4;batteryRaw=mv;return mv>=2500&&mv<=4500?int(mv):-1;}
uint16_t* displayBuffer(){return displayOK?pixels:nullptr;}
bool displayStrip(int y,const uint16_t* data){
 if(!displayOK||y<0||y+stripRows>360)return false;
 if(esp_lcd_panel_draw_bitmap(panel,0,y,360,y+stripRows,data)!=ESP_OK||xSemaphoreTake(completed,pdMS_TO_TICKS(80))!=pdTRUE){
  // Latch failure. Never modify/free a buffer that may still be owned by DMA.
  ++displayErrors;displayOK=false;digitalWrite(15,LOW);return false;
 }
 ++displayTransfers;return true;
}
bool audioBegin(){
 i2s_chan_config_t channel=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
 channel.dma_desc_num=10;channel.dma_frame_num=240;channel.auto_clear=true;
 audioStage=1;audioResult=i2s_new_channel(&channel,&tx,nullptr);if(audioResult!=ESP_OK)return false;
 i2s_std_config_t cfg={};cfg.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(sampleRate);
 cfg.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO);
 cfg.gpio_cfg.mclk=GPIO_NUM_2;cfg.gpio_cfg.bclk=GPIO_NUM_48;cfg.gpio_cfg.ws=GPIO_NUM_38;
 cfg.gpio_cfg.dout=GPIO_NUM_47;cfg.gpio_cfg.din=I2S_GPIO_UNUSED; // TX only: no microphone channel
 audioStage=2;audioResult=i2s_channel_init_std_mode(tx,&cfg);if(audioResult!=ESP_OK)return false;
 audioStage=3;auto codec=es8311_create(I2C_NUM_0,ES8311_ADDRRES_0);if(!codec)return false;
 es8311_clock_config_t clk={};clk.mclk_from_mclk_pin=true;clk.mclk_frequency=sampleRate*256;clk.sample_frequency=sampleRate;
 audioStage=4;audioResult=es8311_init(codec,&clk,ES8311_RESOLUTION_16,ES8311_RESOLUTION_16);if(audioResult!=ESP_OK)return false;
 audioStage=5;audioResult=es8311_voice_volume_set(codec,45,nullptr);if(audioResult!=ESP_OK)return false;
 // The local vendor adaptation leaves ES8311 ADC/PGA powered down. ES7210 is never initialized.
 audioStage=6;uint8_t power=0;if(!readReg(0x18,0x0e,&power,1))return false;codecPower=power;if(!codecCaptureOff(power))return false;
 esp_timer_create_args_t timer={};timer.callback=amplifierOff;timer.name="eyes_amp_off";
 audioStage=7;audioResult=esp_timer_create(&timer,&ampTimer);if(audioResult!=ESP_OK)return false;
 audioResult=i2s_channel_enable(tx);if(audioResult!=ESP_OK)return false;
 delay(300); // settle DAC clocks with amplifier off and cleared DMA silence
 audioResult=i2s_channel_disable(tx);if(audioResult!=ESP_OK)return false;
 audioStage=8;audioOK=true;return true;
}
bool setBrightness(unsigned level){return ledcWrite(5,level?400:150);}
bool playSound(unsigned eye,unsigned volume){
 if(!audioOK||playing||!eye)return false;
 for(unsigned i=0;i<soundSamples+480;i++){
  int16_t s=i>=240&&i<soundSamples+240?volumeSample(soundSample(i-240,eye),volume):0;pcm[i*2]=pcm[i*2+1]=s;
 }
 size_t loaded=0;
 if(i2s_channel_preload_data(tx,pcm,sizeof pcm,&loaded)!=ESP_OK||loaded!=sizeof pcm){++audioErrors;audioOK=false;return false;}
 if(i2s_channel_enable(tx)!=ESP_OK){++audioErrors;audioOK=false;return false;}
 // Independent timer caps amplifier-on time even if drawing or USB stalls.
 digitalWrite(15,HIGH);
 if(esp_timer_start_once(ampTimer,110000)!=ESP_OK){digitalWrite(15,LOW);i2s_channel_disable(tx);++audioErrors;return false;}
 soundAt=millis();playing=true;++audioPlays;return true;
}
void stopSound(){digitalWrite(15,LOW);if(ampTimer)esp_timer_stop(ampTimer);if(playing){i2s_channel_disable(tx);playing=false;}}
void pumpSound(){if(playing&&uint32_t(millis()-soundAt)>=110)stopSound();}
bool audioAvailable(){return audioOK;}
bool ampEnabled(){return digitalRead(15)==HIGH;}
}
