// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

namespace opentoy {
namespace camera_eye {

enum class Grade {
  Pass,
  Fail,
  Inconclusive,
};

enum class ClaimKind {
  StaticVisual,
  MotionAnimation,
  TouchPhysical,
  AudioAcoustic,
  ElectricalCharging,
};

struct CaptureEvidence {
  ClaimKind claim = ClaimKind::StaticVisual;
  bool has_provenance = false;
  bool live_hardware = false;
  bool simulated = false;
  bool claimed_live = true;
  bool sequence_advancing = false;
  uint32_t frame_age_ms = 0;
  uint32_t max_fresh_age_ms = 250;
  uint32_t frame_count = 0;
  uint32_t distinct_frame_hashes = 0;
  std::string intended_version;
  std::string observed_version;
  bool clipped_bounds = false;
  bool blank_or_saturated = false;
  bool inverted_or_corrupt = false;
  bool excessive_glare_or_blur = false;
};

struct EvalResult {
  Grade grade = Grade::Inconclusive;
  const char* reason = "unverified";
};

inline const char* gradeName(Grade g) {
  switch (g) {
    case Grade::Pass:
      return "PASS";
    case Grade::Fail:
      return "FAIL";
    case Grade::Inconclusive:
      return "INCONCLUSIVE";
  }
  return "INCONCLUSIVE";
}

inline EvalResult evaluateEvidence(const CaptureEvidence& ev) {
  if (ev.claim == ClaimKind::TouchPhysical ||
      ev.claim == ClaimKind::AudioAcoustic ||
      ev.claim == ClaimKind::ElectricalCharging) {
    return {Grade::Inconclusive,
            "pixels cannot establish touch, audio, electrical, or charging claims"};
  }

  if (ev.simulated) {
    if (ev.claimed_live) {
      return {Grade::Fail, "simulated frame presented as live hardware evidence"};
    }
    return {Grade::Inconclusive, "labelled simulation cannot pass live hardware checks"};
  }

  if (!ev.has_provenance) {
    return {Grade::Inconclusive, "missing capture provenance metadata"};
  }
  if (!ev.live_hardware) {
    return {Grade::Inconclusive, "frame lacks live_hardware flag"};
  }
  if (!ev.sequence_advancing) {
    return {Grade::Inconclusive, "frame sequence did not advance"};
  }
  if (ev.frame_age_ms > ev.max_fresh_age_ms) {
    return {Grade::Inconclusive, "stale frame exceeds freshness window"};
  }

  if (!ev.intended_version.empty() && ev.observed_version.empty()) {
    return {Grade::Inconclusive, "observed firmware version unavailable"};
  }
  if (!ev.intended_version.empty() && !ev.observed_version.empty() &&
      ev.intended_version != ev.observed_version) {
    return {Grade::Fail, "observed firmware version mismatches intended version"};
  }

  if (ev.clipped_bounds || ev.blank_or_saturated || ev.inverted_or_corrupt) {
    return {Grade::Fail,
            "visual defect detected (clipped bounds, blank/saturated, or inverted/corrupt)"};
  }
  if (ev.excessive_glare_or_blur) {
    return {Grade::Inconclusive, "excessive glare or blur obscures target"};
  }

  if (ev.claim == ClaimKind::MotionAnimation) {
    if (ev.frame_count < 3u) {
      return {Grade::Inconclusive,
              "single still or fewer than 3 frames cannot prove motion or smoothness"};
    }
    if (ev.distinct_frame_hashes < 2u) {
      return {Grade::Fail, "frozen sequence: identical frames across bounded clip"};
    }
    return {Grade::Pass, "verified fresh live bounded animation clip"};
  }

  if (ev.frame_count < 1u) {
    return {Grade::Inconclusive, "no valid frame available"};
  }
  return {Grade::Pass, "verified fresh live visual still"};
}

}  // namespace camera_eye
}  // namespace opentoy
