// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include "Tracking.h"
constexpr unsigned camera_width=320, camera_height=180;
struct CameraStatus {
 bool ready=false,detector_ready=false;
 uint16_t sensor_id=0;
 unsigned stage=0; // Startup boundary reached; see camera-notes.md.
 int error=0; // ESP error or negative errno. Zero means no observed failure.
 uint16_t width=0,height=0; // Raw sensor dimensions; preview is always 320x180.
 uint32_t capture_count=0,captured_ms=0,inference_ms=0;
};
// Starts once per boot. Failure never causes automatic retries or GPIO resets.
bool camera_begin();
// Nonblocking lock: false leaves caller pixels/observations unchanged.
// A successful snapshot includes the same capture's pixels, geometry and status.
// Status alone may be copied before a first frame exists. Capacity is in pixels.
bool camera_copy_latest(uint16_t* pixels,size_t capacity,
                        copy_face::Frame* observations,CameraStatus* status);
