// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "CameraTab5.h"
#include "adapter/Tab5Ui.h"

namespace opentoy {
namespace tab5 {

struct FaceTrackSnapshot {
  bool detector_ready = false;
  bool tracking = false;
  unsigned face_count = 0;
  uint32_t frames_processed = 0;
  uint32_t infer_ms = 0;
  float gaze_x = 0.0f;
  float gaze_y = 0.0f;
  FaceBoxOverlay primary_face{};
};

class FaceDetectorTab5 {
 public:
  FaceDetectorTab5() = default;
  ~FaceDetectorTab5();

  bool start(CameraTab5* cam);
  void stop();

  void setMirror(bool mirror) { mirror_.store(mirror); }
  FaceTrackSnapshot snapshot(uint32_t now_ms);

  // Copies latest 320x180 downsampled RGB565 frame for non-blocking UI viewfinder/inset.
  bool copyPreview320x180(uint16_t* dst320x180);

 private:
  static void workerEntry(void* arg);
  void workerLoop();

  CameraTab5* cam_ = nullptr;
  SemaphoreHandle_t mutex_ = nullptr;
  TaskHandle_t task_ = nullptr;
  std::atomic<bool> stop_{false};
  std::atomic<bool> mirror_{true};

  uint16_t* work_320x180_ = nullptr;
  uint16_t* preview_320x180_ = nullptr;
  bool have_preview_ = false;

  FaceTrackSnapshot state_{};
};

}  // namespace tab5
}  // namespace opentoy
