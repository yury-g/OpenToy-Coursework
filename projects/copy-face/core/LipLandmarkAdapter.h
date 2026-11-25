// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "Geometry.h"
#include "Tracking.h"

namespace copy_face {

// Validated portable adapter for dense facial landmark regressors (MediaPipe FaceMesh 468-pt
// Apache-2.0 and 68-pt iBUG / 98-pt WFLW PFLD MIT topologies) targeting ESP-DL 3.3.15 (.espdl).
//
// Honesty rules:
// 1. Five-point face detectors (e.g. human_face_detect MSRMNP_S8_V1) NEVER populate inner lips
//    or eyelids; applyFivePointOnly() explicitly clears mouthValid and landmark valid flags.
// 2. Direct 1D/2D image dark-valley/ridge scans are lighting-dependent diagnostics only and
//    must NEVER set mouthValid = true.
// 3. Only dense landmark outputs with finite in-frame coordinates, confidence >= minConfidence,
//    and passing Geometry.h normalizeLips() + validEye() invariants set mouthValid = true.

struct FaceRoiTransform {
  // Normalized [0, 1] crop origin and size within the full camera frame.
  float crop_x = 0.0f;
  float crop_y = 0.0f;
  float crop_w = 1.0f;
  float crop_h = 1.0f;
  // Full camera frame width / height (isotropic correction used by Geometry.h: y / imageAspect).
  // Note: In Geometry.h, imageAspect = frame_height / frame_width when y is stretched, or 1.0f
  // for square frames; specifically, physical_dx = dx * W and physical_dy = dy * H = (dy / (W/H)) * W,
  // so imageAspect = float(frame_width) / float(frame_height).
  float image_aspect = 1.0f;
};

constexpr float kMinLandmarkConfidence = 0.60f;

// Canonical MediaPipe FaceMesh 468-point inner-lip indices (Apache-2.0 topology):
// 78 and 308 are the inner mouth commissures; 13 and 14 are the upper/lower inner midlines.
constexpr std::array<uint16_t, lipPoints> kFaceMeshInnerUpper9 = {
    78, 191, 80, 82, 13, 312, 310, 415, 308};
constexpr std::array<uint16_t, lipPoints> kFaceMeshInnerLower9 = {
    78, 95, 88, 87, 14, 317, 318, 324, 308};

// Canonical MediaPipe FaceMesh 6-point EAR eye rings (corner, upper1, upper2, corner, lower2, lower1).
constexpr std::array<uint16_t, 6> kFaceMeshLeftEye6 = {33, 160, 158, 133, 153, 144};
constexpr std::array<uint16_t, 6> kFaceMeshRightEye6 = {362, 385, 387, 263, 373, 380};

inline bool validRoiTransform(const FaceRoiTransform& roi) {
  return std::isfinite(roi.crop_x) && std::isfinite(roi.crop_y) &&
         std::isfinite(roi.crop_w) && std::isfinite(roi.crop_h) &&
         std::isfinite(roi.image_aspect) &&
         roi.crop_x >= 0.0f && roi.crop_y >= 0.0f &&
         roi.crop_w > 0.02f && roi.crop_h > 0.02f &&
         roi.crop_x + roi.crop_w <= 1.0001f &&
         roi.crop_y + roi.crop_h <= 1.0001f &&
         roi.image_aspect >= 0.2f && roi.image_aspect <= 5.0f;
}

inline Point mapRoiPointToFrame(Point local_pt, const FaceRoiTransform& roi) {
  return Point{
      roi.crop_x + local_pt.x * roi.crop_w,
      roi.crop_y + local_pt.y * roi.crop_h,
  };
}

inline void clearDenseMouthAndEyes(Detection& det) {
  det.mouthValid = false;
  det.aperture = 0.0f;
  det.bend = 0.0f;
  det.lips = LipLandmarks{};
  det.eyes[0] = EyeLandmarks{};
  det.eyes[1] = EyeLandmarks{};
}

// Enforces that 5-keypoint face detection NEVER claims mouth or eyelid geometry.
inline void applyFivePointOnly(Detection& det) {
  clearDenseMouthAndEyes(det);
}

// Computes normalized aperture [0, 1] and bend [-1, 1] directly from a validated LipShape.
inline void computeMouthMetricsFromShape(const LipShape& shape,
                                         float& out_aperture,
                                         float& out_bend) {
  out_aperture = 0.0f;
  out_bend = 0.0f;
  if (!shape.valid) return;

  float center_gap = 0.0f;
  float center_mid_y = 0.0f;
  for (unsigned i = 3; i <= 5; ++i) {
    center_gap += (shape.lower[i].y - shape.upper[i].y);
    center_mid_y += 0.5f * (shape.lower[i].y + shape.upper[i].y);
  }
  center_gap /= 3.0f;
  center_mid_y /= 3.0f;

  // Contact zone: <= 0.03 in normalized local mouth coordinates maps cleanly to 0 aperture.
  if (center_gap <= 0.03f) {
    out_aperture = 0.0f;
  } else {
    // Full open mouth (~0.65 local height) maps to 1.0.
    out_aperture = clamp((center_gap - 0.03f) / 0.62f, 0.0f, 1.0f);
  }

  // In Geometry.h local coordinates, positive center_mid_y corresponds to Smile (center bowed down
  // relative to upturned commissures) and negative center_mid_y corresponds to Frown.
  out_bend = clamp(center_mid_y / 0.28f, -1.0f, 1.0f);
}

// Ensures left-to-right image ordering if a landmark model orders from subject-left to subject-right.
inline void ensureImageLeftToRight(LipEdge& upper, LipEdge& lower) {
  const float x_first = 0.5f * (upper.front().x + lower.front().x);
  const float x_last = 0.5f * (upper.back().x + lower.back().x);
  if (x_first > x_last) {
    std::reverse(upper.begin(), upper.end());
    std::reverse(lower.begin(), lower.end());
  }
}

// Populates Detection from a 468-point MediaPipe FaceMesh landmark array (ROI-normalized [0, 1]).
inline bool adaptFaceMesh468(const Point* roi_points,
                             size_t point_count,
                             float landmark_confidence,
                             const FaceRoiTransform& roi,
                             Detection& det) {
  clearDenseMouthAndEyes(det);
  if (!roi_points || point_count < 468 || !std::isfinite(landmark_confidence) ||
      landmark_confidence < kMinLandmarkConfidence || !validRoiTransform(roi)) {
    return false;
  }

  LipLandmarks lips;
  lips.imageAspect = roi.image_aspect;
  for (unsigned i = 0; i < lipPoints; ++i) {
    const Point u = mapRoiPointToFrame(roi_points[kFaceMeshInnerUpper9[i]], roi);
    const Point l = mapRoiPointToFrame(roi_points[kFaceMeshInnerLower9[i]], roi);
    if (!imagePoint(u) || !imagePoint(l)) return false;
    lips.upper[i] = u;
    lips.lower[i] = l;
  }
  ensureImageLeftToRight(lips.upper, lips.lower);
  lips.valid = true;

  const LipShape shape = normalizeLips(lips);
  if (!shape.valid) {
    return false;
  }

  EyeLandmarks eye_a, eye_b;
  eye_a.imageAspect = roi.image_aspect;
  eye_b.imageAspect = roi.image_aspect;
  for (unsigned i = 0; i < 6; ++i) {
    eye_a.contour[i] = mapRoiPointToFrame(roi_points[kFaceMeshLeftEye6[i]], roi);
    eye_b.contour[i] = mapRoiPointToFrame(roi_points[kFaceMeshRightEye6[i]], roi);
  }
  eye_a.valid = true;
  eye_b.valid = true;
  if (!validEye(eye_a) || !validEye(eye_b)) {
    return false;
  }

  // Order eyes[0] as image-left and eyes[1] as image-right.
  if (eye_a.contour[0].x <= eye_b.contour[0].x) {
    det.eyes[0] = eye_a;
    det.eyes[1] = eye_b;
  } else {
    det.eyes[0] = eye_b;
    det.eyes[1] = eye_a;
  }

  det.lips = lips;
  computeMouthMetricsFromShape(shape, det.aperture, det.bend);
  det.mouthValid = true;
  return true;
}

// Resamples a 5-vertex inner-lip polyline [corner_L, p1, p2, p3, corner_R] (from 68-pt iBUG
// indices 60..67 or 98-pt WFLW indices 88..95) into 9 matched stations at u = 0, 1/8, ..., 1.
// Unlike 2-corner fabrication, all 5 vertices are true inner-margin detections (midpoints fall
// directly on measured inner-lip segments).
inline LipEdge resampleInnerPolyline5To9(const std::array<Point, 5>& poly) {
  LipEdge out{};
  for (unsigned i = 0; i < lipPoints; ++i) {
    const unsigned seg = i / 2;
    if (i % 2 == 0) {
      out[i] = poly[seg];
    } else {
      out[i] = Point{
          0.5f * (poly[seg].x + poly[seg + 1].x),
          0.5f * (poly[seg].y + poly[seg + 1].y),
      };
    }
  }
  return out;
}

// Populates Detection from a 68-point iBUG/300W landmark array (ROI-normalized [0, 1]).
// Eyes: 36..41 (left), 42..47 (right).
// Inner lip: 60 (left corner), 61, 62, 63 (upper inner), 64 (right corner), 65, 66, 67 (lower inner R->L).
inline bool adaptIbug68(const Point* roi_points,
                        size_t point_count,
                        float landmark_confidence,
                        const FaceRoiTransform& roi,
                        Detection& det) {
  clearDenseMouthAndEyes(det);
  if (!roi_points || point_count < 68 || !std::isfinite(landmark_confidence) ||
      landmark_confidence < kMinLandmarkConfidence || !validRoiTransform(roi)) {
    return false;
  }

  std::array<Point, 5> up5 = {
      mapRoiPointToFrame(roi_points[60], roi),
      mapRoiPointToFrame(roi_points[61], roi),
      mapRoiPointToFrame(roi_points[62], roi),
      mapRoiPointToFrame(roi_points[63], roi),
      mapRoiPointToFrame(roi_points[64], roi),
  };
  std::array<Point, 5> lo5 = {
      mapRoiPointToFrame(roi_points[60], roi),
      mapRoiPointToFrame(roi_points[67], roi),
      mapRoiPointToFrame(roi_points[66], roi),
      mapRoiPointToFrame(roi_points[65], roi),
      mapRoiPointToFrame(roi_points[64], roi),
  };
  for (unsigned i = 0; i < 5; ++i) {
    if (!imagePoint(up5[i]) || !imagePoint(lo5[i])) return false;
  }

  LipLandmarks lips;
  lips.imageAspect = roi.image_aspect;
  lips.upper = resampleInnerPolyline5To9(up5);
  lips.lower = resampleInnerPolyline5To9(lo5);
  ensureImageLeftToRight(lips.upper, lips.lower);
  lips.valid = true;

  const LipShape shape = normalizeLips(lips);
  if (!shape.valid) return false;

  EyeLandmarks left_eye, right_eye;
  left_eye.imageAspect = roi.image_aspect;
  right_eye.imageAspect = roi.image_aspect;
  for (unsigned i = 0; i < 6; ++i) {
    left_eye.contour[i] = mapRoiPointToFrame(roi_points[36 + i], roi);
    right_eye.contour[i] = mapRoiPointToFrame(roi_points[42 + i], roi);
  }
  left_eye.valid = true;
  right_eye.valid = true;
  if (!validEye(left_eye) || !validEye(right_eye)) return false;

  det.eyes[0] = left_eye;
  det.eyes[1] = right_eye;
  det.lips = lips;
  computeMouthMetricsFromShape(shape, det.aperture, det.bend);
  det.mouthValid = true;
  return true;
}

}  // namespace copy_face
