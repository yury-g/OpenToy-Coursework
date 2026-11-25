// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace flipcam {

enum class ButtonId : uint8_t {
  SNAP = 0,
  BROWSE = 1,
};

struct ButtonEvents {
  bool snapTap = false;
  bool snapHold = false;
  bool browseTap = false;
  bool browseHold = false;  // Only ever emitted internally if enabled; Browse hold is disabled per STEERING-BROWSE.md
};

struct ButtonChannel {
  bool rawPressed = false;
  bool debouncedPressed = false;
  bool active = false;
  bool holdFired = false;
  bool bootSuppressed = false;
  bool stuck = false;
  bool isInjected = false;
  bool allowHoldEvent = true;

  uint32_t lastRawChangeMs = 0;
  uint32_t pressStartMs = 0;
  float holdProgress = 0.0f;

  // Diagnostics: strictly separate physical vs injected counters
  uint32_t physPressCount = 0;
  uint32_t physTapCount = 0;
  uint32_t physHoldCount = 0;
  uint32_t injPressCount = 0;
  uint32_t injTapCount = 0;
  uint32_t injHoldCount = 0;
  uint32_t bounceSuppressedCount = 0;
};

class ButtonController {
 public:
  static constexpr uint32_t kDebounceMs = 25;
  static constexpr uint32_t kHoldMs = 1000;
  static constexpr uint32_t kStuckMs = 15000;

  void init(bool snapHeldAtBoot, bool browseHeldAtBoot, uint32_t nowMs,
            bool allowSnapHold = false, bool allowBrowseHold = false) {
    // Hold capability is configured explicitly per board/channel so shared ButtonController
    // (e.g., XIAO D1) never gains an unintended long-hold action unless opted in.
    resetChannel(snap_, allowSnapHold, snapHeldAtBoot, nowMs);
    resetChannel(browse_, allowBrowseHold, browseHeldAtBoot, nowMs);
  }

  void setTestMode(bool enabled, uint32_t nowMs) {
    if (testMode_ && !enabled) {
      cancelInjected(nowMs);
    }
    testMode_ = enabled;
  }

  bool isTestMode() const { return testMode_; }

  bool injectDown(ButtonId id, uint32_t nowMs) {
    if (!testMode_) return false;
    ButtonChannel& ch = channel(id);
    if (ch.active || ch.bootSuppressed) return false;
    ch.active = true;
    ch.isInjected = true;
    ch.holdFired = false;
    ch.stuck = false;
    ch.pressStartMs = nowMs;
    ch.holdProgress = 0.0f;
    ch.injPressCount++;
    return true;
  }

  bool injectUp(ButtonId id, uint32_t nowMs, ButtonEvents& outEvents) {
    if (!testMode_) return false;
    ButtonChannel& ch = channel(id);
    if (!ch.active || !ch.isInjected) return false;
    const uint32_t elapsed = (nowMs >= ch.pressStartMs) ? (nowMs - ch.pressStartMs) : 0;
    if (elapsed >= kStuckMs) {
      ch.stuck = true;
    } else if (ch.allowHoldEvent && !ch.holdFired && !ch.stuck && elapsed >= kHoldMs) {
      ch.holdFired = true;
      ch.injHoldCount++;
      emitHold(id, outEvents);
    }
    const bool suppressed = ch.holdFired || ch.stuck;
    ch.active = false;
    ch.isInjected = false;
    ch.holdProgress = 0.0f;
    ch.holdFired = false;
    ch.stuck = false;
    if (!suppressed) {
      ch.injTapCount++;
      emitTap(id, outEvents);
    }
    return true;
  }

  bool injectCancel(ButtonId id) {
    if (!testMode_) return false;
    ButtonChannel& ch = channel(id);
    if (!ch.active || !ch.isInjected) return false;
    ch.active = false;
    ch.isInjected = false;
    ch.holdProgress = 0.0f;
    ch.holdFired = false;
    ch.stuck = false;
    return true;
  }

  void cancelInjected(uint32_t nowMs) {
    (void)nowMs;
    if (snap_.isInjected) {
      snap_.active = false;
      snap_.isInjected = false;
      snap_.holdProgress = 0.0f;
      snap_.holdFired = false;
      snap_.stuck = false;
    }
    if (browse_.isInjected) {
      browse_.active = false;
      browse_.isInjected = false;
      browse_.holdProgress = 0.0f;
      browse_.holdFired = false;
      browse_.stuck = false;
    }
  }

  ButtonEvents update(bool snapRawPressed, bool browseRawPressed, uint32_t nowMs) {
    ButtonEvents ev{};
    updateChannel(ButtonId::SNAP, snap_, snapRawPressed, nowMs, ev);
    updateChannel(ButtonId::BROWSE, browse_, browseRawPressed, nowMs, ev);
    return ev;
  }

  const ButtonChannel& snap() const { return snap_; }
  const ButtonChannel& browse() const { return browse_; }

 private:
  ButtonChannel snap_{};
  ButtonChannel browse_{};
  bool testMode_ = false;

  ButtonChannel& channel(ButtonId id) {
    return id == ButtonId::SNAP ? snap_ : browse_;
  }

  static void resetChannel(ButtonChannel& ch, bool allowHold, bool heldAtBoot, uint32_t nowMs) {
    ch = ButtonChannel{};
    ch.allowHoldEvent = allowHold;
    ch.rawPressed = heldAtBoot;
    ch.debouncedPressed = heldAtBoot;
    ch.bootSuppressed = heldAtBoot;
    ch.lastRawChangeMs = nowMs;
  }

  static void emitTap(ButtonId id, ButtonEvents& ev) {
    if (id == ButtonId::SNAP) ev.snapTap = true;
    else ev.browseTap = true;
  }

  static void emitHold(ButtonId id, ButtonEvents& ev) {
    if (id == ButtonId::SNAP) ev.snapHold = true;
    else ev.browseHold = true;
  }

  static void updateChannel(ButtonId id, ButtonChannel& ch, bool rawPressed, uint32_t nowMs,
                            ButtonEvents& ev) {
    if (rawPressed != ch.rawPressed) {
      if (ch.rawPressed != ch.debouncedPressed && (nowMs - ch.lastRawChangeMs) < kDebounceMs) {
        ch.bounceSuppressedCount++;
      }
      ch.rawPressed = rawPressed;
      ch.lastRawChangeMs = nowMs;
    }

    // Physical debounce transition
    if ((nowMs - ch.lastRawChangeMs) >= kDebounceMs && ch.debouncedPressed != ch.rawPressed) {
      ch.debouncedPressed = ch.rawPressed;

      if (ch.bootSuppressed) {
        if (!ch.debouncedPressed) {
          // Released cleanly after being held at boot
          ch.bootSuppressed = false;
        }
      } else if (!ch.isInjected) {
        if (ch.debouncedPressed) {
          ch.active = true;
          ch.holdFired = false;
          ch.stuck = false;
          ch.pressStartMs = nowMs;
          ch.holdProgress = 0.0f;
          ch.physPressCount++;
        } else if (ch.active) {
          const bool suppressed = ch.holdFired || ch.stuck;
          ch.active = false;
          ch.holdProgress = 0.0f;
          ch.holdFired = false;
          ch.stuck = false;
          if (!suppressed) {
            ch.physTapCount++;
            emitTap(id, ev);
          }
        }
      }
    }

    // Active hold progression (physical or injected)
    if (ch.active && !ch.bootSuppressed) {
      const uint32_t elapsed = (nowMs >= ch.pressStartMs) ? (nowMs - ch.pressStartMs) : 0;
      if (elapsed >= kStuckMs) {
        ch.stuck = true;
      }
      if (ch.allowHoldEvent && !ch.stuck) {
        const float p = static_cast<float>(elapsed) / static_cast<float>(kHoldMs);
        ch.holdProgress = p > 1.0f ? 1.0f : p;
        if (elapsed >= kHoldMs && !ch.holdFired) {
          ch.holdFired = true;
          if (ch.isInjected) ch.injHoldCount++;
          else ch.physHoldCount++;
          emitHold(id, ev);
        }
      } else {
        // Non-hold button: immediate active highlight, NO hold progress bar or hold action
        ch.holdProgress = 0.0f;
      }
    }
  }
};

}  // namespace flipcam
