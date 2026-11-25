// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include "FlipCamDisplay.h"

namespace xiao_hw {

bool oledBegin();
bool oledPresent(const flipcam::OledFrameBuffer& fb);
bool oledIsOk();
uint32_t oledFrameCount();

}  // namespace xiao_hw
