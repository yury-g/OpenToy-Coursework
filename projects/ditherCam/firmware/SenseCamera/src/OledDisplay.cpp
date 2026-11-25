// SPDX-License-Identifier: MIT
// Lightweight SSD1306 128x64 I2C driver for Seeed XIAO Expansion Base (0x3C, SDA=5, SCL=6).
#include "OledDisplay.h"
#include <Arduino.h>
#include <Wire.h>
#include <algorithm>

namespace xiao_hw {

namespace {

constexpr int kPinOledSda = 5;
constexpr int kPinOledScl = 6;
constexpr uint8_t kSsd1306Addr = 0x3C;

// Dedicated hardware I2C controller 0 (I2C_NUM_1) so OLED never shares I2C0 with Camera SCCB!
TwoWire s_oledWire(0);
bool s_oledOk = false;
uint32_t s_frames = 0;

bool sendCmd(uint8_t cmd) {
  s_oledWire.beginTransmission(kSsd1306Addr);
  s_oledWire.write(0x00);
  s_oledWire.write(cmd);
  return s_oledWire.endTransmission() == 0;
}

bool sendCmdList(const uint8_t* cmds, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (!sendCmd(cmds[i])) return false;
  }
  return true;
}

}  // namespace

bool oledBegin() {
  s_oledWire.begin(kPinOledSda, kPinOledScl, 400000);
  s_oledWire.beginTransmission(kSsd1306Addr);
  if (s_oledWire.endTransmission() != 0) {
    s_oledOk = false;
    return false;
  }

  static constexpr uint8_t kInitCmds[] = {
      0xAE,        // Display off
      0xD5, 0x80,  // Clock div
      0xA8, 0x3F,  // Multiplex 64
      0xD3, 0x00,  // Display offset 0
      0x40,        // Start line 0
      0x8D, 0x14,  // Enable charge pump
      0x20, 0x00,  // Horizontal addressing mode
      0xA1,        // Segment remap
      0xC8,        // COM scan dec
      0xDA, 0x12,  // COM pins
      0x81, 0xCF,  // Contrast
      0xD9, 0xF1,  // Pre-charge
      0xDB, 0x40,  // VCOMH
      0xA4,        // Resume RAM display
      0xA6,        // Normal display
      0x2E,        // Deactivate scroll
      0xAF         // Display ON
  };
  s_oledOk = sendCmdList(kInitCmds, sizeof(kInitCmds));
  return s_oledOk;
}

bool oledPresent(const flipcam::OledFrameBuffer& fb) {
  if (!s_oledOk) return false;
  static constexpr uint8_t kAddrWindow[] = {
      0x21, 0x00, 0x7F,  // Column 0..127
      0x22, 0x00, 0x07   // Page 0..7
  };
  if (!sendCmdList(kAddrWindow, sizeof(kAddrWindow))) {
    s_oledOk = false;
    return false;
  }
  for (size_t off = 0; off < flipcam::OledFrameBuffer::kBytes; off += 16) {
    s_oledWire.beginTransmission(kSsd1306Addr);
    s_oledWire.write(0x40);
    s_oledWire.write(fb.bytes + off, std::min<size_t>(16, flipcam::OledFrameBuffer::kBytes - off));
    if (s_oledWire.endTransmission() != 0) {
      s_oledOk = false;
      return false;
    }
  }
  s_frames++;
  return true;
}

bool oledIsOk() { return s_oledOk; }
uint32_t oledFrameCount() { return s_frames; }

}  // namespace xiao_hw
