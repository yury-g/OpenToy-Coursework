// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>

inline uint8_t qwiicButtonBrightness(bool lit, bool pressed, uint8_t command,
                                     bool active, bool breathing = false) {
    if (!lit) return 0;
    if (pressed) return 128;
    if (!active) return 16;
    const unsigned brightness = breathing ? unsigned(command) * 6 : 16 + unsigned(command) * 3;
    const unsigned maximum = breathing ? 192 : 255;
    return uint8_t(brightness > maximum ? maximum : brightness);
}
