// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>

namespace opentoy {
namespace tab5 {

// Official M5Stack Tab5 camera sensor identity on internal I2C0 (G31/G32):
// SmartSens SC202CS (marketed as SC2356/SC202CS 2MP MIPI-CSI), 7-bit addr 0x36,
// ID registers 0x3107 (high) / 0x3108 (low) = 0xEB52.
constexpr uint8_t kCameraSccbAddr = 0x36;
constexpr uint16_t kSc202csSensorId = 0xEB52;
constexpr uint16_t kDefaultWidth = 1280;
constexpr uint16_t kDefaultHeight = 720;
constexpr uint32_t kMaxFreshAgeMs = 250;
constexpr uint32_t kDefaultCaptureTimeoutMs = 1500;
constexpr unsigned kColdStartSettleFrames = 6;

enum class PixelFormat : uint8_t {
  None = 0,
  Jpeg = 1,
  Rgb565Le = 2,
};

// Startup and capture stages for honest diagnostics (zero silent fallbacks):
//  0 = idle / uninitialized
//  1 = 24 MHz camera MCLK configured on GPIO36 (LEDC_TIMER_2)
//  2 = internal I2C0 bus initialized (SDA=GPIO31, SCL=GPIO32)
//  3 = PI4IOE5V6408-1 (0x43) CAM_RST (P6) pulsed without disturbing LCD_RST (P4)
//  4 = SCCB 0x36 sensor ID registers (0x3107/0x3108) read
//  5 = sensor ID verified against supported MIPI-CSI driver
//  6 = MIPI DPHY LDO_VO3 (2500 mV) acquired
//  7 = esp_video CSI subsystem initialized
//  8 = /dev/video0 opened and V4L2 RGB565 1280x720 format negotiated
//  9 = V4L2 250 ms DQBUF timeout verified and 2 MMAP buffers queued
// 10 = hardware JPEG encoder engine + DMA-aligned buffers allocated
// 11 = V4L2 stream active and background drain task running
struct SensorStatus {
  bool initialized = false;
  bool ready = false;
  bool streaming = false;
  bool jpeg_ready = false;
  bool ae_warmed = false;
  uint16_t sensor_id = 0;
  unsigned stage = 0;
  int error = 0;  // ESP_OK (0), esp_err_t, or negative errno
  uint16_t width = 0;
  uint16_t height = 0;
  uint32_t capture_count = 0;
  uint32_t latest_seq = 0;
  uint32_t captured_ms = 0;
  uint32_t last_encode_us = 0;
};

struct CapturedFrame {
  const uint8_t* data = nullptr;
  size_t byte_length = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  PixelFormat format = PixelFormat::None;
  uint32_t sequence = 0;
  uint32_t captured_ms = 0;
  uint16_t sensor_id = 0;
  bool live_hardware = false;  // True ONLY when produced by real Tab5 CSI hardware
  bool ae_warmed = false;
};

inline bool isFreshFrame(uint32_t seq_before_request,
                         const CapturedFrame& frame,
                         uint32_t now_ms,
                         uint32_t max_age_ms = kMaxFreshAgeMs) {
  if (!frame.data || frame.byte_length == 0 || frame.width == 0 || frame.height == 0) {
    return false;
  }
  if (frame.format == PixelFormat::None || frame.sequence == 0) {
    return false;
  }
  // Sequence must strictly advance past the state observed when the command arrived.
  if (int32_t(frame.sequence - seq_before_request) <= 0) {
    return false;
  }
  const uint32_t age = uint32_t(now_ms - frame.captured_ms);
  return age <= max_age_ms;
}

inline bool hasValidJpegMarkers(const uint8_t* bytes, size_t len) {
  if (!bytes || len < 4) return false;
  return bytes[0] == 0xFF && bytes[1] == 0xD8 &&
         bytes[len - 2] == 0xFF && bytes[len - 1] == 0xD9;
}

// Reusable adapter boundary shared between Vibe Coding Camera Eye and Copy-Face.
// Calling stopStream() or end() releases V4L2/CSI/JPEG resources cleanly.
class Tab5CameraInterface {
 public:
  virtual ~Tab5CameraInterface() = default;
  virtual bool begin() = 0;
  virtual bool startStream() = 0;
  virtual void stopStream() = 0;
  virtual void end() = 0;
  virtual SensorStatus status() const = 0;
  virtual bool captureFreshJpeg(uint8_t quality,
                                uint32_t timeout_ms,
                                CapturedFrame& out) = 0;
  virtual bool captureFreshRgb565(unsigned scale_div,
                                  uint32_t timeout_ms,
                                  CapturedFrame& out) = 0;
};

}  // namespace tab5
}  // namespace opentoy
