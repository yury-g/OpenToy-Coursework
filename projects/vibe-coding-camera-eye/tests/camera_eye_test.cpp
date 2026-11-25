// SPDX-License-Identifier: MIT
#include <cassert>
#include <iostream>
#include <string>

#include "../core/EvidenceGrade.h"

using namespace opentoy::camera_eye;

namespace {

CaptureEvidence makeGoodStill() {
  CaptureEvidence ev;
  ev.claim = ClaimKind::StaticVisual;
  ev.has_provenance = true;
  ev.live_hardware = true;
  ev.simulated = false;
  ev.sequence_advancing = true;
  ev.frame_age_ms = 40;
  ev.frame_count = 1;
  ev.distinct_frame_hashes = 1;
  ev.intended_version = "1.1.0";
  ev.observed_version = "1.1.0";
  return ev;
}

CaptureEvidence makeGoodClip() {
  CaptureEvidence ev = makeGoodStill();
  ev.claim = ClaimKind::MotionAnimation;
  ev.frame_count = 10;
  ev.distinct_frame_hashes = 6;
  return ev;
}

}  // namespace

int main() {
  // 1. Valid still and bounded motion clip pass.
  assert(evaluateEvidence(makeGoodStill()).grade == Grade::Pass);
  assert(evaluateEvidence(makeGoodClip()).grade == Grade::Pass);

  // 2. Single still cannot prove motion/smoothness -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodStill();
    ev.claim = ClaimKind::MotionAnimation;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  // 3. Two frames still insufficient for motion claim -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodClip();
    ev.frame_count = 2;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  // 4. Frozen burst across >=3 frames -> FAIL.
  {
    CaptureEvidence ev = makeGoodClip();
    ev.distinct_frame_hashes = 1;
    assert(evaluateEvidence(ev).grade == Grade::Fail);
  }

  // 5. Missing provenance, non-live, non-advancing sequence, or stale -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodStill();
    ev.has_provenance = false;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);

    ev = makeGoodStill();
    ev.live_hardware = false;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);

    ev = makeGoodStill();
    ev.sequence_advancing = false;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);

    ev = makeGoodStill();
    ev.frame_age_ms = 251;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  // 6. Simulated frame claimed live -> FAIL; labelled simulation -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodStill();
    ev.simulated = true;
    ev.claimed_live = true;
    assert(evaluateEvidence(ev).grade == Grade::Fail);

    ev.claimed_live = false;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  // 7. Intended vs observed version mismatch -> FAIL; missing observed -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodStill();
    ev.intended_version = "1.1.1";
    ev.observed_version = "1.1.0";
    assert(evaluateEvidence(ev).grade == Grade::Fail);

    ev.observed_version = "";
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  // 8. Visual defects -> FAIL; glare/blur -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodStill();
    ev.clipped_bounds = true;
    assert(evaluateEvidence(ev).grade == Grade::Fail);

    ev = makeGoodStill();
    ev.blank_or_saturated = true;
    assert(evaluateEvidence(ev).grade == Grade::Fail);

    ev = makeGoodStill();
    ev.inverted_or_corrupt = true;
    assert(evaluateEvidence(ev).grade == Grade::Fail);

    ev = makeGoodStill();
    ev.excessive_glare_or_blur = true;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  // 9. Pixels never prove touch, audio, electrical, or charging -> INCONCLUSIVE.
  {
    CaptureEvidence ev = makeGoodStill();
    ev.claim = ClaimKind::ElectricalCharging;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);

    ev.claim = ClaimKind::TouchPhysical;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);

    ev.claim = ClaimKind::AudioAcoustic;
    assert(evaluateEvidence(ev).grade == Grade::Inconclusive);
  }

  std::cout << "PASS Vibe Coding Camera Eye honesty & grading unit tests\n";
  return 0;
}
