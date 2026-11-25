// SPDX-License-Identifier: MIT
#include "Board.h"
#include <SPI.h>
#include <algorithm>

namespace rlcd_eyes {

uint32_t transfers = 0;
uint32_t transferMaxUs = 0;

namespace {

constexpr int kPinSck = 11;
constexpr int kPinMosi = 12;
constexpr int kPinDc = 5;
constexpr int kPinCs = 40;
constexpr int kPinRst = 41;
constexpr int kPinPaCtrl = 46;
constexpr uint32_t kSpiHz = 10000000;

SPIClass bus(FSPI);

class Panel {
  void _cmd(uint8_t value) {
    digitalWrite(kPinDc, LOW);
    digitalWrite(kPinCs, LOW);
    bus.transfer(value);
    digitalWrite(kPinCs, HIGH);
  }

  void _cmd_data(uint8_t cmd, const uint8_t* data, size_t n) {
    _cmd(cmd);
    digitalWrite(kPinDc, HIGH);
    digitalWrite(kPinCs, LOW);
    bus.writeBytes(data, n);
    digitalWrite(kPinCs, HIGH);
  }

  void reset() {
    digitalWrite(kPinRst, HIGH);
    delay(50);
    digitalWrite(kPinRst, LOW);
    delay(20);
    digitalWrite(kPinRst, HIGH);
    delay(50);
  }

 public:
  void begin();

  void send(const uint8_t* data) {
    const uint8_t columns[] = {0x12, 0x2A};
    const uint8_t rows[] = {0x00, 0xC7};
    _cmd_data(0x2A, columns, sizeof(columns));
    _cmd_data(0x2B, rows, sizeof(rows));
    _cmd(0x2C);
    digitalWrite(kPinDc, HIGH);
    digitalWrite(kPinCs, LOW);
    bus.writeBytes(data, frameBytes);
    digitalWrite(kPinCs, HIGH);
  }
};

#include "PanelInit.h"

Panel panel;

}  // namespace

bool boardBegin() {
  // Exact RLCD codec configuration assigns PA_CTRL to GPIO46; keep speaker amp disabled.
  pinMode(kPinPaCtrl, OUTPUT);
  digitalWrite(kPinPaCtrl, LOW);

  pinMode(kPinDc, OUTPUT);
  pinMode(kPinCs, OUTPUT);
  pinMode(kPinRst, OUTPUT);
  digitalWrite(kPinCs, HIGH);

  bus.begin(kPinSck, -1, kPinMosi, kPinCs);
  bus.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
  panel.begin();
  bus.endTransaction();
  return true;
}

uint32_t present(const uint8_t* bytes) {
  const uint32_t startUs = micros();
  bus.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
  panel.send(bytes);
  bus.endTransaction();
  const uint32_t elapsedUs = micros() - startUs;
  ++transfers;
  transferMaxUs = std::max(transferMaxUs, elapsedUs);
  return elapsedUs;
}

}  // namespace rlcd_eyes
