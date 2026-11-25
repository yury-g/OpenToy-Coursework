// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>

namespace flipcam {

constexpr const char* kFwVersion = "1.0.0";
constexpr const char* kMediaPartitionLabel = "media";

enum class AppMode : uint8_t {
  LIVE = 0,
  GALLERY = 1,
};

enum class LookMode : uint8_t {
  DITHER = 0,
  THRESHOLD = 1,
};

enum class ContrastLevel : uint8_t {
  C_LOW = 0,
  C_NORMAL = 1,
  C_HIGH = 2,
};

enum class OrientationMode : uint8_t {
  SELFIE = 0,
  REAR = 1,
};

enum class MediaType : uint8_t {
  STILL = 1,
  CLIP = 2,
};

enum class LinkState : uint8_t {
  DISCONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2,
  STALE = 3,
};

struct FlipCamSettings {
  uint32_t magic = 0x46435331u;  // 'FCS1'
  uint16_t version = 1;
  uint8_t look = static_cast<uint8_t>(LookMode::DITHER);
  uint8_t contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
  uint8_t mirror = 0;   // 0=off, 1=on
  uint8_t sound = 1;    // 1=on by default for real audio playback
  uint8_t orientation = static_cast<uint8_t>(OrientationMode::SELFIE);  // 0=selfie, 1=rear (was reserved[0])
  uint8_t reserved = 0;
  uint32_t crc32 = 0;

  bool operator==(const FlipCamSettings& o) const {
    return look == o.look && contrast == o.contrast && mirror == o.mirror &&
           sound == o.sound && orientation == o.orientation;
  }
  bool operator!=(const FlipCamSettings& o) const { return !(*this == o); }
};
static_assert(sizeof(FlipCamSettings) == 16, "FlipCamSettings must remain 16 bytes");

struct MediaEntry {
  uint32_t id = 0;
  uint8_t type = static_cast<uint8_t>(MediaType::STILL);
  uint8_t look = 0;
  uint8_t contrast = 1;
  uint8_t mirror = 0;
  uint8_t synthetic = 0;
  uint8_t hasAudio = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint32_t sizeBytes = 0;
  uint32_t durationMs = 0;
  uint32_t crc32 = 0;
  uint32_t createdSeq = 0;
  char filename[32] = {};
  char title[24] = {};
};

inline const char* modeName(AppMode m) {
  switch (m) {
    case AppMode::LIVE: return "live";
    case AppMode::GALLERY: return "gallery";
  }
  return "live";
}

inline const char* lookName(uint8_t look) {
  return look == static_cast<uint8_t>(LookMode::THRESHOLD) ? "threshold" : "dither";
}

inline const char* contrastName(uint8_t c) {
  switch (static_cast<ContrastLevel>(c)) {
    case ContrastLevel::C_LOW: return "low";
    case ContrastLevel::C_HIGH: return "high";
    default: return "normal";
  }
}

inline const char* orientationName(uint8_t o) {
  return o == static_cast<uint8_t>(OrientationMode::REAR) ? "rear" : "selfie";
}

inline const char* linkStateName(LinkState s) {
  switch (s) {
    case LinkState::DISCONNECTED: return "link_lost";
    case LinkState::CONNECTING: return "reconnecting";
    case LinkState::CONNECTED: return "connected";
    case LinkState::STALE: return "stale";
  }
  return "link_lost";
}

}  // namespace flipcam
