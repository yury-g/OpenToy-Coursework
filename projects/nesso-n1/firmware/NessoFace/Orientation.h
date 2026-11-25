#pragma once

#include <cmath>
#include <cstdint>

class Orientation {
 public:
  void sample(uint32_t now, float x, float y, float z, float spin) {
    const float xy = x * x + y * y;
    const float magnitude = xy + z * z;
    const float ax = std::fabs(x);
    const float ay = std::fabs(y);

    // Wait for a steady pose with a clear up direction in the screen plane.
    if (magnitude < 0.72f || magnitude > 1.30f || xy < 0.38f || spin > 70 ||
        std::fabs(ax - ay) < 0.20f) {
      candidate_ = rotation_;
      return;
    }

    // IMU X and Y are in the screen plane. This mapping assumes the
    // breakout's Y axis runs along the long edge of the Feather screen.
    const uint8_t next = ax > ay ? (x > 0 ? 1 : 3) : (y > 0 ? 2 : 0);
    if (next != candidate_) {
      candidate_ = next;
      candidateSince_ = now;
    }
    if (now - candidateSince_ >= 260) rotation_ = candidate_;
  }

  uint8_t rotation() const { return rotation_; }

 private:
  uint8_t rotation_ = 1;
  uint8_t candidate_ = 1;
  uint32_t candidateSince_ = 0;
};
