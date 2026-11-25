// SPDX-License-Identifier: MIT
#include "BoardTab5.h"

#include <algorithm>
#include <cstring>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_cache.h"
#include "esp_codec_dev_defaults.h"
#include "esp_err.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_st7123.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_touch_st7123.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panel_init/disp_init_data.h"
#include "panel_init/disp_init_data_1.h"
#include "panel_init/disp_init_data_st7121.h"

namespace opentoy {
namespace tab5 {

namespace {

constexpr uint8_t kPi4ioe1Addr = 0x43;
constexpr uint8_t kPi4ioe2Addr = 0x44;
constexpr uint8_t kEs8388Addr7 = 0x10;
constexpr uint8_t kEs7210Addr7 = 0x40;
constexpr uint8_t kBmi270AddrLow = 0x68;
constexpr uint8_t kBmi270AddrHigh = 0x69;
constexpr uint8_t kRx8130Addr = 0x32;

constexpr gpio_num_t kLcdBacklightPin = GPIO_NUM_22;
constexpr gpio_num_t kTouchIntPin = GPIO_NUM_23;

constexpr gpio_num_t kI2sMclkPin = GPIO_NUM_30;
constexpr gpio_num_t kI2sBclkPin = GPIO_NUM_27;
constexpr gpio_num_t kI2sWsPin = GPIO_NUM_29;
constexpr gpio_num_t kI2sDoutPin = GPIO_NUM_26;
constexpr gpio_num_t kI2sDinPin = GPIO_NUM_28;

constexpr int kPanelNativeW = 720;
constexpr int kPanelNativeH = 1280;
constexpr size_t kPanelFbBytes = size_t(kPanelNativeW) * size_t(kPanelNativeH) * sizeof(uint16_t);

esp_err_t writeReg8(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val) {
  const uint8_t buf[2] = {reg, val};
  return i2c_master_transmit(dev, buf, sizeof(buf), 50);
}

bool readReg8(i2c_master_bus_handle_t bus, uint8_t addr7, uint8_t reg, uint8_t& out) {
  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = addr7;
  cfg.scl_speed_hz = 100000;
  i2c_master_dev_handle_t dev = nullptr;
  if (i2c_master_bus_add_device(bus, &cfg, &dev) != ESP_OK) return false;
  const esp_err_t err = i2c_master_transmit_receive(dev, &reg, 1, &out, 1, 40);
  (void)i2c_master_bus_rm_device(dev);
  return err == ESP_OK;
}

}  // namespace

BoardTab5::~BoardTab5() {
  end();
}

bool BoardTab5::pulseLcdResetAndInitIo2() {
  if (!i2c_bus_) return false;

  // Safely pulse LCD_RST (P4 on PI4IOE1 0x43) open-drain:
  // OUT_SET bit4=0 (0V latch), P6=1 (CAM_RST high), P5=1 (TOUCH_EN high), P2=1, P1=0 (SPEAKER_EN off).
  // Drive IO_DIR bit4=1 briefly to pull LCD_RST low, then IO_DIR bit4=0 to release via pull-up
  // without driving 3.3V push-pull onto 1.8V LCD_RST.
  i2c_device_config_t cfg1 = {};
  cfg1.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg1.device_address = kPi4ioe1Addr;
  cfg1.scl_speed_hz = 400000;
  i2c_master_dev_handle_t io1 = nullptr;
  if (i2c_master_bus_add_device(i2c_bus_, &cfg1, &io1) != ESP_OK) return false;

  esp_err_t err = writeReg8(io1, 0x05 /*OUT_SET*/, 0b01100100);
  if (err == ESP_OK) err = writeReg8(io1, 0x03 /*IO_DIR*/, 0b01111111);  // P4 output low
  vTaskDelay(pdMS_TO_TICKS(15));
  if (err == ESP_OK) err = writeReg8(io1, 0x03 /*IO_DIR*/, 0b01101111);  // P4 input pull-up release
  (void)i2c_master_bus_rm_device(io1);
  if (err != ESP_OK) return false;

  // Probe PI4IOE2 (0x44) non-destructively.
  if (i2c_master_probe(i2c_bus_, kPi4ioe2Addr, 40) == ESP_OK) {
    qual_.pi4ioe2_ok = true;
  }

  // Allow LCD + touch controller internal MCU boot time before I2C touch probe.
  vTaskDelay(pdMS_TO_TICKS(280));
  return true;
}

void BoardTab5::probePeripherals() {
  if (!i2c_bus_) return;

  if (i2c_master_probe(i2c_bus_, kEs8388Addr7, 40) == ESP_OK ||
      i2c_master_probe(i2c_bus_, 0x11, 40) == ESP_OK) {
    qual_.es8388_ok = true;
  }

  if (i2c_master_probe(i2c_bus_, kEs7210Addr7, 40) == ESP_OK) {
    qual_.es7210_ok = true;
    uint8_t id_hi = 0, id_lo = 0;
    if (readReg8(i2c_bus_, kEs7210Addr7, 0x3D, id_lo) &&
        readReg8(i2c_bus_, kEs7210Addr7, 0x3E, id_hi)) {
      qual_.es7210_id = (uint16_t(id_hi) << 8) | id_lo;
    }
  }

  uint8_t bmi_id = 0;
  if (readReg8(i2c_bus_, kBmi270AddrLow, 0x00, bmi_id) ||
      readReg8(i2c_bus_, kBmi270AddrHigh, 0x00, bmi_id)) {
    qual_.bmi270_chip_id = bmi_id;
    qual_.bmi270_ok = (bmi_id == 0x24 || bmi_id != 0x00);
  }

  if (i2c_master_probe(i2c_bus_, kRx8130Addr, 40) == ESP_OK) {
    qual_.rx8130_ok = true;
  }
}

Tab5PanelRev BoardTab5::detectPanelRevision() {
  // 1. Probe ST712x touch at 0x55 (Board Ver 2 ST7123 or Ver 3 ST7121).
  if (i2c_master_probe(i2c_bus_, ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS, 80) == ESP_OK) {
    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS;
    io_cfg.control_phase_bytes = 1;
    io_cfg.lcd_cmd_bits = 16;
    io_cfg.flags.disable_control_phase = 1;
    io_cfg.scl_speed_hz = 100000;
    uint8_t fw_ver = 0;
    if (esp_lcd_new_panel_io_i2c(i2c_bus_, &io_cfg, &io) == ESP_OK) {
      const esp_err_t rx_err = esp_lcd_panel_io_rx_param(io, 0x0000, &fw_ver, sizeof(fw_ver));
      (void)esp_lcd_panel_io_del(io);
      if (rx_err == ESP_OK) {
        qual_.touch_i2c_addr = ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS;
        qual_.touch_fw_ver = fw_ver;
        if (fw_ver == 1) {
          qual_.panel_rev = Tab5PanelRev::St7121;
          qual_.panel_name = "ST7121";
          return Tab5PanelRev::St7121;
        }
        qual_.panel_rev = Tab5PanelRev::St7123;
        qual_.panel_name = "ST7123";
        return Tab5PanelRev::St7123;
      }
    }
  }

  // 2. Drive GPIO23 low per official Tab5 BSP Ver 1 fix, then probe GT911 (0x14 backup / 0x5D primary).
  gpio_config_t int_cfg = {};
  int_cfg.mode = GPIO_MODE_OUTPUT;
  int_cfg.intr_type = GPIO_INTR_DISABLE;
  int_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  int_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  int_cfg.pin_bit_mask = (1ULL << kTouchIntPin);
  (void)gpio_config(&int_cfg);
  (void)gpio_set_level(kTouchIntPin, 0);
  vTaskDelay(pdMS_TO_TICKS(10));

  for (uint8_t addr : {uint8_t(ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP),
                       uint8_t(ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS)}) {
    if (i2c_master_probe(i2c_bus_, addr, 80) == ESP_OK) {
      qual_.touch_i2c_addr = addr;
      qual_.panel_rev = Tab5PanelRev::Ili9881cGt911;
      qual_.panel_name = "ILI9881C-GT911";
      return Tab5PanelRev::Ili9881cGt911;
    }
  }

  qual_.panel_rev = Tab5PanelRev::Unknown;
  qual_.panel_name = "UNKNOWN";
  return Tab5PanelRev::Unknown;
}

bool BoardTab5::initBacklight() {
  ledc_timer_config_t tcfg = {};
  tcfg.speed_mode = LEDC_LOW_SPEED_MODE;
  tcfg.duty_resolution = LEDC_TIMER_10_BIT;
  tcfg.timer_num = LEDC_TIMER_0;  // Camera MCLK uses LEDC_TIMER_2
  tcfg.freq_hz = 5000;
  tcfg.clk_cfg = LEDC_AUTO_CLK;
  if (ledc_timer_config(&tcfg) != ESP_OK) return false;

  ledc_channel_config_t ccfg = {};
  ccfg.gpio_num = kLcdBacklightPin;
  ccfg.speed_mode = LEDC_LOW_SPEED_MODE;
  ccfg.channel = LEDC_CHANNEL_0;  // Camera MCLK uses LEDC_CHANNEL_2
  ccfg.intr_type = LEDC_INTR_DISABLE;
  ccfg.timer_sel = LEDC_TIMER_0;
  ccfg.duty = 0;
  ccfg.hpoint = 0;
  if (ledc_channel_config(&ccfg) != ESP_OK) return false;

  backlight_inited_ = true;
  return true;
}

bool BoardTab5::setBacklightPercent(int percent) {
  if (!backlight_inited_) return false;
  percent = std::max(0, std::min(100, percent));
  const uint32_t duty = uint32_t((1023 * percent) / 100);
  if (ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty) != ESP_OK) return false;
  return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) == ESP_OK;
}

bool BoardTab5::initDisplayPanel(Tab5PanelRev rev) {
  if (rev == Tab5PanelRev::Unknown) {
    qual_.display_err = ESP_ERR_NOT_SUPPORTED;
    return false;
  }

  esp_lcd_dsi_bus_config_t bus_cfg = {};
  bus_cfg.bus_id = 0;
  bus_cfg.num_data_lanes = 2;
  bus_cfg.phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT;
  bus_cfg.lane_bit_rate_mbps = (rev == Tab5PanelRev::St7121) ? 965 : 1000;

  esp_err_t err = esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus_);
  if (err != ESP_OK) {
    qual_.display_err = err;
    return false;
  }

  esp_lcd_dbi_io_config_t dbi_cfg = {};
  dbi_cfg.virtual_channel = 0;
  dbi_cfg.lcd_cmd_bits = 8;
  dbi_cfg.lcd_param_bits = 8;
  err = esp_lcd_new_panel_io_dbi(dsi_bus_, &dbi_cfg, &dbi_io_);
  if (err != ESP_OK) {
    qual_.display_err = err;
    return false;
  }

  esp_lcd_dpi_panel_config_t dpi_cfg = {};
  dpi_cfg.virtual_channel = 0;
  dpi_cfg.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
  dpi_cfg.in_color_format = LCD_COLOR_FMT_RGB565;
  dpi_cfg.num_fbs = 1;
  dpi_cfg.video_timing.h_size = kPanelNativeW;
  dpi_cfg.video_timing.v_size = kPanelNativeH;

  if (rev == Tab5PanelRev::Ili9881cGt911) {
    dpi_cfg.dpi_clock_freq_mhz = 60;
    dpi_cfg.video_timing.hsync_back_porch = 140;
    dpi_cfg.video_timing.hsync_pulse_width = 40;
    dpi_cfg.video_timing.hsync_front_porch = 40;
    dpi_cfg.video_timing.vsync_back_porch = 20;
    dpi_cfg.video_timing.vsync_pulse_width = 4;
    dpi_cfg.video_timing.vsync_front_porch = 20;

    ili9881c_vendor_config_t vcfg = {};
    vcfg.init_cmds = disp_init_data_ili9881c;
    vcfg.init_cmds_size = sizeof(disp_init_data_ili9881c) / sizeof(disp_init_data_ili9881c[0]);
    vcfg.mipi_config.dsi_bus = dsi_bus_;
    vcfg.mipi_config.dpi_config = &dpi_cfg;
    vcfg.mipi_config.lane_num = 2;

    esp_lcd_panel_dev_config_t pcfg = {};
    pcfg.reset_gpio_num = GPIO_NUM_NC;
    pcfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    pcfg.bits_per_pixel = 16;
    pcfg.vendor_config = &vcfg;
    err = esp_lcd_new_panel_ili9881c(dbi_io_, &pcfg, &panel_);
  } else {
    dpi_cfg.dpi_clock_freq_mhz = 70;
    dpi_cfg.video_timing.hsync_back_porch = 40;
    dpi_cfg.video_timing.hsync_pulse_width = 2;
    dpi_cfg.video_timing.hsync_front_porch = 40;
    if (rev == Tab5PanelRev::St7121) {
      dpi_cfg.video_timing.vsync_back_porch = 24;
      dpi_cfg.video_timing.vsync_pulse_width = 20;
      dpi_cfg.video_timing.vsync_front_porch = 200;
    } else {
      dpi_cfg.video_timing.vsync_back_porch = 8;
      dpi_cfg.video_timing.vsync_pulse_width = 2;
      dpi_cfg.video_timing.vsync_front_porch = 220;
    }

    st7123_vendor_config_t vcfg = {};
    if (rev == Tab5PanelRev::St7121) {
      vcfg.init_cmds = disp_init_data_st7121;
      vcfg.init_cmds_size = sizeof(disp_init_data_st7121) / sizeof(disp_init_data_st7121[0]);
    } else {
      vcfg.init_cmds = disp_init_data_st7123;
      vcfg.init_cmds_size = sizeof(disp_init_data_st7123) / sizeof(disp_init_data_st7123[0]);
    }
    vcfg.mipi_config.dsi_bus = dsi_bus_;
    vcfg.mipi_config.dpi_config = &dpi_cfg;

    esp_lcd_panel_dev_config_t pcfg = {};
    pcfg.reset_gpio_num = GPIO_NUM_NC;
    pcfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    pcfg.bits_per_pixel = 16;
    pcfg.vendor_config = &vcfg;
    err = esp_lcd_new_panel_st7123(dbi_io_, &pcfg, &panel_);
  }

  if (err != ESP_OK) {
    qual_.display_err = err;
    return false;
  }

  (void)esp_lcd_panel_reset(panel_);
  err = esp_lcd_panel_init(panel_);
  if (err != ESP_OK) {
    qual_.display_err = err;
    return false;
  }
  (void)esp_lcd_panel_invert_color(panel_, false);
  (void)esp_lcd_panel_mirror(panel_, false, false);
  (void)esp_lcd_panel_disp_on_off(panel_, true);

  void* fb0 = nullptr;
  err = esp_lcd_dpi_panel_get_frame_buffer(panel_, 1, &fb0);
  if (err != ESP_OK || !fb0) {
    qual_.display_err = (err != ESP_OK) ? err : ESP_ERR_INVALID_STATE;
    return false;
  }
  dpi_fb_ = static_cast<uint16_t*>(fb0);
  std::memset(dpi_fb_, 0, kPanelFbBytes);
  (void)esp_cache_msync(
      dpi_fb_, kPanelFbBytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);

  qual_.display_ready = true;
  return true;
}

bool BoardTab5::initTouchController(Tab5PanelRev rev) {
  esp_lcd_touch_config_t tp_cfg = {};
  tp_cfg.x_max = kPanelNativeW;
  tp_cfg.y_max = kPanelNativeH;
  tp_cfg.rst_gpio_num = GPIO_NUM_NC;
  tp_cfg.int_gpio_num = kTouchIntPin;

  esp_err_t err = ESP_ERR_NOT_SUPPORTED;
  if (rev == Tab5PanelRev::Ili9881cGt911) {
    tp_cfg.int_gpio_num = GPIO_NUM_NC;  // Held low on Ver 1
    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr = qual_.touch_i2c_addr ? qual_.touch_i2c_addr
                                           : uint8_t(ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP);
    io_cfg.control_phase_bytes = 1;
    io_cfg.dc_bit_offset = 0;
    io_cfg.lcd_cmd_bits = 16;
    io_cfg.flags.disable_control_phase = 1;
    io_cfg.scl_speed_hz = 100000;
    err = esp_lcd_new_panel_io_i2c(i2c_bus_, &io_cfg, &tp_io_);
    if (err == ESP_OK) {
      err = esp_lcd_touch_new_i2c_gt911(tp_io_, &tp_cfg, &tp_);
    }
  } else if (rev == Tab5PanelRev::St7123 || rev == Tab5PanelRev::St7121) {
    // Release GPIO23 as input pull-up for ST712x touch IRQ.
    gpio_config_t int_in = {};
    int_in.mode = GPIO_MODE_INPUT;
    int_in.pull_up_en = GPIO_PULLUP_ENABLE;
    int_in.pin_bit_mask = (1ULL << kTouchIntPin);
    (void)gpio_config(&int_in);

    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS;
    io_cfg.control_phase_bytes = 1;
    io_cfg.lcd_cmd_bits = 16;
    io_cfg.flags.disable_control_phase = 1;
    io_cfg.scl_speed_hz = 100000;
    err = esp_lcd_new_panel_io_i2c(i2c_bus_, &io_cfg, &tp_io_);
    if (err == ESP_OK) {
      err = esp_lcd_touch_new_i2c_st7123(tp_io_, &tp_cfg, &tp_);
    }
  }

  qual_.touch_err = err;
  qual_.touch_ready = (err == ESP_OK && tp_ != nullptr);
  return qual_.touch_ready;
}

bool BoardTab5::begin(i2c_master_bus_handle_t i2c_bus) {
  i2c_bus_ = i2c_bus;
  qual_ = BoardQualification{};
  if (!i2c_bus_) return false;

  (void)initBacklight();
  if (!pulseLcdResetAndInitIo2()) return false;
  probePeripherals();

  const Tab5PanelRev rev = detectPanelRevision();
  if (initDisplayPanel(rev)) {
    (void)setBacklightPercent(75);
  }
  (void)initTouchController(rev);
  return qual_.display_ready;
}

UiTouchPoint BoardTab5::pollTouch(bool flip_180) {
  UiTouchPoint out;
  if (!qual_.touch_ready || !tp_) return out;

  if (esp_lcd_touch_read_data(tp_) != ESP_OK) {
    return out;
  }
  esp_lcd_touch_point_data_t pt = {};
  uint8_t cnt = 0;
  if (esp_lcd_touch_get_data(tp_, &pt, &cnt, 1) == ESP_OK && cnt > 0) {
    return mapPortraitTouchToLandscape(true, int(pt.x), int(pt.y), flip_180);
  }
  return out;
}

bool BoardTab5::presentLandscape640x360(const uint16_t* fb640x360, bool flip_180) {
  if (!qual_.display_ready || !dpi_fb_ || !fb640x360) return false;
  blitLandscape640x360ToPortrait720x1280(fb640x360, dpi_fb_, flip_180);
  (void)esp_cache_msync(
      dpi_fb_, kPanelFbBytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
  return true;
}

bool BoardTab5::setMicEnabled(bool enable) {
  if (!enable) {
    if (mic_dev_) {
      (void)esp_codec_dev_close(mic_dev_);
    }
    qual_.mic_open = false;
    return true;
  }

  if (qual_.mic_open && mic_dev_) return true;
  if (!i2c_bus_ || !qual_.es7210_ok) return false;

  if (!i2s_rx_) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    if (i2s_new_channel(&chan_cfg, &i2s_tx_, &i2s_rx_) != ESP_OK) return false;

    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
    std_cfg.slot_cfg =
        I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    std_cfg.gpio_cfg.mclk = kI2sMclkPin;
    std_cfg.gpio_cfg.bclk = kI2sBclkPin;
    std_cfg.gpio_cfg.ws = kI2sWsPin;
    std_cfg.gpio_cfg.dout = kI2sDoutPin;
    std_cfg.gpio_cfg.din = kI2sDinPin;

    if (i2s_channel_init_std_mode(i2s_tx_, &std_cfg) != ESP_OK ||
        i2s_channel_init_std_mode(i2s_rx_, &std_cfg) != ESP_OK) {
      return false;
    }
    (void)i2s_channel_enable(i2s_tx_);
    (void)i2s_channel_enable(i2s_rx_);

    audio_codec_i2s_cfg_t i2s_cfg = {};
    i2s_cfg.port = I2S_NUM_0;
    i2s_cfg.rx_handle = i2s_rx_;
    i2s_cfg.tx_handle = i2s_tx_;
    i2s_data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
    if (!i2s_data_if_) return false;
  }

  if (!mic_dev_) {
    audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.port = I2C_NUM_0;
    i2c_cfg.addr = ES7210_CODEC_DEFAULT_ADDR;  // 0x80 (8-bit for 7-bit 0x40)
    i2c_cfg.bus_handle = i2c_bus_;
    mic_ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!mic_ctrl_if_) return false;

    es7210_codec_cfg_t ccfg = {};
    ccfg.ctrl_if = mic_ctrl_if_;
    ccfg.mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2;
    mic_codec_if_ = es7210_codec_new(&ccfg);
    if (!mic_codec_if_) return false;

    esp_codec_dev_cfg_t dev_cfg = {};
    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = mic_codec_if_;
    dev_cfg.data_if = i2s_data_if_;
    mic_dev_ = esp_codec_dev_new(&dev_cfg);
    if (!mic_dev_) return false;
  }

  esp_codec_dev_sample_info_t fs = {};
  fs.bits_per_sample = 16;
  fs.channel = 1;
  fs.sample_rate = 16000;
  if (esp_codec_dev_open(mic_dev_, &fs) != ESP_CODEC_DEV_OK) {
    return false;
  }
  (void)esp_codec_dev_set_in_gain(mic_dev_, 30.0f);
  qual_.mic_open = true;
  return true;
}

opentoy_sound::PcmBlockSummary BoardTab5::sampleMicVolatile() {
  opentoy_sound::PcmBlockSummary out;
  if (!qual_.mic_open || !mic_dev_) return out;

  int16_t pcm[256] = {};
  if (esp_codec_dev_read(mic_dev_, pcm, sizeof(pcm)) != ESP_CODEC_DEV_OK) {
    return out;
  }
  out = opentoy_sound::analyzePcm16Mono(pcm, 256);
  // Immediately scrub volatile stack buffer.
  std::memset(pcm, 0, sizeof(pcm));
  return out;
}

void BoardTab5::end() {
  (void)setMicEnabled(false);
  if (backlight_inited_) {
    (void)setBacklightPercent(0);
  }
  if (tp_) {
    (void)esp_lcd_touch_del(tp_);
    tp_ = nullptr;
  }
  if (tp_io_) {
    (void)esp_lcd_panel_io_del(tp_io_);
    tp_io_ = nullptr;
  }
  if (panel_) {
    (void)esp_lcd_panel_del(panel_);
    panel_ = nullptr;
  }
  if (dbi_io_) {
    (void)esp_lcd_panel_io_del(dbi_io_);
    dbi_io_ = nullptr;
  }
  if (dsi_bus_) {
    (void)esp_lcd_del_dsi_bus(dsi_bus_);
    dsi_bus_ = nullptr;
  }
  dpi_fb_ = nullptr;
}

}  // namespace tab5
}  // namespace opentoy
