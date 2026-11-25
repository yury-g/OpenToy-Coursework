// SPDX-License-Identifier: MIT
#pragma once

#include <Arduino.h>
#include <initializer_list>
#include <zephyr/drivers/gpio.h>

namespace opentoy {
namespace uno_q {

// MAR2808-style 8080 shield. RD stays inactive for the entire sketch.
// The shield's SD, touch, power and ground connections are left alone.
class ParallelPanel {
 public:
  static constexpr uint16_t kWidth = 320;
  static constexpr uint16_t kHeight = 240;

  bool begin() {
    // UNO Q header: A0..A3 map to PA4..PA7, and A4 maps to PC1.
    const auto* portA = DEVICE_DT_GET(DT_NODELABEL(gpioa));
    const auto* portC = DEVICE_DT_GET(DT_NODELABEL(gpioc));
    if (!outputHigh(portA, 4) || !outputHigh(portA, 7) || !outputHigh(portA, 5) ||
        !outputHigh(portA, 6) || !outputHigh(portC, 1)) {
      return false;
    }
    for (const auto pin : kDataPins) {
      pinMode(pin, OUTPUT);
    }
    digitalWrite(A4, LOW);
    delay(20);
    digitalWrite(A4, HIGH);
    delay(150);

    command(0x01);  // Software reset.
    delay(150);
    command(0xC0, {0x23});        // Power control 1.
    command(0xC1, {0x10});        // Power control 2.
    command(0xC5, {0x3E, 0x28});  // VCOM control 1.
    command(0xC7, {0x86});        // VCOM control 2.
    command(0x36, {0x28});        // Landscape, BGR panel order.
    command(0x3A, {0x55});        // RGB565 on the MCU interface.
    command(0x11);               // Sleep out.
    delay(150);
    command(0x29);               // Display on.
    return true;
  }

  void startPixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    const uint16_t right = x + width - 1;
    const uint16_t bottom = y + height - 1;
    command(0x2A, {high(x), low(x), high(right), low(right)});
    command(0x2B, {high(y), low(y), high(bottom), low(bottom)});
    digitalWrite(A3, LOW);
    digitalWrite(A2, LOW);
    writeByte(0x2C);
    digitalWrite(A2, HIGH);
  }

  void pixel(uint16_t rgb) {
    writeByte(high(rgb));
    writeByte(low(rgb));
  }

  void endPixels() { digitalWrite(A3, HIGH); }

 private:
  static constexpr pin_size_t kDataPins[] = {D8, D9, D2, D3, D4, D5, D6, D7};

  static bool outputHigh(const struct device* port, gpio_pin_t index) {
    // pinMode(OUTPUT) starts low on this core, so configure idle-high atomically.
    return gpio_pin_configure(port, index, GPIO_OUTPUT_HIGH) == 0;
  }

  static uint8_t high(uint16_t value) { return uint8_t(value >> 8); }
  static uint8_t low(uint16_t value) { return uint8_t(value); }

  static void writeByte(uint8_t value) {
    for (unsigned bit = 0; bit < sizeof(kDataPins) / sizeof(kDataPins[0]); ++bit) {
      digitalWrite(kDataPins[bit], (value >> bit) & 1u ? HIGH : LOW);
    }
    digitalWrite(A1, LOW);
    digitalWrite(A1, HIGH);
  }

  static void command(uint8_t opcode, std::initializer_list<uint8_t> payload = {}) {
    digitalWrite(A3, LOW);
    digitalWrite(A2, LOW);
    writeByte(opcode);
    digitalWrite(A2, HIGH);
    for (const uint8_t byte : payload) {
      writeByte(byte);
    }
    digitalWrite(A3, HIGH);
  }
};

}  // namespace uno_q
}  // namespace opentoy
