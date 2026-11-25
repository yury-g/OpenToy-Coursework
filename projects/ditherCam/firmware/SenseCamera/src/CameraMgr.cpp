// SPDX-License-Identifier: MIT
// XIAO ESP32S3 Sense Camera Driver:
// XCLK=10 (20MHz), SCCB SDA=40 SCL=39, D0..D7=[15,17,18,16,14,12,11,48],
// VSYNC=38, HREF=47, PCLK=13, PWDN/RESET=-1.
// Detects sensor PID via esp_camera_sensor_get() and chooses sensor-supported high-res still.
#include "CameraMgr.h"
#include <esp_camera.h>
#include <img_converters.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "FlipCamCrc.h"
#include "FlipCamDither.h"
#include "FlipCamJpegDecode.h"
#include "FlipCamProtocol.h"

namespace xiao_hw {

namespace {

CameraInfo s_info{};
SemaphoreHandle_t s_camMutex = nullptr;
framesize_t s_previewFs = FRAMESIZE_SVGA;
framesize_t s_stillFs = FRAMESIZE_UXGA;

// Idempotent capture cache keyed by (bootNonce, reqId, synthetic, orientation)
uint32_t s_lastBootNonce = 0;
uint32_t s_lastReqId = 0;
bool s_lastSynth = false;
uint8_t s_lastOrientation = 0;
std::vector<uint8_t> s_lastStillJpeg;
uint16_t s_lastStillW = 0;
uint16_t s_lastStillH = 0;
uint32_t s_lastStillCrc = 0;

void selectResolutionsForPid(uint16_t pid) {
  // Fast QVGA (320x240) JPEG q16 for live 1-bit stream; temporary switch to SVGA (800x600)
  // on capture for 90CW + 4:3 cropped 600x450 grayscale JPEG q72 stills.
  s_previewFs = FRAMESIZE_QVGA;
  s_stillFs = FRAMESIZE_SVGA;
  s_info.previewW = 320;
  s_info.previewH = 240;
  s_info.stillW = 600;
  s_info.stillH = 450;
  std::snprintf(s_info.previewRes, sizeof(s_info.previewRes), "320x240");
  std::snprintf(s_info.stillRes, sizeof(s_info.stillRes), "600x450");

  switch (pid) {
    case OV2640_PID: std::snprintf(s_info.sensorName, sizeof(s_info.sensorName), "OV2640"); break;
    case OV3660_PID: std::snprintf(s_info.sensorName, sizeof(s_info.sensorName), "OV3660"); break;
    case OV5640_PID: std::snprintf(s_info.sensorName, sizeof(s_info.sensorName), "OV5640"); break;
    case GC2145_PID: std::snprintf(s_info.sensorName, sizeof(s_info.sensorName), "GC2145"); break;
    default: std::snprintf(s_info.sensorName, sizeof(s_info.sensorName), "PID_0x%04X", pid); break;
  }
}

}  // namespace

bool cameraBegin() {
  if (!s_camMutex) {
    s_camMutex = xSemaphoreCreateMutex();
  }
  s_info = CameraInfo{};

  camera_config_t cfg{};
  // Isolate camera XCLK from Arduino tone()/ledcAttach (which allocates from channel 0 / timer 0)
  cfg.ledc_channel = LEDC_CHANNEL_7;
  cfg.ledc_timer = LEDC_TIMER_3;
  cfg.pin_d0 = 15;
  cfg.pin_d1 = 17;
  cfg.pin_d2 = 18;
  cfg.pin_d3 = 16;
  cfg.pin_d4 = 14;
  cfg.pin_d5 = 12;
  cfg.pin_d6 = 11;
  cfg.pin_d7 = 48;
  cfg.pin_xclk = 10;
  cfg.pin_pclk = 13;
  cfg.pin_vsync = 38;
  cfg.pin_href = 47;
  cfg.pin_sccb_sda = 40;
  cfg.pin_sccb_scl = 39;
  cfg.sccb_i2c_port = 1;  // Hardware compiled CONFIG_SCCB_HARDWARE_I2C_PORT1=y; OLED uses TwoWire(0)
  cfg.pin_pwdn = -1;
  cfg.pin_reset = -1;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.frame_size = FRAMESIZE_SVGA;  // 800x600 JPEG q16 for both preview and compact stored stills
  cfg.jpeg_quality = 16;
  cfg.fb_count = 2;
  cfg.fb_location = CAMERA_FB_IN_PSRAM;
  cfg.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&cfg);
  if (err == ESP_ERR_CAMERA_FAILED_TO_SET_FRAME_SIZE) {
    cfg.frame_size = FRAMESIZE_VGA;
    err = esp_camera_init(&cfg);
  }
  if (err != ESP_OK) {
    std::snprintf(s_info.fault, sizeof(s_info.fault), "CAM_INIT_0x%X", static_cast<unsigned>(err));
    s_info.ok = false;
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (!s) {
    std::snprintf(s_info.fault, sizeof(s_info.fault), "NO_SENSOR");
    s_info.ok = false;
    return false;
  }

  s_info.pid = s->id.PID;
  selectResolutionsForPid(s_info.pid);

  // Switch active sensor output to fast QVGA (320x240) preview resolution & user-confirmed orientation
  if (s->set_framesize(s, s_previewFs) != 0) {
    s_previewFs = FRAMESIZE_SVGA;
    s_info.previewW = 800;
    s_info.previewH = 600;
    std::snprintf(s_info.previewRes, sizeof(s_info.previewRes), "800x600");
    s->set_framesize(s, s_previewFs);
  }
  s->set_quality(s, 16);
  s->set_vflip(s, 1);
  s->set_hmirror(s, 0);

  // Flush initial pre-QVGA frame queued during esp_camera_init(SVGA)
  if (camera_fb_t* initDiscard = esp_camera_fb_get()) {
    esp_camera_fb_return(initDiscard);
  }

  s_info.ok = true;
  return true;
}

CameraInfo cameraInfo() { return s_info; }

bool cameraGrabPreview(std::vector<uint8_t>& outJpeg, uint16_t& outW, uint16_t& outH) {
  if (!s_info.ok) return false;
  if (s_camMutex && xSemaphoreTake(s_camMutex, pdMS_TO_TICKS(150)) != pdTRUE) return false;

  camera_fb_t* fb = nullptr;
  uint16_t jpgW = 0, jpgH = 0;
  for (int attempt = 0; attempt < 3; ++attempt) {
    fb = esp_camera_fb_get();
    if (!fb) break;
    const bool validJpg = (fb->format == PIXFORMAT_JPEG) &&
                          flipcam::isValidJpegBuffer(fb->buf, fb->len, flipcam::kMaxPreviewJpegBytes) &&
                          flipcam::parseJpegDimensions(fb->buf, fb->len, jpgW, jpgH);
    const bool dimMatch = (s_previewFs != FRAMESIZE_QVGA) || (jpgW <= 400 && jpgH <= 300);
    if (validJpg && dimMatch) {
      break;
    }
    esp_camera_fb_return(fb);
    fb = nullptr;
  }

  bool ok = false;
  if (fb) {
    outJpeg.assign(fb->buf, fb->buf + fb->len);
    outW = jpgW;
    outH = jpgH;
    s_info.framesOk++;
    ok = true;
    esp_camera_fb_return(fb);
  } else {
    s_info.framesErr++;
  }
  if (s_camMutex) xSemaphoreGive(s_camMutex);
  return ok;
}

bool cameraCaptureStill(uint32_t bootNonce, uint32_t reqId, bool synthetic, uint32_t synthSeq,
                        uint8_t orientation,
                        std::vector<uint8_t>& outJpeg, uint16_t& outW, uint16_t& outH,
                        uint32_t& outCrc) {
  if (s_camMutex && xSemaphoreTake(s_camMutex, pdMS_TO_TICKS(1500)) != pdTRUE) return false;

  // Idempotent retry return only when (bootNonce, reqId, synthetic, orientation) match and reqId != 0
  if (reqId != 0 && reqId == s_lastReqId && bootNonce == s_lastBootNonce &&
      synthetic == s_lastSynth && orientation == s_lastOrientation && !s_lastStillJpeg.empty()) {
    outJpeg = s_lastStillJpeg;
    outW = s_lastStillW;
    outH = s_lastStillH;
    outCrc = s_lastStillCrc;
    if (s_camMutex) xSemaphoreGive(s_camMutex);
    return true;
  }

  const bool rotate180 = (orientation == static_cast<uint8_t>(flipcam::OrientationMode::REAR));
  bool ok = false;
  auto encodeRotated600x450 = [&](const uint8_t* srcGray, int srcW, int srcH) -> bool {
    const int dstW = std::min(600, srcH);
    const int dstH = (dstW * 3) / 4;
    uint8_t* rotGray = static_cast<uint8_t*>(
        heap_caps_malloc(static_cast<size_t>(dstW) * static_cast<size_t>(dstH),
                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!rotGray) return false;
    flipcam::rotate90CwCrop43Gray(srcGray, srcW, srcH, rotGray, dstW, dstH, false, rotate180);
    uint8_t* jpgPtr = nullptr;
    size_t jpgLen = 0;
    const bool encOk =
        fmt2jpg(rotGray, static_cast<size_t>(dstW) * static_cast<size_t>(dstH),
                static_cast<uint16_t>(dstW), static_cast<uint16_t>(dstH),
                PIXFORMAT_GRAYSCALE, 72, &jpgPtr, &jpgLen) &&
        jpgPtr && flipcam::isValidJpegBuffer(jpgPtr, jpgLen, flipcam::kMaxStillJpegBytes);
    if (encOk) {
      outJpeg.assign(jpgPtr, jpgPtr + jpgLen);
      outW = static_cast<uint16_t>(dstW);
      outH = static_cast<uint16_t>(dstH);
      outCrc = flipcam::crc32Compute(outJpeg.data(), outJpeg.size());
    }
    if (jpgPtr) free(jpgPtr);
    free(rotGray);
    return encOk;
  };

  if (synthetic) {
    constexpr int sw = 800;
    constexpr int sh = 600;
    uint8_t* gray = static_cast<uint8_t*>(heap_caps_malloc(sw * sh, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (gray) {
      flipcam::generateSyntheticGray(gray, sw, sh, synthSeq);
      if (encodeRotated600x450(gray, sw, sh)) {
        s_info.synthCaptures++;
        ok = true;
      }
      free(gray);
    }
  } else if (s_info.ok) {
    // Switch temporarily to SVGA 800x600 JPEG for crisp 600x450 capture.
    // Flush >=2 queued QVGA DMA buffers unconditionally (fb->width is untrustworthy on stale ring buffers)
    // and validate BOTH parsed JPEG SOF dimensions (800x600) AND decoded grayscale dimensions.
    sensor_t* s = esp_camera_sensor_get();
    if (s && s_stillFs != s_previewFs) {
      s->set_framesize(s, s_stillFs);
      for (int flush = 0; flush < 2; ++flush) {
        if (camera_fb_t* stale = esp_camera_fb_get()) {
          esp_camera_fb_return(stale);
        }
      }
    }

    const uint16_t reqW = (s_stillFs == FRAMESIZE_SVGA) ? 800u : 640u;
    const uint16_t reqH = (s_stillFs == FRAMESIZE_SVGA) ? 600u : 480u;
    for (int attempt = 0; attempt < 4 && !ok; ++attempt) {
      camera_fb_t* fb = esp_camera_fb_get();
      if (!fb) break;
      uint16_t jpgW = 0, jpgH = 0;
      const bool validStillJpg = (fb->format == PIXFORMAT_JPEG) &&
                                 flipcam::isValidJpegBuffer(fb->buf, fb->len, flipcam::kMaxStillJpegBytes) &&
                                 flipcam::parseJpegDimensions(fb->buf, fb->len, jpgW, jpgH) &&
                                 (jpgW == reqW && jpgH == reqH);
      if (validStillJpg) {
        const size_t maxRaw = static_cast<size_t>(jpgW) * static_cast<size_t>(jpgH);
        uint8_t* srcGray = static_cast<uint8_t*>(heap_caps_malloc(maxRaw, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        int decW = 0, decH = 0;
        if (srcGray &&
            flipcam::decodeJpegToGray(fb->buf, fb->len, srcGray, maxRaw,
                                      static_cast<int>(jpgW), static_cast<int>(jpgH), decW, decH) &&
            decW == static_cast<int>(reqW) && decH == static_cast<int>(reqH)) {
          if (encodeRotated600x450(srcGray, decW, decH)) {
            s_info.prodCaptures++;
            ok = true;
          }
        }
        if (srcGray) free(srcGray);
      }
      esp_camera_fb_return(fb);
    }
    if (!ok) {
      s_info.framesErr++;
    }

    // Restore fast QVGA live preview on ALL exit paths and flush >=2 stale SVGA ring buffers
    if (s && s_stillFs != s_previewFs) {
      s->set_framesize(s, s_previewFs);
      for (int flush = 0; flush < 2; ++flush) {
        if (camera_fb_t* flushFb = esp_camera_fb_get()) {
          esp_camera_fb_return(flushFb);
        }
      }
    }
  }

  if (ok && reqId != 0) {
    s_lastBootNonce = bootNonce;
    s_lastReqId = reqId;
    s_lastSynth = synthetic;
    s_lastOrientation = orientation;
    s_lastStillJpeg = outJpeg;
    s_lastStillW = outW;
    s_lastStillH = outH;
    s_lastStillCrc = outCrc;
  }

  if (s_camMutex) xSemaphoreGive(s_camMutex);
  return ok;
}

}  // namespace xiao_hw
