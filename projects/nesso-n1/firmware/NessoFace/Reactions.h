#pragma once

#include <cmath>
#include <cstdint>

enum class Tickle { Left, Middle, Right, AllOver };

enum class Mood { Idle, Smile, Startled, Dizzy, Giggle };

struct MotionSample {
  float x, y, z;  // Acceleration in g, with the IMU secured to the Feather.
  float spin;      // Angular speed in degrees per second.
  float gx, gy, gz;  // Signed angular rate about each IMU axis, in degrees per second.
};

class Reactions {
 public:
  void sample(uint32_t now, uint16_t pads, const MotionSample *motion) {
    if (pads & ~pads_) {
      touched_ = true;
      touchAt_ = now;
    }
    if (pads) {
      unsigned zoneCount = 0;
      if (pads & 0x00f) { tickle_ = Tickle::Left; ++zoneCount; }
      if (pads & 0x0f0) { tickle_ = Tickle::Middle; ++zoneCount; }
      if (pads & 0xf00) { tickle_ = Tickle::Right; ++zoneCount; }
      if (zoneCount > 1) tickle_ = Tickle::AllOver;
    }
    if (pads_ && !pads) touchAt_ = now;
    pads_ = pads;
    if (motion) {
      const float magnitude = std::sqrt(motion->x * motion->x + motion->y * motion->y +
                                        motion->z * motion->z);
      const float shock = std::fabs(magnitude - 1.0f);
      const bool shaking = shock > 0.43f || (shock > 0.23f && motion->spin > 210.0f);
      if (shaking && (!shook_ || now - shakeAt_ >= 1200)) {
        shakeAt_ = now;
        shook_ = true;
      }
      if (shock < 0.22f) {
        // The breakout's Y axis follows screen X in the reference mount.
        gazeX_ += 0.17f * (motion->y - gazeX_);
        gazeY_ += 0.17f * (-motion->x - gazeY_);
      }
    } else {
      gazeX_ *= 0.9f;
      gazeY_ *= 0.9f;
    }

    if (shook_ && now - shakeAt_ < 1200 && touchActive(now)) {
      if (!giggled_ || now - giggleAt_ >= 1000) {
        giggled_ = true;
        giggleAt_ = now;
      }
    }
  }

  Mood mood(uint32_t now) const {
    if (giggled_ && now - giggleAt_ < 1000) return Mood::Giggle;
    if (shook_ && now - shakeAt_ < 1200) {
      return now - shakeAt_ < 300 ? Mood::Startled : Mood::Dizzy;
    }
    return touchActive(now) ? Mood::Smile : Mood::Idle;
  }

  Tickle tickle() const { return tickle_; }

  int gazeX() const { return int(std::round(gazeX_ * 11)); }
  int gazeY() const { return int(std::round(gazeY_ * 8)); }

 private:
  bool touchActive(uint32_t now) const {
    return pads_ || (touched_ && now - touchAt_ < 700);
  }

  uint16_t pads_ = 0;
  Tickle tickle_ = Tickle::Middle;
  bool touched_ = false;
  bool shook_ = false;
  bool giggled_ = false;
  uint32_t touchAt_ = 0;
  uint32_t shakeAt_ = 0;
  uint32_t giggleAt_ = 0;
  float gazeX_ = 0;
  float gazeY_ = 0;
};
