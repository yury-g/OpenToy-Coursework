// SPDX-License-Identifier: MIT
#include "CameraTab5.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "esp_video_ioctl.h"
#include "linux/videodev2.h"

namespace opentoy {
namespace tab5 {

namespace {

constexpr gpio_num_t kI2c0SdaPin = GPIO_NUM_31;
constexpr gpio_num_t kI2c0SclPin = GPIO_NUM_32;
constexpr gpio_num_t kCamMclkPin = GPIO_NUM_36;

constexpr uint8_t kPi4ioe1Addr = 0x43;
constexpr uint8_t kIna226Addr = 0x41;

constexpr uint8_t kPi4ioRegChipReset = 0x01;
constexpr uint8_t kPi4ioRegIoDir = 0x03;
constexpr uint8_t kPi4ioRegOutSet = 0x05;
constexpr uint8_t kPi4ioRegOutHIm = 0x07;
constexpr uint8_t kPi4ioRegPullEn = 0x0B;
constexpr uint8_t kPi4ioRegPullSel = 0x0D;

constexpr uint8_t kIna226RegConfig = 0x00;
constexpr uint8_t kIna226RegShuntVoltage = 0x01;
constexpr uint8_t kIna226RegBusVoltage = 0x02;
constexpr uint8_t kIna226RegCurrent = 0x04;
constexpr uint8_t kIna226RegCalibration = 0x05;
constexpr uint8_t kIna226RegDieId = 0xFF;

constexpr uint32_t kFrameBytes = uint32_t(kDefaultWidth) * uint32_t(kDefaultHeight) * 2u;
constexpr size_t kJpegOutRequestBytes = 512u * 1024u;
constexpr uint32_t kDqbufTimeoutUs = 250000u;

uint32_t nowMs() {
  return uint32_t(esp_timer_get_time() / 1000);
}

int currentErrno() {
  return errno ? -errno : ESP_FAIL;
}

esp_err_t writeReg8(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val) {
  uint8_t buf[2] = {reg, val};
  return i2c_master_transmit(dev, buf, sizeof(buf), 50);
}

}  // namespace

CameraTab5::~CameraTab5() {
  end();
}

void CameraTab5::setError(unsigned stage, int err) {
  if (state_mutex_ && xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    status_.stage = stage;
    status_.error = err;
    if (err != 0) {
      status_.ready = false;
    }
    xSemaphoreGive(state_mutex_);
  } else {
    status_.stage = stage;
    status_.error = err;
    if (err != 0) {
      status_.ready = false;
    }
  }
}

bool CameraTab5::initMclkAndExpander() {
  // Stage 1: 24 MHz camera MCLK on GPIO36 using LEDC_TIMER_2 (clear of display backlight TIMER_0).
  status_.stage = 1;
  ledc_timer_config_t timer_conf = {};
  timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
  timer_conf.duty_resolution = LEDC_TIMER_1_BIT;
  timer_conf.timer_num = LEDC_TIMER_2;
  timer_conf.freq_hz = 24000000;
  timer_conf.clk_cfg = LEDC_AUTO_CLK;
  timer_conf.deconfigure = false;
  esp_err_t err = ledc_timer_config(&timer_conf);
  if (err != ESP_OK) {
    setError(1, err);
    return false;
  }

  ledc_channel_config_t ch_conf = {};
  ch_conf.gpio_num = kCamMclkPin;
  ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
  ch_conf.channel = LEDC_CHANNEL_2;
  ch_conf.intr_type = LEDC_INTR_DISABLE;
  ch_conf.timer_sel = LEDC_TIMER_2;
  ch_conf.duty = 1;
  ch_conf.hpoint = 0;
  ch_conf.sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE;
  err = ledc_channel_config(&ch_conf);
  if (err != ESP_OK) {
    setError(1, err);
    return false;
  }
  mclk_inited_ = true;
  vTaskDelay(pdMS_TO_TICKS(20));

  // Stage 2: Internal I2C0 bus (SDA=GPIO31, SCL=GPIO32).
  status_.stage = 2;
  i2c_master_bus_config_t bus_cfg = {};
  bus_cfg.i2c_port = I2C_NUM_0;
  bus_cfg.sda_io_num = kI2c0SdaPin;
  bus_cfg.scl_io_num = kI2c0SclPin;
  bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_cfg.glitch_ignore_cnt = 7;
  bus_cfg.flags.enable_internal_pullup = true;
  err = i2c_new_master_bus(&bus_cfg, &i2c_bus_);
  if (err != ESP_OK) {
    setError(2, err);
    return false;
  }

  // Stage 3: Pulse PI4IOE5V6408-1 (0x43) P6 (CAM_RST) without driving P4 (LCD_RST) push-pull.
  // Keep P4 as input with pull-up (IO_DIR bit4 = 0, OUT_SET bit4 = 0, PULL_SEL/PULL_EN bit4 = 1).
  status_.stage = 3;
  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = kPi4ioe1Addr;
  dev_cfg.scl_speed_hz = 400000;
  i2c_master_dev_handle_t io1 = nullptr;
  err = i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &io1);
  if (err != ESP_OK) {
    setError(3, err);
    return false;
  }

  uint8_t dummy = 0;
  const uint8_t reg_rst = kPi4ioRegChipReset;
  (void)writeReg8(io1, kPi4ioRegChipReset, 0xFF);
  (void)i2c_master_transmit_receive(io1, &reg_rst, 1, &dummy, 1, 50);

  err = writeReg8(io1, kPi4ioRegPullSel, 0b01111111);
  if (err == ESP_OK) err = writeReg8(io1, kPi4ioRegPullEn, 0b01111111);
  if (err == ESP_OK) err = writeReg8(io1, kPi4ioRegOutHIm, 0b00000000);
  // P6=0 (CAM_RST low), P5=1 (TOUCH_EN), P4=0 (LCD_RST safe latch), P2=1, P1=0 (SPEAKER_EN off).
  if (err == ESP_OK) err = writeReg8(io1, kPi4ioRegOutSet, 0b00100100);
  // P4 stays input (bit4=0); P6 output (bit6=1).
  if (err == ESP_OK) err = writeReg8(io1, kPi4ioRegIoDir, 0b01101111);
  if (err == ESP_OK) {
    vTaskDelay(pdMS_TO_TICKS(20));
    err = writeReg8(io1, kPi4ioRegOutSet, 0b01100100);  // Release CAM_RST high, keep speaker off
  }
  (void)i2c_master_bus_rm_device(io1);
  if (err != ESP_OK) {
    setError(3, err);
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(100));
  return true;
}

bool CameraTab5::probeSensorId(uint16_t& out_id) {
  // Stage 4: Probe SCCB 0x36 registers 0x3107 / 0x3108.
  status_.stage = 4;
  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = kCameraSccbAddr;
  dev_cfg.scl_speed_hz = 100000;
  i2c_master_dev_handle_t sccb = nullptr;
  esp_err_t err = i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &sccb);
  if (err != ESP_OK) {
    setError(4, err);
    return false;
  }

  const uint8_t reg_hi[2] = {0x31, 0x07};
  const uint8_t reg_lo[2] = {0x31, 0x08};
  uint8_t hi = 0;
  uint8_t lo = 0;
  err = i2c_master_transmit_receive(sccb, reg_hi, 2, &hi, 1, 50);
  if (err == ESP_OK) {
    err = i2c_master_transmit_receive(sccb, reg_lo, 2, &lo, 1, 50);
  }
  esp_err_t rm_err = i2c_master_bus_rm_device(sccb);
  if (err != ESP_OK) {
    setError(4, err);
    return false;
  }
  if (rm_err != ESP_OK) {
    setError(4, rm_err);
    return false;
  }

  out_id = (uint16_t(hi) << 8) | lo;
  status_.stage = 5;
  status_.sensor_id = out_id;
  if (out_id != kSc202csSensorId) {
    setError(5, ESP_ERR_NOT_SUPPORTED);
    return false;
  }
  return true;
}

bool CameraTab5::setupVideoDevice() {
  // Stage 6: Acquire MIPI DPHY LDO_VO3 (2500 mV).
  status_.stage = 6;
  esp_ldo_channel_config_t ldo_cfg = {};
  ldo_cfg.chan_id = 3;
  ldo_cfg.voltage_mv = 2500;
  esp_err_t err = esp_ldo_acquire_channel(&ldo_cfg, &ldo_chan_);
  if (err != ESP_OK) {
    setError(6, err);
    return false;
  }

  // Stage 7: Initialize esp_video CSI subsystem using shared I2C0 handle.
  status_.stage = 7;
  esp_video_init_csi_config_t csi_cfg = {};
  csi_cfg.sccb_config.init_sccb = false;
  csi_cfg.sccb_config.i2c_handle = i2c_bus_;
  csi_cfg.sccb_config.freq = 100000;
  csi_cfg.reset_pin = GPIO_NUM_NC;
  csi_cfg.pwdn_pin = GPIO_NUM_NC;
  csi_cfg.dont_init_ldo = true;

  esp_video_init_config_t init_cfg = {};
  init_cfg.csi = &csi_cfg;
  err = esp_video_init(&init_cfg);
  if (err != ESP_OK) {
    setError(7, err);
    return false;
  }
  video_inited_ = true;

  // Stage 81: Open /dev/video0.
  status_.stage = 81;
  video_fd_ = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
  if (video_fd_ < 0) {
    setError(81, currentErrno());
    return false;
  }

  // Stage 82: Query V4L2 capabilities.
  status_.stage = 82;
  v4l2_capability cap = {};
  if (ioctl(video_fd_, VIDIOC_QUERYCAP, &cap) < 0) {
    setError(82, currentErrno());
    return false;
  }

  // Stage 83: Query default format (preserve type across G_FMT because esp_video may clear fmt.type).
  status_.stage = 83;
  v4l2_format fmt = {};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(video_fd_, VIDIOC_G_FMT, &fmt) < 0) {
    setError(83, currentErrno());
    return false;
  }
  frame_width_ = uint16_t(fmt.fmt.pix.width ? fmt.fmt.pix.width : kDefaultWidth);
  frame_height_ = uint16_t(fmt.fmt.pix.height ? fmt.fmt.pix.height : kDefaultHeight);
  frame_bytes_ = uint32_t(frame_width_) * uint32_t(frame_height_) * 2u;
  status_.width = frame_width_;
  status_.height = frame_height_;

  // Stage 84: Only request RGB565 if default format is not already RGB565.
  status_.stage = 84;
  if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) {
    v4l2_format sfmt = {};
    sfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sfmt.fmt.pix.width = frame_width_;
    sfmt.fmt.pix.height = frame_height_;
    sfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
    if (ioctl(video_fd_, VIDIOC_S_FMT, &sfmt) < 0) {
      setError(84, currentErrno());
      return false;
    }
    v4l2_format gfmt = {};
    gfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd_, VIDIOC_G_FMT, &gfmt) < 0) {
      setError(84, currentErrno());
      return false;
    }
    fmt = gfmt;
    frame_width_ = uint16_t(fmt.fmt.pix.width ? fmt.fmt.pix.width : frame_width_);
    frame_height_ = uint16_t(fmt.fmt.pix.height ? fmt.fmt.pix.height : frame_height_);
    frame_bytes_ = uint32_t(frame_width_) * uint32_t(frame_height_) * 2u;
    status_.width = frame_width_;
    status_.height = frame_height_;
  }

  // Stage 85: Validate RGB565 format and stride.
  status_.stage = 85;
  if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565 || frame_width_ == 0 || frame_height_ == 0) {
    setError(85, ESP_ERR_NOT_SUPPORTED);
    return false;
  }
  stride_ = fmt.fmt.pix.bytesperline ? fmt.fmt.pix.bytesperline : (uint32_t(frame_width_) * 2u);
  if (stride_ < uint32_t(frame_width_) * 2u || stride_ > 8192u) {
    setError(85, ESP_ERR_INVALID_SIZE);
    return false;
  }

  // Stage 9: Verify 250 ms DQBUF timeout and queue MMAP buffers.
  status_.stage = 9;
  timeval timeout = {0, kDqbufTimeoutUs};
  timeval actual = {};
  if (ioctl(video_fd_, VIDIOC_S_DQBUF_TIMEOUT, &timeout) < 0 ||
      ioctl(video_fd_, VIDIOC_G_DQBUF_TIMEOUT, &actual) < 0) {
    setError(9, currentErrno());
    return false;
  }
  if (actual.tv_sec != 0 || actual.tv_usec <= 0 || actual.tv_usec > long(kDqbufTimeoutUs)) {
    setError(9, ESP_ERR_INVALID_STATE);
    return false;
  }

  v4l2_requestbuffers req = {};
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  req.count = kMaxVideoBuffers;
  if (ioctl(video_fd_, VIDIOC_REQBUFS, &req) < 0 || req.count < 2 || req.count > kMaxVideoBuffers) {
    setError(9, currentErrno());
    return false;
  }
  mmap_count_ = req.count;

  for (uint32_t i = 0; i < mmap_count_; ++i) {
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if (ioctl(video_fd_, VIDIOC_QUERYBUF, &buf) < 0) {
      setError(9, currentErrno());
      return false;
    }
    if (buf.length < size_t(stride_) * frame_height_) {
      setError(9, ESP_ERR_INVALID_SIZE);
      return false;
    }
    void* mapped = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd_, buf.m.offset);
    if (!mapped || mapped == MAP_FAILED) {
      setError(9, currentErrno());
      return false;
    }
    mmap_bufs_[i] = mapped;
    mmap_lens_[i] = buf.length;
    if (ioctl(video_fd_, VIDIOC_QBUF, &buf) < 0) {
      setError(9, currentErrno());
      return false;
    }
  }
  return true;
}

bool CameraTab5::setupJpegEncoder() {
  // Stage 10: Hardware JPEG encoder + PSRAM buffers.
  status_.stage = 10;
  jpeg_encode_engine_cfg_t eng_cfg = {};
  eng_cfg.intr_priority = 0;
  eng_cfg.timeout_ms = 200;
  esp_err_t err = jpeg_new_encoder_engine(&eng_cfg, &jpeg_enc_);
  if (err != ESP_OK) {
    setError(10, err);
    return false;
  }

  latest_rgb_ = static_cast<uint8_t*>(
      heap_caps_calloc(1, frame_bytes_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!latest_rgb_) {
    setError(10, ESP_ERR_NO_MEM);
    return false;
  }

  jpeg_encode_memory_alloc_cfg_t in_cfg = {};
  in_cfg.buffer_direction = JPEG_ENC_ALLOC_INPUT_BUFFER;
  jpeg_in_buf_ = static_cast<uint8_t*>(jpeg_alloc_encoder_mem(frame_bytes_, &in_cfg, &jpeg_in_cap_));

  jpeg_encode_memory_alloc_cfg_t out_cfg = {};
  out_cfg.buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER;
  jpeg_out_buf_ = static_cast<uint8_t*>(
      jpeg_alloc_encoder_mem(kJpegOutRequestBytes, &out_cfg, &jpeg_out_cap_));

  if (!jpeg_in_buf_ || !jpeg_out_buf_ || jpeg_in_cap_ < frame_bytes_ ||
      jpeg_out_cap_ < kJpegOutRequestBytes) {
    setError(10, ESP_ERR_NO_MEM);
    return false;
  }

  status_.jpeg_ready = true;
  return true;
}

bool CameraTab5::begin() {
  if (status_.initialized) {
    return status_.ready;
  }

  state_mutex_ = xSemaphoreCreateMutex();
  frame_mutex_ = xSemaphoreCreateMutex();
  new_frame_sem_ = xSemaphoreCreateBinary();
  if (!state_mutex_ || !frame_mutex_ || !new_frame_sem_) {
    setError(0, ESP_ERR_NO_MEM);
    return false;
  }

  status_ = SensorStatus{};
  frame_width_ = kDefaultWidth;
  frame_height_ = kDefaultHeight;
  frame_bytes_ = kFrameBytes;
  mmap_count_ = 0;

  if (!initMclkAndExpander()) {
    return false;
  }

  uint16_t sensor_id = 0;
  if (!probeSensorId(sensor_id)) {
    return false;
  }

  if (!setupVideoDevice()) {
    return false;
  }

  if (!setupJpegEncoder()) {
    return false;
  }

  configureIna226IfPresent();

  status_.initialized = true;
  status_.ready = true;
  status_.width = frame_width_;
  status_.height = frame_height_;
  status_.error = ESP_OK;
  return true;
}

bool CameraTab5::startStream() {
  if (!status_.initialized || !status_.ready) {
    return false;
  }
  if (stream_on_) {
    return true;
  }

  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(video_fd_, VIDIOC_STREAMON, &type) < 0) {
    setError(11, currentErrno());
    return false;
  }
  stream_on_ = true;
  stop_requested_.store(false);

  if (xTaskCreatePinnedToCore(drainTaskEntry, "tab5_cam_drain", 6144, this, 4, &drain_task_, 1) !=
      pdPASS) {
    setError(11, ESP_ERR_NO_MEM);
    stopStream();
    return false;
  }

  if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    status_.stage = 11;
    status_.streaming = true;
    status_.error = ESP_OK;
    xSemaphoreGive(state_mutex_);
  }
  return true;
}

void CameraTab5::stopStream() {
  stop_requested_.store(true);
  if (drain_task_) {
    for (int i = 0; i < 60 && drain_task_ != nullptr; ++i) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  if (stream_on_ && video_fd_ >= 0) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    (void)ioctl(video_fd_, VIDIOC_STREAMOFF, &type);
  }
  stream_on_ = false;
  if (state_mutex_ && xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    status_.streaming = false;
    xSemaphoreGive(state_mutex_);
  }
}

void CameraTab5::end() {
  stopStream();

  for (unsigned i = 0; i < kMaxVideoBuffers; ++i) {
    if (mmap_bufs_[i]) {
      munmap(mmap_bufs_[i], mmap_lens_[i]);
      mmap_bufs_[i] = nullptr;
      mmap_lens_[i] = 0;
    }
  }
  mmap_count_ = 0;
  if (video_fd_ >= 0) {
    ::close(video_fd_);
    video_fd_ = -1;
  }
  if (video_inited_) {
    (void)esp_video_deinit();
    video_inited_ = false;
  }
  if (ldo_chan_) {
    (void)esp_ldo_release_channel(ldo_chan_);
    ldo_chan_ = nullptr;
  }
  if (jpeg_enc_) {
    (void)jpeg_del_encoder_engine(jpeg_enc_);
    jpeg_enc_ = nullptr;
  }
  if (latest_rgb_) {
    heap_caps_free(latest_rgb_);
    latest_rgb_ = nullptr;
  }
  if (jpeg_in_buf_) {
    heap_caps_free(jpeg_in_buf_);
    jpeg_in_buf_ = nullptr;
  }
  if (jpeg_out_buf_) {
    heap_caps_free(jpeg_out_buf_);
    jpeg_out_buf_ = nullptr;
  }
  if (rgb_out_buf_) {
    heap_caps_free(rgb_out_buf_);
    rgb_out_buf_ = nullptr;
  }
  if (i2c_bus_) {
    (void)i2c_del_master_bus(i2c_bus_);
    i2c_bus_ = nullptr;
  }
  if (mclk_inited_) {
    (void)ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_2);
    mclk_inited_ = false;
  }
  if (new_frame_sem_) {
    vSemaphoreDelete(new_frame_sem_);
    new_frame_sem_ = nullptr;
  }
  if (frame_mutex_) {
    vSemaphoreDelete(frame_mutex_);
    frame_mutex_ = nullptr;
  }
  if (state_mutex_) {
    vSemaphoreDelete(state_mutex_);
    state_mutex_ = nullptr;
  }
  status_ = SensorStatus{};
}

void CameraTab5::drainTaskEntry(void* arg) {
  static_cast<CameraTab5*>(arg)->drainLoop();
}

void CameraTab5::drainLoop() {
  unsigned consecutive_errors = 0;
  while (!stop_requested_.load()) {
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(video_fd_, VIDIOC_DQBUF, &buf) < 0) {
      const int err = currentErrno();
      if (++consecutive_errors >= 8) {
        setError(11, err);
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    consecutive_errors = 0;

    const uint32_t cap_ms = nowMs();
    if (buf.index < mmap_count_ && !(buf.flags & V4L2_BUF_FLAG_ERROR) &&
        buf.bytesused >= size_t(stride_) * frame_height_ &&
        buf.bytesused <= mmap_lens_[buf.index]) {
      const uint8_t* src = static_cast<const uint8_t*>(mmap_bufs_[buf.index]);
      if (xSemaphoreTake(frame_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        if (stride_ == uint32_t(frame_width_) * 2u) {
          std::memcpy(latest_rgb_, src, frame_bytes_);
        } else {
          for (uint16_t y = 0; y < frame_height_; ++y) {
            std::memcpy(latest_rgb_ + size_t(y) * frame_width_ * 2u,
                        src + size_t(y) * stride_,
                        size_t(frame_width_) * 2u);
          }
        }
        xSemaphoreGive(frame_mutex_);

        if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
          ++status_.capture_count;
          ++status_.latest_seq;
          status_.captured_ms = cap_ms;
          status_.ae_warmed = (status_.capture_count >= kColdStartSettleFrames);
          status_.error = ESP_OK;
          xSemaphoreGive(state_mutex_);
        }
        xSemaphoreGive(new_frame_sem_);
      }
    }

    if (ioctl(video_fd_, VIDIOC_QBUF, &buf) < 0) {
      setError(11, currentErrno());
      break;
    }
  }

  drain_task_ = nullptr;
  vTaskDelete(nullptr);
}

SensorStatus CameraTab5::status() const {
  SensorStatus copy = status_;
  if (state_mutex_ && xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    copy = status_;
    xSemaphoreGive(state_mutex_);
  }
  return copy;
}

bool CameraTab5::captureFreshJpeg(uint8_t quality,
                                  uint32_t timeout_ms,
                                  CapturedFrame& out) {
  out = CapturedFrame{};
  if (!status_.ready || !stream_on_ || !jpeg_enc_) {
    return false;
  }
  if (quality < 10) quality = 10;
  if (quality > 95) quality = 95;
  if (timeout_ms == 0) timeout_ms = kDefaultCaptureTimeoutMs;

  // Drain any stale semaphore signal before observing sequence baseline.
  (void)xSemaphoreTake(new_frame_sem_, 0);
  uint32_t seq_before = 0;
  if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    seq_before = status_.latest_seq;
    xSemaphoreGive(state_mutex_);
  }

  const uint32_t deadline_ms = nowMs() + timeout_ms;
  while (true) {
    const uint32_t cur_ms = nowMs();
    if (int32_t(deadline_ms - cur_ms) <= 0) {
      return false;
    }
    const uint32_t wait_ms = deadline_ms - cur_ms;
    if (xSemaphoreTake(new_frame_sem_, pdMS_TO_TICKS(wait_ms)) != pdTRUE) {
      return false;
    }

    SensorStatus st = status();
    if (!st.ready || !st.streaming || int32_t(st.latest_seq - seq_before) <= 0) {
      continue;
    }
    if (uint32_t(nowMs() - st.captured_ms) > kMaxFreshAgeMs) {
      continue;
    }

    uint32_t copied_seq = 0;
    uint32_t copied_ms = 0;
    bool copied_ae = false;
    uint16_t sensor_id = 0;

    if (xSemaphoreTake(frame_mutex_, pdMS_TO_TICKS(50)) != pdTRUE) {
      return false;
    }
    std::memcpy(jpeg_in_buf_, latest_rgb_, frame_bytes_);
    if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
      copied_seq = status_.latest_seq;
      copied_ms = status_.captured_ms;
      copied_ae = status_.ae_warmed;
      sensor_id = status_.sensor_id;
      xSemaphoreGive(state_mutex_);
    }
    xSemaphoreGive(frame_mutex_);

    if (int32_t(copied_seq - seq_before) <= 0) {
      continue;
    }

    jpeg_encode_cfg_t enc_cfg = {};
    enc_cfg.width = frame_width_;
    enc_cfg.height = frame_height_;
    enc_cfg.src_type = JPEG_ENCODE_IN_FORMAT_RGB565;
    enc_cfg.sub_sample = JPEG_DOWN_SAMPLING_YUV422;
    enc_cfg.image_quality = quality;

    uint32_t out_len = 0;
    const uint64_t t0_us = uint64_t(esp_timer_get_time());
    const esp_err_t err = jpeg_encoder_process(jpeg_enc_,
                                               &enc_cfg,
                                               jpeg_in_buf_,
                                               frame_bytes_,
                                               jpeg_out_buf_,
                                               uint32_t(jpeg_out_cap_),
                                               &out_len);
    const uint32_t enc_us = uint32_t(uint64_t(esp_timer_get_time()) - t0_us);

    if (err != ESP_OK || !hasValidJpegMarkers(jpeg_out_buf_, out_len)) {
      setError(11, err != ESP_OK ? err : ESP_ERR_INVALID_RESPONSE);
      return false;
    }

    if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
      status_.last_encode_us = enc_us;
      xSemaphoreGive(state_mutex_);
    }

    out.data = jpeg_out_buf_;
    out.byte_length = out_len;
    out.width = frame_width_;
    out.height = frame_height_;
    out.format = PixelFormat::Jpeg;
    out.sequence = copied_seq;
    out.captured_ms = copied_ms;
    out.sensor_id = sensor_id;
    out.live_hardware = true;
    out.ae_warmed = copied_ae;
    if (isFreshFrame(seq_before, out, nowMs())) {
      return true;
    }
  }
}

bool CameraTab5::captureFreshRgb565(unsigned scale_div,
                                    uint32_t timeout_ms,
                                    CapturedFrame& out) {
  out = CapturedFrame{};
  if (!status_.ready || !stream_on_) {
    return false;
  }
  if (scale_div < 1) scale_div = 1;
  if (scale_div > 8) scale_div = 8;
  const uint16_t dst_w = uint16_t(frame_width_ / scale_div);
  const uint16_t dst_h = uint16_t(frame_height_ / scale_div);
  const size_t needed = size_t(dst_w) * size_t(dst_h) * 2u;

  if (!rgb_out_buf_ || rgb_out_cap_ < needed) {
    if (rgb_out_buf_) heap_caps_free(rgb_out_buf_);
    rgb_out_buf_ = static_cast<uint8_t*>(
        heap_caps_malloc(needed, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!rgb_out_buf_) {
      rgb_out_cap_ = 0;
      return false;
    }
    rgb_out_cap_ = needed;
  }

  if (timeout_ms == 0) timeout_ms = kDefaultCaptureTimeoutMs;
  (void)xSemaphoreTake(new_frame_sem_, 0);
  uint32_t seq_before = 0;
  if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    seq_before = status_.latest_seq;
    xSemaphoreGive(state_mutex_);
  }

  const uint32_t deadline_ms = nowMs() + timeout_ms;
  while (true) {
    const uint32_t cur_ms = nowMs();
    if (int32_t(deadline_ms - cur_ms) <= 0) {
      return false;
    }
    if (xSemaphoreTake(new_frame_sem_, pdMS_TO_TICKS(deadline_ms - cur_ms)) != pdTRUE) {
      return false;
    }
    SensorStatus st = status();
    if (!st.ready || !st.streaming || int32_t(st.latest_seq - seq_before) <= 0) {
      continue;
    }
    if (xSemaphoreTake(frame_mutex_, pdMS_TO_TICKS(50)) != pdTRUE) {
      return false;
    }

    const uint16_t* src = reinterpret_cast<const uint16_t*>(latest_rgb_);
    uint16_t* dst = reinterpret_cast<uint16_t*>(rgb_out_buf_);
    if (scale_div == 1) {
      std::memcpy(dst, src, needed);
    } else {
      for (uint16_t y = 0; y < dst_h; ++y) {
        const uint16_t* row = src + size_t(y * scale_div) * frame_width_;
        uint16_t* drow = dst + size_t(y) * dst_w;
        for (uint16_t x = 0; x < dst_w; ++x) {
          drow[x] = row[x * scale_div];
        }
      }
    }

    uint32_t copied_seq = 0;
    uint32_t copied_ms = 0;
    bool copied_ae = false;
    uint16_t sensor_id = 0;
    if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
      copied_seq = status_.latest_seq;
      copied_ms = status_.captured_ms;
      copied_ae = status_.ae_warmed;
      sensor_id = status_.sensor_id;
      xSemaphoreGive(state_mutex_);
    }
    xSemaphoreGive(frame_mutex_);

    out.data = rgb_out_buf_;
    out.byte_length = needed;
    out.width = dst_w;
    out.height = dst_h;
    out.format = PixelFormat::Rgb565Le;
    out.sequence = copied_seq;
    out.captured_ms = copied_ms;
    out.sensor_id = sensor_id;
    out.live_hardware = true;
    out.ae_warmed = copied_ae;
    if (isFreshFrame(seq_before, out, nowMs())) {
      return true;
    }
  }
}

bool CameraTab5::copyLatestDownsampledRgb565(uint16_t* dst,
                                             uint16_t dst_w,
                                             uint16_t dst_h,
                                             uint32_t& out_seq,
                                             uint32_t& out_captured_ms,
                                             bool& out_ae) {
  out_seq = 0;
  out_captured_ms = 0;
  out_ae = false;
  if (!dst || dst_w == 0 || dst_h == 0 || !status_.ready || !stream_on_ || !latest_rgb_) {
    return false;
  }
  if (xSemaphoreTake(state_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    out_seq = status_.latest_seq;
    out_captured_ms = status_.captured_ms;
    out_ae = status_.ae_warmed;
    xSemaphoreGive(state_mutex_);
  }
  if (out_seq == 0) return false;

  if (xSemaphoreTake(frame_mutex_, pdMS_TO_TICKS(15)) != pdTRUE) {
    return false;
  }
  const uint16_t* src = reinterpret_cast<const uint16_t*>(latest_rgb_);
  for (uint16_t y = 0; y < dst_h; ++y) {
    const uint32_t sy = (uint32_t(y) * frame_height_) / dst_h;
    const uint16_t* srow = src + sy * frame_width_;
    uint16_t* drow = dst + uint32_t(y) * dst_w;
    for (uint16_t x = 0; x < dst_w; ++x) {
      const uint32_t sx = (uint32_t(x) * frame_width_) / dst_w;
      drow[x] = srow[sx];
    }
  }
  xSemaphoreGive(frame_mutex_);
  return true;
}

void CameraTab5::configureIna226IfPresent() {
  if (!i2c_bus_) return;
  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = kIna226Addr;
  dev_cfg.scl_speed_hz = 400000;
  i2c_master_dev_handle_t ina = nullptr;
  if (i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &ina) != ESP_OK) return;

  uint8_t reg_die = kIna226RegDieId;
  uint8_t die_raw[2] = {0, 0};
  if (i2c_master_transmit_receive(ina, &reg_die, 1, die_raw, 2, 30) == ESP_OK) {
    ina_die_id_ = (uint16_t(die_raw[0]) << 8) | die_raw[1];
    // Configure AVG_16 (0b010<<9) | 1.1ms bus (0b100<<6) | 1.1ms shunt (0b100<<3) | CONT_SHUNT_BUS (0b111) = 0x4527
    const uint8_t cfg_cmd[3] = {kIna226RegConfig, 0x45, 0x27};
    (void)i2c_master_transmit(ina, cfg_cmd, sizeof(cfg_cmd), 30);
    // 5 mOhm shunt, current_lsb = 0.25 mA -> cal = 0.00512 / (0.00025 * 0.005) = 4096 (0x1000)
    const uint8_t cal_cmd[3] = {kIna226RegCalibration, 0x10, 0x00};
    (void)i2c_master_transmit(ina, cal_cmd, sizeof(cal_cmd), 30);
    ina_configured_ = true;
  }
  (void)i2c_master_bus_rm_device(ina);
}

PowerDiagnostics CameraTab5::readPowerDiagnostics() {
  PowerDiagnostics diag;
  if (!i2c_bus_) return diag;
  if (!ina_configured_) configureIna226IfPresent();

  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = kIna226Addr;
  dev_cfg.scl_speed_hz = 400000;
  i2c_master_dev_handle_t ina = nullptr;
  if (i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &ina) != ESP_OK) {
    return diag;
  }

  auto read16 = [&](uint8_t reg, uint16_t& out_v) -> bool {
    uint8_t raw[2] = {0, 0};
    if (i2c_master_transmit_receive(ina, &reg, 1, raw, 2, 30) != ESP_OK) return false;
    out_v = (uint16_t(raw[0]) << 8) | raw[1];
    return true;
  };

  uint16_t bus_raw = 0, shunt_raw = 0, cur_raw = 0;
  if (read16(kIna226RegBusVoltage, bus_raw)) {
    diag.ina226_present = true;
    diag.die_id = ina_die_id_;
    diag.vbus_mv = int32_t((uint32_t(bus_raw) * 125u) / 100u);
    if (read16(kIna226RegShuntVoltage, shunt_raw)) {
      diag.shunt_uv = (int32_t(int16_t(shunt_raw)) * 5) / 2;  // 2.5 uV LSB
    }
    if (read16(kIna226RegCurrent, cur_raw)) {
      diag.current_ma = int32_t(int16_t(cur_raw)) / 4;  // 0.25 mA LSB
    }
  }
  (void)i2c_master_bus_rm_device(ina);
  return diag;
}

BatterySample CameraTab5::readBatterySample() {
  BatterySample sample;
  const PowerDiagnostics diag = readPowerDiagnostics();
  if (!diag.ina226_present) return sample;

  sample.kind = BatteryReadingKind::MeasuredMillivolts2S;
  sample.value = diag.vbus_mv;
  sample.sampled_ms = nowMs();
  sample.valid = (diag.vbus_mv >= kMinValid2SMillivolts && diag.vbus_mv <= kMaxValid2SMillivolts);
  return sample;
}

}  // namespace tab5
}  // namespace opentoy
