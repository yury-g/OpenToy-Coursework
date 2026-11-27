// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>

namespace touch169 {
namespace qmi8658 {

// Verified against official Waveshare ESP32-S3-Touch-LCD-1.69 repository:
//   https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.69
//   revision f24f10d53d4cff5c2e2ac47a2e23da39a82454a6
//   files:
//     - examples/arduino/libraries/SensorLib/src/SensorQMI8658.hpp
//     - examples/esp-idf/04_QMI8658/components/SensorLib/src/REG/QMI8658Constants.h
//     - examples/arduino/examples/10_LVGL_QMI8658_ui/10_LVGL_QMI8658_ui.ino
//     - hardware/schematic/ESP32-S3-Touch-LCD-1.69_V2.1.pdf (SAO=VCC3V3 -> 0x6B)

constexpr uint8_t kAddrPrimary = 0x6B;
constexpr uint8_t kAddrSecondary = 0x6A;

constexpr uint8_t kRegWhoAmI = 0x00;
constexpr uint8_t kRegRevision = 0x01;
constexpr uint8_t kRegCtrl1 = 0x02;
constexpr uint8_t kRegCtrl2 = 0x03;
constexpr uint8_t kRegCtrl3 = 0x04;
constexpr uint8_t kRegCtrl5 = 0x06;
constexpr uint8_t kRegCtrl7 = 0x08;
constexpr uint8_t kRegStatus0 = 0x2E;
constexpr uint8_t kRegAxL = 0x35;
constexpr uint8_t kRegRstResult = 0x4D;
constexpr uint8_t kRegReset = 0x60;

constexpr uint8_t kExpectedWhoAmI = 0x05;
constexpr uint8_t kResetCmd = 0xB0;
constexpr uint8_t kRstResultDone = 0x80;

// CTRL1: bit 6 = ADDR_AI (auto-increment, little-endian output).
constexpr uint8_t kCtrl1Value = 0x40;
// CTRL2: ACC_RANGE_4G (1 << 4 = 0x10) | ACC_ODR_250Hz (0x05) = 0x15.
constexpr uint8_t kCtrl2Value = 0x15;
// CTRL3: GYR_RANGE_512DPS (5 << 4 = 0x50) | GYR_ODR_224_2Hz (0x05) = 0x55.
constexpr uint8_t kCtrl3Value = 0x55;
// CTRL5: Gyro LPF_MODE_3 + enable (0x70) | Accel LPF_MODE_0 + enable (0x01) = 0x71.
constexpr uint8_t kCtrl5Value = 0x71;
// CTRL7: aEN (0x01) | gEN (0x02) = 0x03, async sample mode.
constexpr uint8_t kCtrl7Value = 0x03;

constexpr uint8_t kStatus0AccelAvail = 0x01;
constexpr uint8_t kStatus0GyroAvail = 0x02;
constexpr uint8_t kStatus0BothAvail = kStatus0AccelAvail | kStatus0GyroAvail;

constexpr float kAccelScale4G = 4.0f / 32768.0f;
constexpr float kGyroScale512Dps = 512.0f / 32768.0f;

constexpr uint32_t kStaleMs = 200;
constexpr uint32_t kRetryIntervalMs = 2000;
constexpr unsigned kMaxConsecutiveErrors = 10;

struct Sample {
  float ax = 0;
  float ay = 0;
  float az = 0;
  float gx = 0;
  float gy = 0;
  float gz = 0;
};

struct ImuState {
  bool present = false;
  bool hasSample = false;
  uint8_t address = 0;
  uint8_t whoAmI = 0;
  uint8_t revision = 0;
  uint32_t lastSampleMs = 0;
  uint32_t lastProbeMs = 0;
  uint32_t samples = 0;
  uint32_t errors = 0;
  unsigned consecutiveErrors = 0;
  float ax = 0;
  float ay = 0;
  float az = 0;
  float gx = 0;
  float gy = 0;
  float gz = 0;

  bool isFresh(uint32_t nowMs) const {
    return present && hasSample && uint32_t(nowMs - lastSampleMs) < kStaleMs;
  }

  int32_t ageMs(uint32_t nowMs) const {
    if (!hasSample) return -1;
    uint32_t delta = uint32_t(nowMs - lastSampleMs);
    return delta > 0x7fffffffu ? 0x7fffffff : int32_t(delta);
  }
};

inline int16_t decodeLe16(const uint8_t* p) {
  return static_cast<int16_t>(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
}

inline void decodeSample(const uint8_t raw[12], Sample& out) {
  out.ax = decodeLe16(raw + 0) * kAccelScale4G;
  out.ay = decodeLe16(raw + 2) * kAccelScale4G;
  out.az = decodeLe16(raw + 4) * kAccelScale4G;
  out.gx = decodeLe16(raw + 6) * kGyroScale512Dps;
  out.gy = decodeLe16(raw + 8) * kGyroScale512Dps;
  out.gz = decodeLe16(raw + 10) * kGyroScale512Dps;
}

template <class Bus>
bool configureAtAddress(Bus& bus, uint8_t addr, ImuState& state) {
  uint8_t who = 0;
  if (!bus.readReg(addr, kRegWhoAmI, &who, 1) || who != kExpectedWhoAmI) {
    return false;
  }

  // Soft-reset only after confirming WHO_AM_I == 0x05.
  if (!bus.writeReg(addr, kRegReset, kResetCmd)) {
    return false;
  }

  uint8_t rstResult = 0;
  for (int attempt = 0; attempt < 6; ++attempt) {
    if (bus.readReg(addr, kRegRstResult, &rstResult, 1) && rstResult == kRstResultDone) {
      break;
    }
    bus.delayMs(5);
  }

  if (!bus.readReg(addr, kRegWhoAmI, &who, 1) || who != kExpectedWhoAmI) {
    return false;
  }

  uint8_t rev = 0;
  bus.readReg(addr, kRegRevision, &rev, 1);

  if (!bus.writeReg(addr, kRegCtrl1, kCtrl1Value) ||
      !bus.writeReg(addr, kRegCtrl2, kCtrl2Value) ||
      !bus.writeReg(addr, kRegCtrl3, kCtrl3Value) ||
      !bus.writeReg(addr, kRegCtrl5, kCtrl5Value) ||
      !bus.writeReg(addr, kRegCtrl7, kCtrl7Value)) {
    return false;
  }

  uint8_t rb[4] = {};
  if (!bus.readReg(addr, kRegCtrl2, &rb[0], 1) || rb[0] != kCtrl2Value ||
      !bus.readReg(addr, kRegCtrl3, &rb[1], 1) || rb[1] != kCtrl3Value ||
      !bus.readReg(addr, kRegCtrl5, &rb[2], 1) || rb[2] != kCtrl5Value ||
      !bus.readReg(addr, kRegCtrl7, &rb[3], 1) || (rb[3] & kCtrl7Value) != kCtrl7Value) {
    return false;
  }

  state.present = true;
  state.hasSample = false;
  state.address = addr;
  state.whoAmI = who;
  state.revision = rev;
  state.consecutiveErrors = 0;
  return true;
}

template <class Bus>
bool probeAndConfigure(Bus& bus, ImuState& state, uint32_t nowMs) {
  state.lastProbeMs = nowMs;
  state.present = false;
  state.hasSample = false;
  state.address = 0;
  state.whoAmI = 0;
  state.revision = 0;
  for (uint8_t addr : {kAddrPrimary, kAddrSecondary}) {
    if (configureAtAddress(bus, addr, state)) {
      return true;
    }
  }
  return false;
}

template <class Bus>
bool pollSample(Bus& bus, ImuState& state, uint32_t nowMs) {
  if (!state.present) return false;
  uint8_t status0 = 0;
  if (!bus.readReg(state.address, kRegStatus0, &status0, 1)) {
    ++state.errors;
    if (++state.consecutiveErrors >= kMaxConsecutiveErrors) {
      state.present = false;
    }
    return false;
  }
  if ((status0 & kStatus0BothAvail) != kStatus0BothAvail) {
    return false;
  }
  uint8_t raw[12] = {};
  if (!bus.readReg(state.address, kRegAxL, raw, sizeof raw)) {
    ++state.errors;
    if (++state.consecutiveErrors >= kMaxConsecutiveErrors) {
      state.present = false;
    }
    return false;
  }
  Sample s;
  decodeSample(raw, s);
  state.ax = s.ax;
  state.ay = s.ay;
  state.az = s.az;
  state.gx = s.gx;
  state.gy = s.gy;
  state.gz = s.gz;
  state.lastSampleMs = nowMs;
  state.hasSample = true;
  state.consecutiveErrors = 0;
  ++state.samples;
  return true;
}

}  // namespace qmi8658
}  // namespace touch169
