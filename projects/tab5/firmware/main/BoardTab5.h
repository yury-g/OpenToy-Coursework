// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "audio_codec_ctrl_if.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#include "SoundWake.h"
#include "adapter/Tab5Ui.h"

namespace opentoy {
namespace tab5 {

enum class Tab5PanelRev : uint8_t {
  Unknown = 0,
  Ili9881cGt911 = 1,
  St7123 = 2,
  St7121 = 3,
};

struct BoardQualification {
  Tab5PanelRev panel_rev = Tab5PanelRev::Unknown;
  const char* panel_name = "NONE";
  uint8_t touch_i2c_addr = 0;
  uint8_t touch_fw_ver = 0;
  bool display_ready = false;
  bool touch_ready = false;

  bool pi4ioe2_ok = false;
  bool es8388_ok = false;
  bool es7210_ok = false;
  uint16_t es7210_id = 0;
  bool bmi270_ok = false;
  uint8_t bmi270_chip_id = 0;
  bool rx8130_ok = false;

  bool mic_open = false;
  int display_err = 0;
  int touch_err = 0;
};

class BoardTab5 {
 public:
  BoardTab5() = default;
  ~BoardTab5();

  // Initializes peripherals, auto-detects display/touch revision, and starts 720x1280 MIPI-DSI.
  // Expects LDO_VO3 (2500 mV) and I2C0 (GPIO31/32) already initialized by CameraTab5.
  bool begin(i2c_master_bus_handle_t i2c_bus);
  void end();

  const BoardQualification& qualification() const { return qual_; }

  bool setBacklightPercent(int percent);

  // Polls raw capacitive touch and maps to 640x360 landscape coordinates.
  UiTouchPoint pollTouch(bool flip_180);

  // Renders logical 640x360 landscape buffer to physical 720x1280 MIPI-DSI DPI buffer + cache sync.
  bool presentLandscape640x360(const uint16_t* fb640x360, bool flip_180);

  // Opt-in volatile ES7210 microphone capture (default OFF; zero recording/persistence).
  bool setMicEnabled(bool enable);
  opentoy_sound::PcmBlockSummary sampleMicVolatile();

 private:
  bool pulseLcdResetAndInitIo2();
  void probePeripherals();
  Tab5PanelRev detectPanelRevision();
  bool initDisplayPanel(Tab5PanelRev rev);
  bool initTouchController(Tab5PanelRev rev);
  bool initBacklight();

  i2c_master_bus_handle_t i2c_bus_ = nullptr;
  BoardQualification qual_{};

  esp_lcd_dsi_bus_handle_t dsi_bus_ = nullptr;
  esp_lcd_panel_io_handle_t dbi_io_ = nullptr;
  esp_lcd_panel_handle_t panel_ = nullptr;
  uint16_t* dpi_fb_ = nullptr;

  esp_lcd_panel_io_handle_t tp_io_ = nullptr;
  esp_lcd_touch_handle_t tp_ = nullptr;

  i2s_chan_handle_t i2s_tx_ = nullptr;
  i2s_chan_handle_t i2s_rx_ = nullptr;
  const audio_codec_data_if_t* i2s_data_if_ = nullptr;
  const audio_codec_ctrl_if_t* mic_ctrl_if_ = nullptr;
  const audio_codec_if_t* mic_codec_if_ = nullptr;
  esp_codec_dev_handle_t mic_dev_ = nullptr;
  bool backlight_inited_ = false;
};

}  // namespace tab5
}  // namespace opentoy
