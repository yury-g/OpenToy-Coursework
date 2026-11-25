// SPDX-License-Identifier: MIT
// ES8311 audio DAC driver for Waveshare ESP32-S3-RLCD-4.2.
// Pins: I2C0 SDA=13, SCL=14 (addr 0x18); I2S BCLK=9, LRCLK=45, DOUT=8, MCLK=16; PA_CTRL=46.
#include "AudioCodec.h"
#include <ESP_I2S.h>
#include <Wire.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rlcd_hw {

namespace {

constexpr int kPinI2cSda = 13;
constexpr int kPinI2cScl = 14;
constexpr uint8_t kEs8311Addr = 0x18;

constexpr int kPinI2sBclk = 9;
constexpr int kPinI2sLrclk = 45;
constexpr int kPinI2sDout = 8;
constexpr int kPinI2sMclk = 16;
constexpr int kPinPaCtrl = 46;

constexpr uint32_t kSampleRate = 16000;

I2SClass s_i2s;
AudioStatus s_status{};
SemaphoreHandle_t s_audioMutex = nullptr;

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(kEs8311Addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool readReg(uint8_t reg, uint8_t& outVal) {
  Wire.beginTransmission(kEs8311Addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(kEs8311Addr), 1) != 1) return false;
  outVal = static_cast<uint8_t>(Wire.read());
  return true;
}

bool initEs8311Registers() {
  uint8_t id1 = 0, id2 = 0;
  // ES8311 requires noise immunity write twice per Espressif es8311.c
  if (!writeReg(0x44, 0x08)) return false;
  writeReg(0x44, 0x08);

  if (readReg(0xFD, id1) && readReg(0xFE, id2)) {
    s_status.chipId1 = id1;
    s_status.chipId2 = id2;
  }

  // Exact Espressif es8311_open + es8311_set_fs(16000Hz, 16-bit, 256*fs=4.096MHz MCLK) + es8311_start
  bool ok = true;
  ok &= writeReg(0x01, 0x30);
  ok &= writeReg(0x02, 0x00);
  ok &= writeReg(0x03, 0x10);
  ok &= writeReg(0x16, 0x24);
  ok &= writeReg(0x04, 0x10);
  ok &= writeReg(0x05, 0x00);
  ok &= writeReg(0x0B, 0x00);
  ok &= writeReg(0x0C, 0x00);
  ok &= writeReg(0x10, 0x1F);
  ok &= writeReg(0x11, 0x7F);
  ok &= writeReg(0x00, 0x80);  // Slave mode, CSM power on
  ok &= writeReg(0x01, 0x3F);  // Use external MCLK pin, enable all clocks
  ok &= writeReg(0x06, 0x03);  // BCLK div=4 (4-1=3)
  ok &= writeReg(0x13, 0x10);
  ok &= writeReg(0x1B, 0x0A);
  ok &= writeReg(0x1C, 0x6A);
  ok &= writeReg(0x44, 0x58);  // Internal ref

  // 16-bit Philips I2S on SDPIN (0x09) and SDPOUT (0x0A), DAC enabled (bit6=0)
  ok &= writeReg(0x09, 0x0C);
  ok &= writeReg(0x0A, 0x4C);  // ADC tri-stated (bit6=1), DAC active

  // Clock dividers for 4.096MHz MCLK / 16000Hz: pre_div=1, pre_multi=1, adc_div=1, dac_div=1
  ok &= writeReg(0x02, 0x00);
  ok &= writeReg(0x05, 0x00);
  ok &= writeReg(0x03, 0x10);  // fs_mode=0, adc_osr=0x10
  ok &= writeReg(0x04, 0x20);  // dac_osr=0x20
  ok &= writeReg(0x07, 0x00);  // lrck_h=0
  ok &= writeReg(0x08, 0xFF);  // lrck_l=0xFF

  // Power up analog & DAC output
  ok &= writeReg(0x17, 0xBF);
  ok &= writeReg(0x0E, 0x02);
  ok &= writeReg(0x12, 0x00);
  ok &= writeReg(0x14, 0x1A);
  ok &= writeReg(0x0D, 0x01);
  ok &= writeReg(0x15, 0x40);
  ok &= writeReg(0x37, 0x08);
  ok &= writeReg(0x45, 0x00);

  // Set comfortable 0dB DAC volume (0xBF) and mute until playback begins
  ok &= writeReg(0x32, 0xBF);
  ok &= writeReg(0x31, 0x60);
  return ok;
}

}  // namespace

bool audioBegin() {
  if (!s_audioMutex) {
    s_audioMutex = xSemaphoreCreateMutex();
  }
  s_status = AudioStatus{};
  pinMode(kPinPaCtrl, OUTPUT);
  digitalWrite(kPinPaCtrl, LOW);

  // Start I2S first so MCLK (4.096MHz) is active before ES8311 I2C init
  s_i2s.setPins(kPinI2sBclk, kPinI2sLrclk, kPinI2sDout, -1, kPinI2sMclk);
  if (!s_i2s.begin(I2S_MODE_STD, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    std::snprintf(s_status.fault, sizeof(s_status.fault), "I2S_BEGIN_FAIL");
    return false;
  }
  s_status.i2sOk = true;

  Wire.begin(kPinI2cSda, kPinI2cScl, 400000);
  if (!initEs8311Registers()) {
    std::snprintf(s_status.fault, sizeof(s_status.fault), "ES8311_I2C_FAIL");
    s_status.codecOk = false;
    return false;
  }
  s_status.codecOk = true;
  return true;
}

AudioStatus audioStatus() {
  return s_status;
}

void audioSetAmp(bool enable) {
  if (!s_status.codecOk) enable = false;
  if (enable) {
    writeReg(0x31, 0x00);  // Unmute DAC
    digitalWrite(kPinPaCtrl, HIGH);
    s_status.ampEnabled = true;
  } else {
    digitalWrite(kPinPaCtrl, LOW);
    writeReg(0x31, 0x60);  // Soft mute DAC
    s_status.ampEnabled = false;
  }
}

bool audioPlayPcmMono16(const int16_t* samples, size_t sampleCount, bool soundEnabled) {
  if (!soundEnabled || !s_status.codecOk || !s_status.i2sOk || !samples || sampleCount == 0) {
    return false;
  }
  if (s_audioMutex && xSemaphoreTake(s_audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }

  static constexpr size_t kChunkSamples = 256;
  int16_t stereo[kChunkSamples * 2];
  size_t off = 0;
  bool ok = true;

  while (off < sampleCount) {
    const size_t n = std::min(kChunkSamples, sampleCount - off);
    for (size_t i = 0; i < n; ++i) {
      const int16_t v = samples[off + i];
      stereo[i * 2] = v;
      stereo[i * 2 + 1] = v;
    }
    const size_t bytesToWrite = n * 2 * sizeof(int16_t);
    const size_t written = s_i2s.write(reinterpret_cast<const uint8_t*>(stereo), bytesToWrite);
    s_status.i2sTxBytes += written;
    if (written == 0) {
      ok = false;
      break;
    }
    off += n;
  }

  if (s_audioMutex) xSemaphoreGive(s_audioMutex);
  return ok;
}

bool audioPlayTone(uint32_t freqHz, uint32_t durationMs, bool soundEnabled) {
  if (!soundEnabled || !s_status.codecOk || !s_status.i2sOk) return false;
  if (freqHz < 100 || freqHz > 8000 || durationMs < 10 || durationMs > 3000) return false;

  const size_t totalSamples = (static_cast<size_t>(kSampleRate) * durationMs) / 1000u;
  static constexpr size_t kBufSamples = 256;
  int16_t buf[kBufSamples];
  const float omega = 2.0f * 3.14159265f * static_cast<float>(freqHz) / static_cast<float>(kSampleRate);

  audioSetAmp(true);
  size_t pos = 0;
  while (pos < totalSamples) {
    const size_t n = std::min(kBufSamples, totalSamples - pos);
    for (size_t i = 0; i < n; ++i) {
      const size_t idx = pos + i;
      float env = 1.0f;
      if (idx < 64) env = static_cast<float>(idx) / 64.0f;
      else if (totalSamples - idx < 64) env = static_cast<float>(totalSamples - idx) / 64.0f;
      const float s = std::sin(omega * static_cast<float>(idx)) * 9000.0f * env;
      buf[i] = static_cast<int16_t>(s);
    }
    if (!audioPlayPcmMono16(buf, n, true)) break;
    pos += n;
  }
  // Write a tiny silence tail to flush DMA cleanly before disabling PA
  std::memset(buf, 0, sizeof(buf));
  audioPlayPcmMono16(buf, 128, true);
  audioSetAmp(false);
  return pos == totalSamples;
}

void audioPlayShutterClick(bool soundEnabled) {
  if (!soundEnabled || !s_status.codecOk) return;
  audioPlayTone(1047, 35, true);
}

}  // namespace rlcd_hw
