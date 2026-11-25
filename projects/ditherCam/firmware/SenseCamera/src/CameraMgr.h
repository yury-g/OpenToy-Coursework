// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <cstdint>
#include <vector>

namespace xiao_hw {

struct CameraInfo {
  bool ok = false;
  uint16_t pid = 0;
  char sensorName[20] = "none";
  char previewRes[16] = "none";
  char stillRes[16] = "none";
  uint16_t previewW = 0;
  uint16_t previewH = 0;
  uint16_t stillW = 0;
  uint16_t stillH = 0;
  uint32_t framesOk = 0;
  uint32_t framesErr = 0;
  uint32_t prodCaptures = 0;
  uint32_t synthCaptures = 0;
  char fault[32] = {};
};

bool cameraBegin();
CameraInfo cameraInfo();

// Captures a preview JPEG (SVGA/VGA) and optionally copies into outJpeg
bool cameraGrabPreview(std::vector<uint8_t>& outJpeg, uint16_t& outW, uint16_t& outH);

// Idempotent high-res still capture (or synthetic test-pattern JPEG when synthetic=true)
// orientation: 0=selfie (90CW baseline), 1=rear (90CW + 180 rotation)
bool cameraCaptureStill(uint32_t bootNonce, uint32_t reqId, bool synthetic, uint32_t synthSeq,
                        uint8_t orientation,
                        std::vector<uint8_t>& outJpeg, uint16_t& outW, uint16_t& outH,
                        uint32_t& outCrc);

}  // namespace xiao_hw
