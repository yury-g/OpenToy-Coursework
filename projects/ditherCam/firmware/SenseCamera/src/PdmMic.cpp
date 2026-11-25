// SPDX-License-Identifier: MIT
// XIAO ESP32S3 Sense onboard PDM Microphone: CLK=42, DATA=41 via ESP_I2S PDM RX.
#include "PdmMic.h"
#include <Arduino.h>
#include <ESP_I2S.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace xiao_hw {

namespace {

constexpr int kPinPdmClk = 42;
constexpr int kPinPdmData = 41;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kRingBytes = 64u * 1024u;  // ~2s mono 16kHz S16LE ring in PSRAM

I2SClass s_pdm;
MicStatus s_status{};
SemaphoreHandle_t s_micMutex = nullptr;
TaskHandle_t s_micTaskHandle = nullptr;
uint8_t* s_ring = nullptr;
size_t s_head = 0;
size_t s_tail = 0;
size_t s_count = 0;

void pushPcmToRing(const uint8_t* data, size_t len) {
  if (!s_ring || len == 0) return;
  if (s_micMutex && xSemaphoreTake(s_micMutex, pdMS_TO_TICKS(10)) != pdTRUE) return;
  for (size_t i = 0; i < len; ++i) {
    s_ring[s_head] = data[i];
    s_head = (s_head + 1) % kRingBytes;
    if (s_count == kRingBytes) {
      s_tail = (s_tail + 1) % kRingBytes;  // overwrite oldest
    } else {
      s_count++;
    }
  }
  if (s_micMutex) xSemaphoreGive(s_micMutex);
}

void micTask(void*) {
  int16_t buf[512];
  for (;;) {
    if (!s_status.ok) {
      delay(200);
      continue;
    }
    const size_t got = s_pdm.readBytes(reinterpret_cast<char*>(buf), sizeof(buf));
    if (got >= 2) {
      const size_t samples = got / sizeof(int16_t);
      uint64_t sumSq = 0;
      uint16_t pk = 0;
      for (size_t i = 0; i < samples; ++i) {
        const int32_t v = buf[i];
        const uint16_t av = static_cast<uint16_t>(v < 0 ? -v : v);
        if (av > pk) pk = av;
        sumSq += static_cast<uint64_t>(v) * static_cast<uint64_t>(v);
      }
      s_status.sampleCount += samples;
      s_status.peak = pk;
      s_status.rms = static_cast<uint32_t>(std::sqrt(static_cast<double>(sumSq) / static_cast<double>(samples)));
      pushPcmToRing(reinterpret_cast<const uint8_t*>(buf), samples * sizeof(int16_t));
    } else {
      delay(5);
    }
  }
}

}  // namespace

bool micBegin() {
  if (!s_micMutex) s_micMutex = xSemaphoreCreateMutex();
  s_ring = static_cast<uint8_t*>(heap_caps_malloc(kRingBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  s_status = MicStatus{};

  s_pdm.setPinsPdmRx(kPinPdmClk, kPinPdmData);
  if (!s_pdm.begin(I2S_MODE_PDM_RX, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    std::snprintf(s_status.fault, sizeof(s_status.fault), "PDM_RX_BEGIN_FAIL");
    s_status.ok = false;
    return false;
  }
  s_status.ok = true;
  xTaskCreatePinnedToCore(micTask, "xiao_mic", 4096, nullptr, 1, &s_micTaskHandle, 0);
  return true;
}

uint32_t micStackHwm() {
  return s_micTaskHandle ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(s_micTaskHandle)) : 0;
}

MicStatus micStatus() { return s_status; }

void micResetRecordBuffer() {
  if (s_micMutex && xSemaphoreTake(s_micMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    s_head = 0;
    s_tail = 0;
    s_count = 0;
    xSemaphoreGive(s_micMutex);
  }
}

size_t micPopPcmBytes(uint8_t* dst, size_t maxBytes) {
  if (!s_ring || !dst || maxBytes < 2) return 0;
  // Keep even byte count for 16-bit PCM
  maxBytes &= ~static_cast<size_t>(1u);
  size_t copied = 0;
  if (s_micMutex && xSemaphoreTake(s_micMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    const size_t n = std::min(maxBytes, s_count) & ~static_cast<size_t>(1u);
    for (size_t i = 0; i < n; ++i) {
      dst[i] = s_ring[s_tail];
      s_tail = (s_tail + 1) % kRingBytes;
    }
    s_count -= n;
    copied = n;
    xSemaphoreGive(s_micMutex);
  }
  return copied;
}

}  // namespace xiao_hw
