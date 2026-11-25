// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include "FlipCamDisplay.h"

namespace rlcd_hw {

bool panelBegin();
uint32_t panelPresent(const flipcam::RlcdFrameBuffer& fb);
uint32_t panelTransfers();
uint32_t panelMaxTransferUs();

}  // namespace rlcd_hw
