// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace rlcd_hw {

struct AudioStatus {
  bool codecOk = false;
  bool i2sOk = false;
  bool ampEnabled = false;
  uint8_t chipId1 = 0;
  uint8_t chipId2 = 0;
  uint64_t i2sTxBytes = 0;
  char fault[32] = {};
};

bool audioBegin();
AudioStatus audioStatus();
void audioSetAmp(bool enable);
bool audioPlayPcmMono16(const int16_t* samples, size_t sampleCount, bool soundEnabled);
bool audioPlayTone(uint32_t freqHz, uint32_t durationMs, bool soundEnabled);
void audioPlayShutterClick(bool soundEnabled);

}  // namespace rlcd_hw
