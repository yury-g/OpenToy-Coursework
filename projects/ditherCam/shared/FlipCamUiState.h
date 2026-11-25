// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "FlipCamButtons.h"
#include "FlipCamTypes.h"

namespace flipcam {

enum class UiCommand : uint8_t {
  NONE = 0,
  CAPTURE_STILL,
  REC_START,
  REC_STOP,
  PLAY_CLIP,
  STOP_PLAYBACK,
};

struct UiContextHints {
  const char* snapTap = "STILL";
  const char* snapHold = "REC";
  const char* browseTap = "PREV";
};

class FlipCamUiState {
 public:
  static constexpr uint32_t kMaxRecMs = 10000;
  static constexpr uint32_t kToastDefaultMs = 1800;
  static constexpr uint32_t kBlinkMs = 130;

  AppMode mode = AppMode::LIVE;
  LinkState linkState = LinkState::DISCONNECTED;
  bool storageMounted = false;
  bool storageFull = false;
  bool captureBusy = false;
  bool recording = false;
  bool playing = false;
  bool codecFault = false;
  bool micFault = false;
  bool cameraFault = false;
  bool oledFault = false;

  uint32_t recStartMs = 0;
  uint32_t recElapsedMs = 0;
  uint32_t playingClipId = 0;
  uint32_t playElapsedMs = 0;
  uint32_t playDurationMs = 0;

  // Still-only reverse-chronological gallery cursor (0 = newest still)
  int stillIndex = 0;
  int stillCount = 0;

  uint32_t blinkUntilMs = 0;
  uint32_t toastUntilMs = 0;
  char toast[40] = {};

  FlipCamSettings settings{};
  bool settingsDirty = false;

  void showToast(const char* msg, uint32_t nowMs, uint32_t durMs = kToastDefaultMs) {
    if (!msg) return;
    std::snprintf(toast, sizeof(toast), "%s", msg);
    toastUntilMs = nowMs + durMs;
  }

  void setStillCount(int count) {
    stillCount = count < 0 ? 0 : count;
    if (stillCount == 0) {
      stillIndex = 0;
      if (mode == AppMode::GALLERY) {
        mode = AppMode::LIVE;
      }
    } else if (stillIndex >= stillCount) {
      stillIndex = stillCount - 1;
    }
  }

  bool canStartCapture() const {
    if (!storageMounted || storageFull || captureBusy) return false;
    if (linkState != LinkState::CONNECTED) return false;
    return true;
  }

  const char* captureBlockReason() const {
    if (!storageMounted) return "STORAGE NOT MOUNTED";
    if (storageFull) return "STORAGE FULL";
    if (captureBusy) return "CAPTURE BUSY";
    if (linkState == LinkState::CONNECTING) return "RECONNECTING: WAIT";
    if (linkState == LinkState::STALE) return "LINK STALE: WAIT";
    if (linkState != LinkState::CONNECTED) return "LINK LOST: BLOCKED";
    return "";
  }

  UiContextHints contextHints() const {
    UiContextHints h{};
    if (mode == AppMode::LIVE) {
      h.snapTap = "STILL";
      h.snapHold = "FLIP";
      h.browseTap = "PREV";
    } else {
      // GALLERY (reverse chronological still browser per STEERING-BROWSE.md)
      h.snapTap = "LIVE";
      h.snapHold = "";
      h.browseTap = (stillCount > 0 && stillIndex + 1 >= stillCount) ? "OLDEST" : "OLDER";
    }
    return h;
  }

  // Processes physical/injected button events and returns at most one requested action
  UiCommand applyButtonEvents(const ButtonEvents& ev, uint32_t nowMs) {
    if (mode == AppMode::LIVE) {
      if (ev.snapHold) {
        if (captureBusy || recording) {
          showToast("BUSY: WAIT", nowMs, 1500);
          return UiCommand::NONE;
        }
        const bool toRear = (settings.orientation != static_cast<uint8_t>(OrientationMode::REAR));
        settings.orientation = toRear ? static_cast<uint8_t>(OrientationMode::REAR)
                                      : static_cast<uint8_t>(OrientationMode::SELFIE);
        settingsDirty = true;
        showToast(toRear ? "MODE: REAR 180" : "MODE: SELFIE", nowMs, 1600);
        return UiCommand::NONE;
      }
      if (ev.snapTap) {
        if (!canStartCapture()) {
          showToast(captureBlockReason(), nowMs, 2000);
        } else {
          blinkUntilMs = nowMs + kBlinkMs;
          return UiCommand::CAPTURE_STILL;
        }
      } else if (ev.browseTap) {
        if (stillCount <= 0) {
          showToast("NO PHOTOS", nowMs, 1500);
        } else {
          mode = AppMode::GALLERY;
          stillIndex = 0;  // Newest still photo
          if (stillCount == 1) {
            showToast("NEWEST / OLDEST", nowMs, 1200);
          }
        }
      }
      return UiCommand::NONE;
    }

    // GALLERY mode: reverse-chronological stills only; hold Snap in Gallery is a deliberate no-op
    // (never rotates stored photos and never triggers capture).
    if (mode == AppMode::GALLERY) {
      if (ev.snapTap) {
        mode = AppMode::LIVE;
        stillIndex = 0;
        return UiCommand::NONE;
      }
      if (ev.browseTap) {
        if (stillCount <= 0) {
          mode = AppMode::LIVE;
          showToast("NO PHOTOS", nowMs, 1400);
        } else if (stillIndex + 1 < stillCount) {
          stillIndex++;
          if (stillIndex + 1 == stillCount) {
            showToast("OLDEST PHOTO", nowMs, 1400);
          }
        } else {
          // Stay at oldest with OLDEST feedback (NO wraparound!)
          showToast("OLDEST", nowMs, 1400);
        }
      }
    }

    return UiCommand::NONE;
  }

  UiCommand tick(uint32_t nowMs) {
    (void)nowMs;
    recording = false;
    playing = false;
    return UiCommand::NONE;
  }

  bool updateSettingsField(const char* key, const char* val) {
    if (!key || !val) return false;
    FlipCamSettings next = settings;
    if (std::strcmp(key, "look") == 0) {
      if (std::strcmp(val, "dither") == 0) next.look = static_cast<uint8_t>(LookMode::DITHER);
      else if (std::strcmp(val, "threshold") == 0) next.look = static_cast<uint8_t>(LookMode::THRESHOLD);
      else return false;
    } else if (std::strcmp(key, "contrast") == 0) {
      if (std::strcmp(val, "low") == 0) next.contrast = static_cast<uint8_t>(ContrastLevel::C_LOW);
      else if (std::strcmp(val, "normal") == 0) next.contrast = static_cast<uint8_t>(ContrastLevel::C_NORMAL);
      else if (std::strcmp(val, "high") == 0) next.contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
      else return false;
    } else if (std::strcmp(key, "mirror") == 0) {
      if (std::strcmp(val, "1") == 0 || std::strcmp(val, "on") == 0 || std::strcmp(val, "true") == 0) next.mirror = 1;
      else if (std::strcmp(val, "0") == 0 || std::strcmp(val, "off") == 0 || std::strcmp(val, "false") == 0) next.mirror = 0;
      else return false;
    } else if (std::strcmp(key, "sound") == 0) {
      if (std::strcmp(val, "1") == 0 || std::strcmp(val, "on") == 0 || std::strcmp(val, "true") == 0) next.sound = 1;
      else if (std::strcmp(val, "0") == 0 || std::strcmp(val, "off") == 0 || std::strcmp(val, "false") == 0) next.sound = 0;
      else return false;
    } else if (std::strcmp(key, "orientation") == 0) {
      if (std::strcmp(val, "selfie") == 0 || std::strcmp(val, "0") == 0) {
        next.orientation = static_cast<uint8_t>(OrientationMode::SELFIE);
      } else if (std::strcmp(val, "rear") == 0 || std::strcmp(val, "rear180") == 0 || std::strcmp(val, "1") == 0) {
        next.orientation = static_cast<uint8_t>(OrientationMode::REAR);
      } else {
        return false;
      }
    } else {
      return false;
    }

    if (next != settings) {
      settings = next;
      settingsDirty = true;
    }
    return true;
  }
};

}  // namespace flipcam
