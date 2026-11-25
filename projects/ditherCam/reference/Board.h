// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "Display.h"

namespace rlcd_eyes {

class Canvas : public Adafruit_GFX {
 public:
  uint8_t bytes[frameBytes]{};
  Canvas() : Adafruit_GFX(rlcd_eyes::width, rlcd_eyes::height) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    pixel(bytes, x, y, color != 0);
  }
};

bool boardBegin();
uint32_t present(const uint8_t* bytes);

extern uint32_t transfers;
extern uint32_t transferMaxUs;

}  // namespace rlcd_eyes
