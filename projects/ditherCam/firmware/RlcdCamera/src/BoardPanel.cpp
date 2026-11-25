// SPDX-License-Identifier: MIT
// Preserves exact ST7305 panel initialization and SPI pins from reference/Board.cpp & reference/PanelInit.h.
#include "BoardPanel.h"
#include <SPI.h>
#include <algorithm>

namespace rlcd_hw {

namespace {

constexpr int kPinSck = 11;
constexpr int kPinMosi = 12;
constexpr int kPinDc = 5;
constexpr int kPinCs = 40;
constexpr int kPinRst = 41;
constexpr int kPinPaCtrl = 46;
constexpr uint32_t kSpiHz = 10000000;

SPIClass bus(FSPI);
uint32_t s_transfers = 0;
uint32_t s_maxUs = 0;

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
    bus.writeBytes(data, flipcam::RlcdFrameBuffer::kBytes);
    digitalWrite(kPinCs, HIGH);
  }
};

#include "PanelInit.h"

Panel panel;

}  // namespace

bool panelBegin() {
  pinMode(kPinPaCtrl, OUTPUT);
  digitalWrite(kPinPaCtrl, LOW);  // Speaker amp disabled at boot until active playback

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

uint32_t panelPresent(const flipcam::RlcdFrameBuffer& fb) {
  const uint32_t startUs = micros();
  bus.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
  panel.send(fb.bytes);
  bus.endTransaction();
  const uint32_t elapsedUs = micros() - startUs;
  ++s_transfers;
  s_maxUs = std::max(s_maxUs, elapsedUs);
  return elapsedUs;
}

uint32_t panelTransfers() { return s_transfers; }
uint32_t panelMaxTransferUs() { return s_maxUs; }

}  // namespace rlcd_hw
