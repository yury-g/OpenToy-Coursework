// SPDX-License-Identifier: MIT
#pragma once
#include "Interaction.h"
#include "Metadata.h"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace touch169 {

struct Diagnostics {
  bool touch = false;
  bool touchDown = false;
  int touchX = 0;
  int touchY = 0;
  int batteryMv = -1;
  int rawBatteryMv = -1;
  uint32_t frames = 0;
  uint32_t errors = 0;
  uint32_t heap = 0;

  bool imuPresent = false;
  bool imuFresh = false;
  uint8_t imuAddr = 0;
  uint8_t imuWho = 0;
  uint8_t imuRev = 0;
  uint32_t imuSamples = 0;
  uint32_t imuErrors = 0;
  int32_t imuAgeMs = -1;
  float ax = 0;
  float ay = 0;
  float az = 0;
  float gx = 0;
  float gy = 0;
  float gz = 0;
  float downX = 0;
  float downY = 0;
  float gravityStrength = 0;
  uint32_t waterShakes = 0;
  int waterHeat = 0;
  int waterEnergy = 0;
  bool waterSleeping = false;
};

class SerialCommandParser {
 public:
  bool feed(uint8_t ch) {
    if (ch == '?' && len_ == 0 && !overflow_) {
      return true;
    }
    if (ch == '\n' || ch == '\r') {
      bool matched = !overflow_ && len_ == 6 &&
                     (std::memcmp(buf_, "status", 6) == 0);
      len_ = 0;
      overflow_ = false;
      return matched;
    }
    if (ch == ' ' || ch == '\t') {
      if (len_ == 0) return false;
      return false;
    }
    if (ch == '?') {
      len_ = 0;
      overflow_ = false;
      return true;
    }
    if (overflow_) return false;
    if (len_ >= sizeof(buf_)) {
      overflow_ = true;
      return false;
    }
    if (ch >= 'A' && ch <= 'Z') ch = uint8_t(ch + 32);
    buf_[len_++] = char(ch);
    return false;
  }

 private:
  char buf_[16] = {};
  size_t len_ = 0;
  bool overflow_ = false;
};

class TelemetryQueue {
 public:
  bool pending() const { return sent_ < used_; }

  void append(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(bytes_ + used_, sizeof(bytes_) - used_, format, args);
    va_end(args);
    if (n > 0 && size_t(n) < sizeof(bytes_) - used_) {
      used_ += size_t(n);
    }
  }

  template <class Port>
  void pump(Port& port) {
    if (!port) {
      used_ = sent_ = 0;
      return;
    }
    if (!pending()) return;
    int space = port.availableForWrite();
    if (space <= 0) return;
    size_t n = used_ - sent_;
    if (n > size_t(space)) n = size_t(space);
    sent_ += port.write(reinterpret_cast<const unsigned char*>(bytes_ + sent_), n);
    if (sent_ == used_) used_ = sent_ = 0;
  }

 private:
  char bytes_[1024] = {};
  size_t used_ = 0;
  size_t sent_ = 0;
};

inline void formatStatusReport(TelemetryQueue& q, const Interaction& ui, const Diagnostics& d) {
  q.append("TOUCH169 version=%s created=%s app=%s page=%s header=%d persona=%u brightness=%u frames=%lu heap=%lu\n",
           version, created, appShortName(ui.app), pageName(ui.page), ui.header ? 1 : 0,
           ui.persona, ui.brightness, (unsigned long)d.frames, (unsigned long)d.heap);
  q.append("TOUCH ok=%d down=%d x=%d y=%d pokes=%lu swipes=%lu errors=%lu bat_raw_mv=%d bat_mv=%d\n",
           d.touch ? 1 : 0, d.touchDown ? 1 : 0, d.touchX, d.touchY,
           (unsigned long)ui.pokes, (unsigned long)ui.swipes, (unsigned long)d.errors,
           d.rawBatteryMv, d.batteryMv);
  q.append("IMU present=%d fresh=%d addr=0x%02x who=0x%02x rev=0x%02x samples=%lu errors=%lu age_ms=%ld accel=%.4f,%.4f,%.4f gyro=%.2f,%.2f,%.2f down=%.4f,%.4f\n",
           d.imuPresent ? 1 : 0, d.imuFresh ? 1 : 0, d.imuAddr, d.imuWho, d.imuRev,
           (unsigned long)d.imuSamples, (unsigned long)d.imuErrors, (long)d.imuAgeMs,
           d.ax, d.ay, d.az, d.gx, d.gy, d.gz, d.downX, d.downY);
  if (ui.app == App::Water) {
    q.append("WATER particles=400 heat=%d energy=%d sleeping=%d shakes=%lu strength=%.3f\n",
             d.waterHeat, d.waterEnergy, d.waterSleeping ? 1 : 0,
             (unsigned long)d.waterShakes, d.gravityStrength);
  }
}

}  // namespace touch169
