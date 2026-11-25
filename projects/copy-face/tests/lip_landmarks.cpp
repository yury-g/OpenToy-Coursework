// SPDX-License-Identifier: MIT
#include "../core/LipLandmarkAdapter.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

using namespace copy_face;

namespace {

std::vector<Point> buildSyntheticFaceMesh468(float bend, float gap, float eye_open = 0.08f) {
  std::vector<Point> pts(468, Point{0.5f, 0.5f});

  // Populate 9 upper and 9 lower inner-lip landmarks in ROI [0, 1] space.
  for (unsigned i = 0; i < lipPoints; ++i) {
    const float u = float(i) / float(lipPoints - 1);
    const float arch = 4.0f * u * (1.0f - u);
    const float x = 0.30f + 0.40f * u;
    const float mid_y = 0.68f + bend * arch;
    pts[kFaceMeshInnerUpper9[i]] = Point{x, mid_y - 0.5f * gap * arch};
    pts[kFaceMeshInnerLower9[i]] = Point{x, mid_y + 0.5f * gap * arch};
  }

  // Populate 6-point left and right eye contours.
  auto fillEye = [&](const std::array<uint16_t, 6>& idx, float cx, float cy) {
    pts[idx[0]] = Point{cx - 0.10f, cy};
    pts[idx[1]] = Point{cx - 0.04f, cy - eye_open};
    pts[idx[2]] = Point{cx + 0.04f, cy - eye_open};
    pts[idx[3]] = Point{cx + 0.10f, cy};
    pts[idx[4]] = Point{cx + 0.04f, cy + eye_open};
    pts[idx[5]] = Point{cx - 0.04f, cy + eye_open};
  };
  fillEye(kFaceMeshLeftEye6, 0.34f, 0.36f);
  fillEye(kFaceMeshRightEye6, 0.66f, 0.36f);
  return pts;
}

std::vector<Point> buildSyntheticIbug68(float bend, float gap, float eye_open = 0.07f) {
  std::vector<Point> pts(68, Point{0.5f, 0.5f});
  // Left eye 36..41, right eye 42..47
  auto fillEye = [&](unsigned base, float cx, float cy) {
    pts[base + 0] = Point{cx - 0.09f, cy};
    pts[base + 1] = Point{cx - 0.03f, cy - eye_open};
    pts[base + 2] = Point{cx + 0.03f, cy - eye_open};
    pts[base + 3] = Point{cx + 0.09f, cy};
    pts[base + 4] = Point{cx + 0.03f, cy + eye_open};
    pts[base + 5] = Point{cx - 0.03f, cy + eye_open};
  };
  fillEye(36, 0.35f, 0.36f);
  fillEye(42, 0.65f, 0.36f);

  // Inner lip 60..67
  const float xs[5] = {0.30f, 0.40f, 0.50f, 0.60f, 0.70f};
  auto archAt = [](float u) { return 4.0f * u * (1.0f - u); };
  pts[60] = Point{xs[0], 0.68f};
  pts[64] = Point{xs[4], 0.68f};
  for (unsigned k = 1; k <= 3; ++k) {
    const float u = 0.25f * float(k);
    const float a = archAt(u);
    const float mid = 0.68f + bend * a;
    pts[60 + k] = Point{xs[k], mid - 0.5f * gap * a};
  }
  // 65 (right-inner lower), 66 (center lower), 67 (left-inner lower)
  pts[65] = Point{xs[3], (0.68f + bend * archAt(0.75f)) + 0.5f * gap * archAt(0.75f)};
  pts[66] = Point{xs[2], (0.68f + bend * archAt(0.50f)) + 0.5f * gap * archAt(0.50f)};
  pts[67] = Point{xs[1], (0.68f + bend * archAt(0.25f)) + 0.5f * gap * archAt(0.25f)};
  return pts;
}

}  // namespace

int main() {
  FaceRoiTransform roi;
  roi.crop_x = 0.25f;
  roi.crop_y = 0.15f;
  roi.crop_w = 0.50f;
  roi.crop_h = 0.70f;
  roi.image_aspect = 16.0f / 9.0f;

  // 1. Neutral closed-mouth FaceMesh 468: full inner-lip contact, aperture == 0.
  {
    auto mesh = buildSyntheticFaceMesh468(0.0f, 0.0f, 0.08f);
    Detection det;
    det.x = roi.crop_x;
    det.y = roi.crop_y;
    det.w = roi.crop_w;
    det.h = roi.crop_h;
    det.confidence = 0.92f;
    assert(adaptFaceMesh468(mesh.data(), mesh.size(), 0.90f, roi, det));
    assert(det.mouthValid);
    assert(det.lips.valid);
    assert(det.aperture == 0.0f);
    assert(std::fabs(det.bend) < 0.05f);
    const LipShape shape = normalizeLips(det.lips);
    assert(shape.valid);
    assert(expression(shape) == Expression::Neutral);
    for (unsigned i = 0; i < lipPoints; ++i) {
      assert(contact(shape, i));
    }
    assert(validEye(det.eyes[0]) && validEye(det.eyes[1]));
    assert(std::string(eyeActivity(det.eyes[0])) == "open");
  }

  // 2. Open mouth + closed blink on FaceMesh 468: separated center lips, corner contact preserved.
  {
    auto mesh = buildSyntheticFaceMesh468(0.0f, 0.18f, 0.005f);
    Detection det;
    assert(adaptFaceMesh468(mesh.data(), mesh.size(), 0.88f, roi, det));
    assert(det.mouthValid);
    assert(det.aperture > 0.20f);
    const LipShape shape = normalizeLips(det.lips);
    assert(shape.valid);
    assert(expression(shape) == Expression::Open);
    assert(contact(shape, 0) && contact(shape, lipPoints - 1));
    assert(!contact(shape, 4));
    assert(std::string(eyeActivity(det.eyes[0])) == "closed");
  }

  // 3. Smile and Frown on 68-point iBUG/PFLD adapter + Tracker mirror integration.
  {
    auto smile68 = buildSyntheticIbug68(0.08f, 0.0f);
    Detection d_smile;
    d_smile.x = roi.crop_x;
    d_smile.y = roi.crop_y;
    d_smile.w = roi.crop_w;
    d_smile.h = roi.crop_h;
    d_smile.confidence = 0.95f;
    assert(adaptIbug68(smile68.data(), smile68.size(), 0.85f, roi, d_smile));
    assert(d_smile.mouthValid && d_smile.bend > 0.25f);
    assert(expression(normalizeLips(d_smile.lips)) == Expression::Smile);

    auto frown68 = buildSyntheticIbug68(-0.08f, 0.0f);
    Detection d_frown;
    assert(adaptIbug68(frown68.data(), frown68.size(), 0.85f, roi, d_frown));
    assert(d_frown.mouthValid && d_frown.bend < -0.25f);
    assert(expression(normalizeLips(d_frown.lips)) == Expression::Frown);

    // Feed into Tracker across 3 frames and verify smooth mirrored Pose.
    Tracker tracker;
    for (uint32_t t = 100; t <= 160; t += 30) {
      Frame f;
      f.captured = t;
      f.count = 1;
      f.faces[0] = d_smile;
      tracker.observe(f, t);
    }
    Pose p = tracker.step(160, true);
    assert(p.tracking && p.lips.valid);
    for (int step = 0; step < 8; ++step) {
      p = tracker.step(160 + uint32_t(step + 1) * 40, true);
    }
    assert(p.bend > 0.15f);
  }

  // 4. Strict failure & honesty gates: 5-point-only, low confidence, inverted lips, NaN, out-of-frame.
  {
    auto mesh = buildSyntheticFaceMesh468(0.06f, 0.10f);
    Detection det;
    assert(adaptFaceMesh468(mesh.data(), mesh.size(), 0.95f, roi, det));
    assert(det.mouthValid);

    // 5-point detector path MUST clear dense mouth/eyes.
    applyFivePointOnly(det);
    assert(!det.mouthValid && !det.lips.valid && !det.eyes[0].valid && det.aperture == 0.0f);

    // Low confidence rejected.
    assert(!adaptFaceMesh468(mesh.data(), mesh.size(), 0.45f, roi, det));
    assert(!det.mouthValid && !det.lips.valid);

    // Inverted/crossed lips (lower above upper) rejected.
    auto crossed = buildSyntheticFaceMesh468(0.0f, 0.0f);
    crossed[kFaceMeshInnerLower9[4]].y = crossed[kFaceMeshInnerUpper9[4]].y - 0.08f;
    assert(!adaptFaceMesh468(crossed.data(), crossed.size(), 0.95f, roi, det));
    assert(!det.mouthValid && !det.lips.valid);

    // NaN coordinate rejected.
    auto nan_mesh = buildSyntheticFaceMesh468(0.0f, 0.0f);
    nan_mesh[kFaceMeshInnerUpper9[2]].x = std::numeric_limits<float>::quiet_NaN();
    assert(!adaptFaceMesh468(nan_mesh.data(), nan_mesh.size(), 0.95f, roi, det));
    assert(!det.mouthValid);

    // Out-of-bounds ROI rejected.
    FaceRoiTransform bad_roi = roi;
    bad_roi.crop_x = 0.80f;
    bad_roi.crop_w = 0.40f;
    assert(!adaptFaceMesh468(mesh.data(), mesh.size(), 0.95f, bad_roi, det));
    assert(!det.mouthValid);
  }

  std::puts("lip_landmarks: FaceMesh468/iBUG68 inner-lip contact, aperture, bend, eyes, and failure gates passed");
  return 0;
}
