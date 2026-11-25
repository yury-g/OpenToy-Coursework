// SPDX-License-Identifier: MIT
#pragma once

namespace feather_face {
constexpr char version[] = "1.4.0";
constexpr char created[] = "2026-11-17";
constexpr char targetBoard[] = "feather-esp32s3-reversetft";

inline const char* appShortName(int face_version) {
  return face_version == 2 ? "EYES2" : "EYES1";
}

inline const char* appCanonicalId(int face_version) {
  return face_version == 2 ? "animatronic-eyes-2" : "animatronic-eyes-1";
}
}  // namespace feather_face
