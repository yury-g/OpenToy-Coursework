#pragma once

#include <Adafruit_LSM6DS3TRC.h>
#include <Adafruit_MPR121.h>
#include <Wire.h>
#include "Reactions.h"

class Inputs {
 public:
  void begin() {
    pinMode(TFT_I2C_POWER, OUTPUT);
    digitalWrite(TFT_I2C_POWER, HIGH);
    delay(20);
    if (!Wire.begin(SDA, SCL, 100000)) return;
    Wire.setTimeOut(15);
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
      Wire.beginTransmission(address);
      found_[address] = Wire.endTransmission() == 0;
    }

    // Let the MPR121 match its charge time/current to the Gator pads.
    for (uint8_t address = 0x5a; address <= 0x5d && !touchAddress; ++address) {
      if (found_[address] && touch_.begin(address, &Wire, 8, 4, true)) touchAddress = address;
    }
    for (uint8_t address = 0x6a; address <= 0x6b && !imuAddress; ++address) {
      if (found_[address] && imu_.begin_I2C(address, &Wire)) imuAddress = address;
    }
    if (imuAddress) {
      imu_.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
      imu_.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
      imu_.setAccelDataRate(LSM6DS_RATE_104_HZ);
      imu_.setGyroDataRate(LSM6DS_RATE_104_HZ);
    }
  }

  void poll() {
    uint8_t data[12];
    touchOk = touchAddress && read(touchAddress, MPR121_TOUCHSTATUS_L, data, 4);
    touchOutOfRange = touchOk ? unsignedOf(data + 2) & 0x0fff : 0;
    pads = touchOk ? unsignedOf(data) & ~touchOutOfRange & 0x0fff : 0;
    motionOk = imuAddress && read(imuAddress, LSM6DS_OUTX_L_G, data, sizeof data);
    if (!motionOk) return;

    // One coherent burst. Sensitivities correspond to the configured ranges.
    constexpr float accelScale = 0.000122f;
    constexpr float gyroScale = 0.0175f;
    const float gx = signedOf(data) * gyroScale;
    const float gy = signedOf(data + 2) * gyroScale;
    const float gz = signedOf(data + 4) * gyroScale;
    motion = {signedOf(data + 6) * accelScale, signedOf(data + 8) * accelScale,
              signedOf(data + 10) * accelScale, std::sqrt(gx * gx + gy * gy + gz * gz),
              gx, gy, gz};
  }

  void printScan() const {
    Serial.print("I2C");
    for (unsigned address = 0x08; address <= 0x77; ++address) {
      if (found_[address]) Serial.printf(" 0x%02x", address);
    }
    Serial.println();
  }

  uint8_t touchAddress = 0;
  uint8_t imuAddress = 0;
  uint16_t pads = 0;
  uint16_t touchOutOfRange = 0;
  bool touchOk = false;
  bool motionOk = false;
  MotionSample motion{};

 private:
  static uint16_t unsignedOf(const uint8_t *data) {
    return uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  }

  static int16_t signedOf(const uint8_t *data) { return int16_t(unsignedOf(data)); }

  static bool read(uint8_t address, uint8_t reg, uint8_t *data, size_t size) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(address, size, true) != size) return false;
    return Wire.readBytes(data, size) == size;
  }

  Adafruit_MPR121 touch_;
  Adafruit_LSM6DS3TRC imu_;
  bool found_[128] = {};
};
