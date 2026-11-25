// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace opentoy_sound {

// Shared local acoustic wake & expressive listening state machine (ROADMAP.md "Sound wakes the
// character and shapes its expressions").
//
// Guarantees:
// - Processes short volatile PCM blocks in RAM only; zero recording, storage, or upload.
// - Never infers sound direction, speaker identity, speech content, or human emotion from loudness.
// - Wakes character from quiet visible idle (never blanks screen) when sound rises meaningfully
//   above adaptive ambient background, with sensitivity, onset debounce, and cooldown.
// - Suppresses microphone detection during the device's own speaker output plus a post-playback
//   ringdown holdoff so quiet replies never self-trigger.
// - Yields priority to direct touch pokes and Copy-Face camera gaze/mouth tracking.

enum class Sensitivity : uint8_t {
  Off = 0,
  Low = 1,
  Normal = 2,
  High = 3,
};

struct PcmBlockSummary {
  float rms = 0.0f;       // Normalized [0, 1] RMS amplitude
  float peak = 0.0f;      // Normalized [0, 1] peak amplitude
  bool clipped = false;   // Near full-scale saturation
  bool valid = false;     // Non-empty, non-stuck/non-all-zero valid stream
};

inline PcmBlockSummary analyzePcm16Mono(const int16_t* samples, size_t count) {
  PcmBlockSummary out;
  if (!samples || count < 16) return out;

  double sum_sq = 0.0;
  int32_t max_abs = 0;
  int16_t first = samples[0];
  bool any_diff = false;

  for (size_t i = 0; i < count; ++i) {
    const int32_t v = int32_t(samples[i]);
    if (samples[i] != first) any_diff = true;
    const int32_t a = v < 0 ? -v : v;
    if (a > max_abs) max_abs = a;
    const double norm = double(v) / 32768.0;
    sum_sq += norm * norm;
  }

  // An all-constant rail (e.g. disconnected I2S DIN stuck at 0 or -1) is not valid acoustic input.
  if (!any_diff) {
    out.valid = false;
    return out;
  }

  out.rms = float(std::sqrt(sum_sq / double(count)));
  out.peak = std::min(1.0f, float(max_abs) / 32767.0f);
  out.clipped = (max_abs >= 32400);
  out.valid = std::isfinite(out.rms) && std::isfinite(out.peak);
  return out;
}

struct ListeningExpression {
  bool awake = false;           // Meaningfully alert above quiet idle
  bool suppressed = false;      // Muted by self-speaker holdoff or disabled sensitivity
  float alertness = 0.0f;       // [0, 1] smooth wake envelope
  float surprise = 0.0f;        // [0, 1] transient sudden-onset lift
  float noise_floor = 0.0f;     // Estimated ambient RMS [0, 1]
  int lid_open_boost = 0;       // [0, 24] upward lid lift reduction on upper-lid closure
  int left_perk = 0;            // Asymmetric lid offset for Cheeky persona
  int right_perk = 0;
  int gaze_stabilize_pct = 0;   // [0, 100] pulls wandering idle gaze gently toward attentive center
};

class SoundWakeMonitor {
  Sensitivity sensitivity_ = Sensitivity::Off;
  float noise_floor_ = 0.008f;
  float alertness_ = 0.0f;
  float surprise_ = 0.0f;
  float prev_rms_ = 0.0f;
  uint32_t last_ms_ = 0;
  uint32_t alert_until_ms_ = 0;
  uint32_t suppress_until_ms_ = 0;
  unsigned active_streak_ = 0;
  bool initialized_ = false;
  bool awake_ = false;

  static float thresholdMultiplier(Sensitivity s) {
    switch (s) {
      case Sensitivity::Low: return 3.6f;
      case Sensitivity::Normal: return 2.5f;
      case Sensitivity::High: return 1.8f;
      default: return 999.0f;
    }
  }

  static float minDeltaAboveFloor(Sensitivity s) {
    switch (s) {
      case Sensitivity::Low: return 0.030f;
      case Sensitivity::Normal: return 0.016f;
      case Sensitivity::High: return 0.009f;
      default: return 1.0f;
    }
  }

 public:
  uint32_t speaker_cooldown_ms = 280;
  uint32_t alert_hold_ms = 900;
  unsigned debounce_blocks = 2;

  void setSensitivity(Sensitivity s) {
    sensitivity_ = s;
    if (s == Sensitivity::Off) {
      awake_ = false;
      alertness_ = 0.0f;
      surprise_ = 0.0f;
      active_streak_ = 0;
    }
  }

  Sensitivity sensitivity() const { return sensitivity_; }

  void reset() {
    noise_floor_ = 0.008f;
    alertness_ = 0.0f;
    surprise_ = 0.0f;
    prev_rms_ = 0.0f;
    active_streak_ = 0;
    initialized_ = false;
    awake_ = false;
  }

  ListeningExpression update(const PcmBlockSummary& block,
                             uint32_t now_ms,
                             unsigned persona_id,
                             bool speaker_active,
                             bool direct_poke_active = false,
                             bool camera_tracking_active = false) {
    ListeningExpression out;
    const uint32_t dt_ms = initialized_ ? std::min<uint32_t>(now_ms - last_ms_, 250u) : 20u;
    initialized_ = true;
    last_ms_ = now_ms;

    if (speaker_active) {
      suppress_until_ms_ = now_ms + speaker_cooldown_ms;
    }
    const bool in_speaker_holdoff = speaker_active || (int32_t(suppress_until_ms_ - now_ms) > 0);

    // Personality timing factors: Curious=1 (nimble), Dreamy=2 (slow/gentle), Cheeky=3 (playful).
    const float attack_tau_ms = (persona_id == 1) ? 55.0f : (persona_id == 2) ? 140.0f : 80.0f;
    const float release_tau_ms = (persona_id == 1) ? 380.0f : (persona_id == 2) ? 720.0f : 480.0f;

    if (sensitivity_ == Sensitivity::Off || in_speaker_holdoff || !block.valid) {
      active_streak_ = 0;
      const float decay = std::exp(-float(dt_ms) / release_tau_ms);
      alertness_ *= decay;
      surprise_ *= std::exp(-float(dt_ms) / 140.0f);
      if (alertness_ < 0.03f) {
        alertness_ = 0.0f;
        awake_ = false;
      }
      out.suppressed = (sensitivity_ == Sensitivity::Off) || in_speaker_holdoff;
      out.awake = awake_;
      out.alertness = alertness_;
      out.surprise = surprise_;
      out.noise_floor = noise_floor_;
      return out;
    }

    const float rms = std::max(0.0f, std::min(1.0f, block.rms));
    const float trig_level = std::max(
        noise_floor_ * thresholdMultiplier(sensitivity_),
        noise_floor_ + minDeltaAboveFloor(sensitivity_));

    const bool above = (rms >= trig_level);
    if (!above) {
      // Adapt ambient floor: quicker downward follow, cautious upward creep during quiet.
      const float alpha = (rms < noise_floor_) ? 0.12f : 0.02f;
      noise_floor_ = std::max(0.002f, std::min(0.25f, noise_floor_ + alpha * (rms - noise_floor_)));
      active_streak_ = 0;
    } else {
      ++active_streak_;
    }

    // Sudden onset detection before debounce clamping for crisp surprise when woken.
    const float jump = rms - std::max(prev_rms_, trig_level);
    prev_rms_ = rms;

    if (active_streak_ >= debounce_blocks) {
      awake_ = true;
      alert_until_ms_ = now_ms + alert_hold_ms;
      if (jump > 0.035f) {
        const float persona_surprise = (persona_id == 2) ? 0.65f : 1.0f;
        surprise_ = std::min(1.0f, std::max(surprise_, (jump / 0.18f) * persona_surprise));
      }
    } else if (awake_ && int32_t(alert_until_ms_ - now_ms) <= 0) {
      awake_ = false;
    }

    const float raw_intensity = above
        ? std::min(1.0f, (rms - noise_floor_) / std::max(0.04f, 4.0f * minDeltaAboveFloor(sensitivity_)))
        : 0.0f;
    const float target_alert = awake_ ? std::max(0.35f, raw_intensity) : 0.0f;
    const float tau = (target_alert > alertness_) ? attack_tau_ms : release_tau_ms;
    const float step = 1.0f - std::exp(-float(dt_ms) / tau);
    alertness_ += (target_alert - alertness_) * step;
    surprise_ *= std::exp(-float(dt_ms) / 160.0f);
    if (!awake_ && alertness_ < 0.02f) alertness_ = 0.0f;

    out.awake = awake_;
    out.suppressed = false;
    out.alertness = alertness_;
    out.surprise = surprise_;
    out.noise_floor = noise_floor_;

    // Direct touch pokes own the expression completely; Copy-Face camera owns gaze & mouth.
    if (direct_poke_active) {
      return out;
    }

    out.lid_open_boost = int(std::round((alertness_ * 14.0f) + (surprise_ * 10.0f)));
    if (persona_id == 3 && (awake_ || alertness_ > 0.20f)) {
      // Cheeky asymmetric attentive perk.
      out.left_perk = -2;
      out.right_perk = 3;
    }
    if (!camera_tracking_active) {
      out.gaze_stabilize_pct = int(std::round(std::min(1.0f, alertness_ * 0.85f) * 100.0f));
    }
    return out;
  }
};

}  // namespace opentoy_sound
