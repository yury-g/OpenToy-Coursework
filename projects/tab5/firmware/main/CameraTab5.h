// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstdint>

#include "driver/i2c_master.h"
#include "driver/jpeg_encode.h"
#include "esp_ldo_regulator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "adapter/Tab5Battery.h"
#include "adapter/Tab5CameraAdapter.h"

namespace opentoy {
namespace tab5 {

struct PowerDiagnostics {
  bool ina226_present = false;
  uint16_t die_id = 0;
  int32_t vbus_mv = 0;
  int32_t shunt_uv = 0;
  int32_t current_ma = 0;
};

class CameraTab5 final : public Tab5CameraInterface {
 public:
  CameraTab5() = default;
  ~CameraTab5() override;

  bool begin() override;
  bool startStream() override;
  void stopStream() override;
  void end() override;

  SensorStatus status() const override;
  bool captureFreshJpeg(uint8_t quality,
                        uint32_t timeout_ms,
                        CapturedFrame& out) override;
  bool captureFreshRgb565(unsigned scale_div,
                          uint32_t timeout_ms,
                          CapturedFrame& out) override;

  // Non-blocking downsample copy from latest_rgb_ for background face detection & UI preview.
  bool copyLatestDownsampledRgb565(uint16_t* dst,
                                   uint16_t dst_w,
                                   uint16_t dst_h,
                                   uint32_t& out_seq,
                                   uint32_t& out_captured_ms,
                                   bool& out_ae);

  BatterySample readBatterySample();
  PowerDiagnostics readPowerDiagnostics();
  i2c_master_bus_handle_t i2cBus() const { return i2c_bus_; }

 private:
  static void drainTaskEntry(void* arg);
  void drainLoop();
  bool initMclkAndExpander();
  bool probeSensorId(uint16_t& out_id);
  bool setupVideoDevice();
  bool setupJpegEncoder();
  void configureIna226IfPresent();
  void setError(unsigned stage, int err);

  mutable SemaphoreHandle_t state_mutex_ = nullptr;
  SemaphoreHandle_t frame_mutex_ = nullptr;
  SemaphoreHandle_t new_frame_sem_ = nullptr;
  TaskHandle_t drain_task_ = nullptr;
  std::atomic<bool> stop_requested_{false};

  i2c_master_bus_handle_t i2c_bus_ = nullptr;
  esp_ldo_channel_handle_t ldo_chan_ = nullptr;
  bool video_inited_ = false;
  int video_fd_ = -1;
  bool stream_on_ = false;
  static constexpr unsigned kMaxVideoBuffers = 3;
  unsigned mmap_count_ = 0;
  void* mmap_bufs_[kMaxVideoBuffers] = {};
  size_t mmap_lens_[kMaxVideoBuffers] = {};
  uint32_t stride_ = 0;
  uint16_t frame_width_ = kDefaultWidth;
  uint16_t frame_height_ = kDefaultHeight;
  uint32_t frame_bytes_ = uint32_t(kDefaultWidth) * uint32_t(kDefaultHeight) * 2u;

  jpeg_encoder_handle_t jpeg_enc_ = nullptr;
  uint8_t* latest_rgb_ = nullptr;
  uint8_t* jpeg_in_buf_ = nullptr;
  uint8_t* jpeg_out_buf_ = nullptr;
  size_t jpeg_in_cap_ = 0;
  size_t jpeg_out_cap_ = 0;
  uint8_t* rgb_out_buf_ = nullptr;
  size_t rgb_out_cap_ = 0;

  SensorStatus status_{};
  bool mclk_inited_ = false;
  bool ina_configured_ = false;
  uint16_t ina_die_id_ = 0;
};

}  // namespace tab5
}  // namespace opentoy
