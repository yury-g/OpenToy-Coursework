// SPDX-License-Identifier: MIT
// Hardware timing/pins: Elecrow DHE04209D V1.0 Lesson05/07, see THIRD-PARTY.md.
#include "Board.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#define TRY(call) do { esp_err_t e=(call);if(e!=ESP_OK)return e; } while(0)
static esp_lcd_touch_handle_t touch;
static esp_ldo_channel_handle_t ldo3,ldo4;
// IDF's generic LCD I2C transport waits indefinitely. This register-only
// transport keeps GT911 transactions bounded without changing its decoder.
static i2c_master_dev_handle_t touch_device;
static bool touch_packet_ready;
static esp_err_t touch_rx(esp_lcd_panel_io_t* io,int reg,void* data,size_t size){
 if(reg<0||reg>65535||size>64)return ESP_ERR_INVALID_ARG;
 uint8_t address[2]={(uint8_t)(reg>>8),(uint8_t)reg};
 esp_err_t result=i2c_master_transmit_receive(touch_device,address,2,data,size,20);
 if(reg==0x814e&&size==1)touch_packet_ready=result==ESP_OK&&(*(uint8_t*)data&0x80);
 return result;
}
static esp_err_t touch_tx(esp_lcd_panel_io_t* io,int reg,const void* data,size_t size){
 if(reg<0||reg>65535||size>62)return ESP_ERR_INVALID_ARG;
 uint8_t bytes[64]={(uint8_t)(reg>>8),(uint8_t)reg};
 if(size)memcpy(bytes+2,data,size);
 return i2c_master_transmit(touch_device,bytes,size+2,20);
}
static esp_err_t touch_io_delete(esp_lcd_panel_io_t* io){return i2c_master_bus_rm_device(touch_device);}
static esp_lcd_panel_io_t touch_io={.rx_param=touch_rx,.tx_param=touch_tx,.del=touch_io_delete};
esp_err_t board_brightness(unsigned percent){
 if(percent>100)return ESP_ERR_INVALID_ARG;
 TRY(ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,percent?percent*18+200:0));
 return ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
}
esp_err_t board_display_begin(void){
 esp_ldo_channel_config_t l3={.chan_id=3,.voltage_mv=2500},l4={.chan_id=4,.voltage_mv=3300};
 TRY(esp_ldo_acquire_channel(&l3,&ldo3));TRY(esp_ldo_acquire_channel(&l4,&ldo4));
 ledc_timer_config_t timer={.speed_mode=LEDC_LOW_SPEED_MODE,.duty_resolution=LEDC_TIMER_11_BIT,.timer_num=LEDC_TIMER_0,.freq_hz=30000,.clk_cfg=LEDC_USE_PLL_DIV_CLK};
 ledc_channel_config_t light={.gpio_num=31,.speed_mode=LEDC_LOW_SPEED_MODE,.channel=LEDC_CHANNEL_0,.timer_sel=LEDC_TIMER_0,.duty=0};
 TRY(ledc_timer_config(&timer));TRY(ledc_channel_config(&light));
 esp_lcd_dsi_bus_handle_t bus;esp_lcd_panel_io_handle_t io;esp_lcd_panel_handle_t panel;
 esp_lcd_dsi_bus_config_t bus_cfg={.bus_id=0,.num_data_lanes=2,.phy_clk_src=MIPI_DSI_PHY_CLK_SRC_DEFAULT,.lane_bit_rate_mbps=900};
 TRY(esp_lcd_new_dsi_bus(&bus_cfg,&bus));
 esp_lcd_dbi_io_config_t io_cfg={.virtual_channel=0,.lcd_cmd_bits=8,.lcd_param_bits=8};
 TRY(esp_lcd_new_panel_io_dbi(bus,&io_cfg,&io));
 esp_lcd_dpi_panel_config_t dpi={.virtual_channel=0,.dpi_clk_src=MIPI_DSI_DPI_CLK_SRC_DEFAULT,.dpi_clock_freq_mhz=51,.pixel_format=LCD_COLOR_PIXEL_FORMAT_RGB565,.num_fbs=1,
  .video_timing={.h_size=1024,.v_size=600,.hsync_pulse_width=70,.hsync_back_porch=160,.hsync_front_porch=160,.vsync_pulse_width=10,.vsync_back_porch=23,.vsync_front_porch=12},.flags.use_dma2d=true};
 ek79007_vendor_config_t vendor={.mipi_config={.dsi_bus=bus,.dpi_config=&dpi}};
 esp_lcd_panel_dev_config_t panel_cfg={.reset_gpio_num=-1,.rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB,.bits_per_pixel=16,.vendor_config=&vendor};
 TRY(esp_lcd_new_panel_ek79007(io,&panel_cfg,&panel));TRY(esp_lcd_panel_reset(panel));TRY(esp_lcd_panel_init(panel));
 lvgl_port_cfg_t lv=ESP_LVGL_PORT_INIT_CONFIG();lv.task_stack=16384;lv.task_priority=4;lv.task_max_sleep_ms=10;
 TRY(lvgl_port_init(&lv));
 lvgl_port_display_cfg_t display={.io_handle=io,.panel_handle=panel,.control_handle=panel,.buffer_size=1024*100,.double_buffer=true,.hres=1024,.vres=600,.monochrome=false,.flags={.buff_spiram=true}};
 lvgl_port_display_dsi_cfg_t dsi={0};
 if(!lvgl_port_add_disp_dsi(&display,&dsi))return ESP_FAIL;
 return ESP_OK;
}
esp_err_t board_touch_begin(void){
 i2c_master_bus_handle_t bus;
 i2c_master_bus_config_t bus_cfg={.i2c_port=0,.sda_io_num=45,.scl_io_num=46,.clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
 TRY(i2c_new_master_bus(&bus_cfg,&bus));
 esp_lcd_touch_config_t cfg={.x_max=1024,.y_max=600,.rst_gpio_num=40,.int_gpio_num=42,.levels={.reset=0,.interrupt=0}};
 // Driver performs the GT911 reset/address-selection sequence. Retry only its two documented addresses.
 for(unsigned attempt=0;attempt<2;attempt++){
  i2c_device_config_t device={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=attempt?ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP:ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,.scl_speed_hz=400000};
  esp_lcd_touch_io_gt911_config_t selection={.dev_addr=device.device_address};cfg.driver_data=&selection;
  TRY(i2c_master_bus_add_device(bus,&device,&touch_device));
  esp_err_t result=esp_lcd_touch_new_i2c_gt911(&touch_io,&cfg,&touch);
  if(result==ESP_OK)return ESP_OK;
  esp_lcd_panel_io_del(&touch_io);
 }
 return ESP_FAIL;
}
esp_err_t board_touch_read(uint16_t* x,uint16_t* y,uint8_t* count){
 *count=0;if(!touch)return ESP_ERR_INVALID_STATE;
 touch_packet_ready=false;
 TRY(esp_lcd_touch_read_data(touch));
 if(!touch_packet_ready)return ESP_ERR_NOT_FINISHED;
 uint16_t strength[5];esp_lcd_touch_get_coordinates(touch,x,y,strength,count,5);
 return ESP_OK;
}
