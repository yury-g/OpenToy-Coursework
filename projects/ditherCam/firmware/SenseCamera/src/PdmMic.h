// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>

namespace xiao_hw {

struct MicStatus {
  bool ok = false;
  uint64_t sampleCount = 0;
  uint32_t rms = 0;
  uint16_t peak = 0;
  char fault[32] = {};
};

bool micBegin();
MicStatus micStatus();
void micResetRecordBuffer();
size_t micPopPcmBytes(uint8_t* dst, size_t maxBytes);
uint32_t micStackHwm();

}  // namespace xiao_hw
