// SPDX-License-Identifier: MIT
#include "FaceDetectorTab5.h"

#include <cstring>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "human_face_detect.hpp"

#include "adapter/copy_face/Geometry.h"
#include "adapter/copy_face/LipLandmarkAdapter.h"
#include "adapter/copy_face/Tracking.h"

namespace opentoy {
namespace tab5 {

namespace {

constexpr uint16_t kInferW = 320;
constexpr uint16_t kInferH = 180;
constexpr size_t kInferBytes = size_t(kInferW) * size_t(kInferH) * sizeof(uint16_t);

uint32_t nowMs() {
  return uint32_t(esp_timer_get_time() / 1000);
}

copy_face::Tracker g_tracker;

}  // namespace

FaceDetectorTab5::~FaceDetectorTab5() {
  stop();
}

bool FaceDetectorTab5::start(CameraTab5* cam) {
  if (!cam || task_) return false;
  cam_ = cam;
  mutex_ = xSemaphoreCreateMutex();
  work_320x180_ =
      static_cast<uint16_t*>(heap_caps_calloc(1, kInferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  preview_320x180_ =
      static_cast<uint16_t*>(heap_caps_calloc(1, kInferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!mutex_ || !work_320x180_ || !preview_320x180_) {
    stop();
    return false;
  }

  stop_.store(false);
  // Priority 2 on Core 0 runs smoothly below tab5_ui (priority 3) and USB SNAP/cmd (priority 5).
  if (xTaskCreatePinnedToCore(workerEntry, "tab5_face_det", 16384, this, 2, &task_, 0) != pdPASS) {
    stop();
    return false;
  }
  return true;
}

void FaceDetectorTab5::stop() {
  stop_.store(true);
  for (int i = 0; i < 60 && task_ != nullptr; ++i) {
    vTaskDelay(pdMS_TO_TICKS(15));
  }
  if (work_320x180_) {
    heap_caps_free(work_320x180_);
    work_320x180_ = nullptr;
  }
  if (preview_320x180_) {
    heap_caps_free(preview_320x180_);
    preview_320x180_ = nullptr;
  }
  if (mutex_) {
    vSemaphoreDelete(mutex_);
    mutex_ = nullptr;
  }
}

void FaceDetectorTab5::workerEntry(void* arg) {
  static_cast<FaceDetectorTab5*>(arg)->workerLoop();
}

void FaceDetectorTab5::workerLoop() {
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >= 4u * 1024u * 1024u) {
    HumanFaceDetect detector(HumanFaceDetect::MSRMNP_S8_V1, false);
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
      state_.detector_ready = true;
      xSemaphoreGive(mutex_);
    }

    uint32_t last_seq = 0;
    while (!stop_.load()) {
      uint32_t seq = 0, cap_ms = 0;
      bool ae = false;
      if (!cam_->copyLatestDownsampledRgb565(work_320x180_, kInferW, kInferH, seq, cap_ms, ae) ||
          seq == last_seq) {
        vTaskDelay(pdMS_TO_TICKS(15));
        continue;
      }
      last_seq = seq;

      dl::image::img_t img = {
          work_320x180_,
          kInferW,
          kInferH,
          dl::image::DL_IMAGE_PIX_TYPE_RGB565LE,
      };
      const uint32_t t0 = nowMs();
      auto& results = detector.run(img);
      const uint32_t dt_ms = nowMs() - t0;

      copy_face::Frame cf_frame;
      cf_frame.captured = nowMs();  // Stamp post-inference freshness inside 250ms window
      for (const auto& r : results) {
        if (cf_frame.count >= 8 || r.box.size() < 4) continue;
        copy_face::Detection d;
        const float x0 = copy_face::clamp(float(r.box[0]) / float(kInferW), 0.0f, 1.0f);
        const float y0 = copy_face::clamp(float(r.box[1]) / float(kInferH), 0.0f, 1.0f);
        const float x1 = copy_face::clamp(float(r.box[2]) / float(kInferW), 0.0f, 1.0f);
        const float y1 = copy_face::clamp(float(r.box[3]) / float(kInferH), 0.0f, 1.0f);
        d.x = x0;
        d.y = y0;
        d.w = x1 - x0;
        d.h = y1 - y0;
        d.confidence = r.score;
        if (!copy_face::valid(d)) continue;
        if (r.keypoint.size() == 10) {
          d.keypointsValid = true;
          for (unsigned k = 0; k < 5u; ++k) {
            const float kx = float(r.keypoint[k * 2]) / float(kInferW);
            const float ky = float(r.keypoint[k * 2 + 1]) / float(kInferH);
            if (kx < 0.0f || kx > 1.0f || ky < 0.0f || ky > 1.0f) d.keypointsValid = false;
            d.keypoints[k] = {kx, ky};
          }
        }
        // Strictly enforce 5-point-only mouth boundary (mouthValid = false).
        copy_face::applyFivePointOnly(d);
        cf_frame.faces[cf_frame.count++] = d;
      }

      if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        std::memcpy(preview_320x180_, work_320x180_, kInferBytes);
        have_preview_ = true;
        g_tracker.observe(cf_frame, cf_frame.captured);
        const copy_face::Pose pose = g_tracker.step(cf_frame.captured, mirror_.load());

        state_.frames_processed++;
        state_.infer_ms = dt_ms;
        state_.face_count = cf_frame.count;
        state_.tracking = pose.tracking;
        state_.gaze_x = pose.x;
        state_.gaze_y = pose.y;

        copy_face::Detection sel{};
        uint32_t sel_at = 0;
        if (g_tracker.selected(sel, sel_at, cf_frame.captured)) {
          state_.primary_face.valid = true;
          state_.primary_face.x = sel.x;
          state_.primary_face.y = sel.y;
          state_.primary_face.w = sel.w;
          state_.primary_face.h = sel.h;
          state_.primary_face.confidence = sel.confidence;
          state_.primary_face.keypoints_valid = sel.keypointsValid;
          for (unsigned k = 0; k < 5u; ++k) {
            state_.primary_face.kp_x[k] = sel.keypoints[k].x;
            state_.primary_face.kp_y[k] = sel.keypoints[k].y;
          }
        } else if (cf_frame.count > 0) {
          const auto& f0 = cf_frame.faces[0];
          state_.primary_face = {
              true, f0.x, f0.y, f0.w, f0.h, f0.confidence, f0.keypointsValid, {}, {}};
        } else {
          state_.primary_face = FaceBoxOverlay{};
        }
        xSemaphoreGive(mutex_);
      }

      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }

  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
    state_.detector_ready = false;
    xSemaphoreGive(mutex_);
  }
  task_ = nullptr;
  vTaskDelete(nullptr);
}

FaceTrackSnapshot FaceDetectorTab5::snapshot(uint32_t now_ms) {
  FaceTrackSnapshot out;
  if (mutex_ && xSemaphoreTake(mutex_, pdMS_TO_TICKS(15)) == pdTRUE) {
    const copy_face::Pose pose = g_tracker.step(now_ms, mirror_.load());
    state_.tracking = pose.tracking;
    state_.gaze_x = pose.x;
    state_.gaze_y = pose.y;
    if (!pose.tracking && state_.face_count == 0) {
      state_.primary_face.valid = false;
    }
    out = state_;
    xSemaphoreGive(mutex_);
  }
  return out;
}

bool FaceDetectorTab5::copyPreview320x180(uint16_t* dst320x180) {
  if (!dst320x180 || !mutex_) return false;
  bool ok = false;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    if (have_preview_ && preview_320x180_) {
      std::memcpy(dst320x180, preview_320x180_, kInferBytes);
      ok = true;
    }
    xSemaphoreGive(mutex_);
  }
  return ok;
}

}  // namespace tab5
}  // namespace opentoy
