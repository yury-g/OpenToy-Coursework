// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "driver/usb_serial_jtag.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "BoardTab5.h"
#include "CameraTab5.h"
#include "FaceDetectorTab5.h"
#include "SoundWake.h"
#include "adapter/Tab5Battery.h"
#include "adapter/Tab5CaptureProtocol.h"
#include "adapter/Tab5Ui.h"

using namespace opentoy::tab5;

namespace {

constexpr size_t kUsjTxBufBytes = 16u * 1024u;
constexpr size_t kUsjRxBufBytes = 1024u;
constexpr size_t kWriteChunkBytes = 4096u;
constexpr uint32_t kWriteTimeoutMs = 1500u;
constexpr size_t kLogRingBytes = 2048u;

portMUX_TYPE g_log_mux = portMUX_INITIALIZER_UNLOCKED;
char g_log_ring[kLogRingBytes] = {};
size_t g_log_head = 0;
size_t g_log_len = 0;

SemaphoreHandle_t g_ui_mutex = nullptr;
Tab5UiState g_ui;
opentoy_sound::SoundWakeMonitor g_sound_wake;
opentoy_sound::PcmBlockSummary g_last_pcm{};
opentoy_sound::ListeningExpression g_last_listen{};
BatterySample g_cached_bat{};
PowerDiagnostics g_cached_pwr{};

CameraTab5 g_cam;
BoardTab5 g_board;
FaceDetectorTab5 g_face;

int ringLogVprintf(const char* fmt, va_list args) {
  char msg[192] = {};
  const int written = std::vsnprintf(msg, sizeof(msg), fmt, args);
  if (written <= 0) return written;
  const size_t n = std::min<size_t>(size_t(written), sizeof(msg) - 1u);

  portENTER_CRITICAL_SAFE(&g_log_mux);
  for (size_t i = 0; i < n; ++i) {
    g_log_ring[g_log_head] = msg[i];
    g_log_head = (g_log_head + 1u) % kLogRingBytes;
    if (g_log_len < kLogRingBytes) {
      ++g_log_len;
    }
  }
  portEXIT_CRITICAL_SAFE(&g_log_mux);
  return written;
}

uint32_t nowMs() {
  return uint32_t(esp_timer_get_time() / 1000);
}

bool writeAllUsb(const uint8_t* data, size_t len) {
  size_t off = 0;
  uint32_t idle_cycles = 0;
  while (off < len) {
    const size_t chunk = std::min(kWriteChunkBytes, len - off);
    const int written = usb_serial_jtag_write_bytes(data + off, chunk, pdMS_TO_TICKS(100));
    if (written < 0) return false;
    if (written == 0) {
      if (++idle_cycles * 100u >= kWriteTimeoutMs) return false;
      continue;
    }
    idle_cycles = 0;
    off += size_t(written);
  }
  return usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(kWriteTimeoutMs)) == ESP_OK;
}

bool writeStrUsb(const char* text) {
  return writeAllUsb(reinterpret_cast<const uint8_t*>(text), std::strlen(text));
}

bool sendFramePacket(const CapturedFrame& frame, bool burst_last) {
  FrameHeader hdr;
  hdr.payload_len = uint32_t(frame.byte_length);
  hdr.payload_crc32 = crc32Bytes(frame.data, frame.byte_length);
  hdr.sequence = frame.sequence;
  hdr.captured_ms = frame.captured_ms;
  hdr.sent_ms = nowMs();
  hdr.width = frame.width;
  hdr.height = frame.height;
  hdr.sensor_id = frame.sensor_id;
  hdr.format = uint8_t(frame.format);
  hdr.flags = (frame.live_hardware ? kFlagLiveHardware : 0) |
              (frame.ae_warmed ? kFlagAeWarmed : 0) |
              (burst_last ? kFlagBurstLast : 0);

  if (uint32_t(hdr.sent_ms - hdr.captured_ms) > kMaxFreshAgeMs) {
    return false;
  }

  uint8_t raw_hdr[kHeaderBytes] = {};
  encodeFrameHeader(hdr, raw_hdr);

  if (!writeAllUsb(kPacketMagic, sizeof(kPacketMagic))) return false;
  if (!writeAllUsb(raw_hdr, sizeof(raw_hdr))) return false;
  if (!writeAllUsb(frame.data, frame.byte_length)) return false;
  if (!writeAllUsb(kPacketTrailer, sizeof(kPacketTrailer))) return false;
  return true;
}

void sendStatus() {
  const SensorStatus st = g_cam.status();
  const PowerDiagnostics pwr = g_cam.readPowerDiagnostics();
  BatterySample bat;
  if (pwr.ina226_present) {
    bat.kind = BatteryReadingKind::MeasuredMillivolts2S;
    bat.value = pwr.vbus_mv;
    bat.sampled_ms = nowMs();
    bat.valid = (pwr.vbus_mv >= kMinValid2SMillivolts && pwr.vbus_mv <= kMaxValid2SMillivolts);
  }
  char bat_str[24] = {};
  (void)formatBatteryStatus(bat, nowMs(), bat_str, sizeof(bat_str));

  const BoardQualification& bq = g_board.qualification();
  const FaceTrackSnapshot ft = g_face.snapshot(nowMs());
  Tab5AppMode cur_mode = Tab5AppMode::CameraEye;
  bool header_vis = true;
  if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
    cur_mode = g_ui.mode;
    header_vis = g_ui.header;
    g_cached_bat = bat;
    g_cached_pwr = pwr;
    xSemaphoreGive(g_ui_mutex);
  }

  char line[440] = {};
  std::snprintf(
      line,
      sizeof(line),
      "OK STATUS app=vibe-coding-camera-eye ver=%s date=%s mode=%s header=%u "
      "stage=%u ready=%u stream=%u jpeg=%u ae=%u sensor=0x%04x "
      "w=%u h=%u seq=%lu cap_ms=%lu enc_us=%lu "
      "panel=%s disp=%u touch=%u det=%u infer_ms=%lu faces=%u "
      "vbus_mv=%ld bat=\"%s\" err=%d\n",
      kTab5FirmwareVersion,
      kTab5VersionCreated,
      modeShortName(cur_mode),
      header_vis ? 1u : 0u,
      st.stage,
      st.ready ? 1u : 0u,
      st.streaming ? 1u : 0u,
      st.jpeg_ready ? 1u : 0u,
      st.ae_warmed ? 1u : 0u,
      unsigned(st.sensor_id),
      unsigned(st.width),
      unsigned(st.height),
      (unsigned long)(st.latest_seq),
      (unsigned long)(st.captured_ms),
      (unsigned long)(st.last_encode_us),
      bq.panel_name ? bq.panel_name : "NONE",
      bq.display_ready ? 1u : 0u,
      bq.touch_ready ? 1u : 0u,
      ft.detector_ready ? 1u : 0u,
      (unsigned long)(ft.infer_ms),
      ft.face_count,
      (long)(pwr.vbus_mv),
      bat_str,
      st.error);
  (void)writeStrUsb(line);
}

void sendHardwareReport() {
  const BoardQualification& bq = g_board.qualification();
  const PowerDiagnostics pwr = g_cam.readPowerDiagnostics();
  const size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

  char line[384] = {};
  std::snprintf(
      line,
      sizeof(line),
      "OK HW panel=%s panel_rev=%u disp=%u disp_err=%d "
      "touch=%u touch_addr=0x%02x touch_fw=%u touch_err=%d "
      "ina226=%u ina_die=0x%04x vbus_mv=%ld shunt_uv=%ld curr_ma=%ld "
      "io2=%u es8388=%u es7210=%u es7210_id=0x%04x bmi270=%u bmi_id=0x%02x rx8130=%u "
      "spiram_free=%lu iram_free=%lu\n",
      bq.panel_name ? bq.panel_name : "NONE",
      unsigned(bq.panel_rev),
      bq.display_ready ? 1u : 0u,
      bq.display_err,
      bq.touch_ready ? 1u : 0u,
      unsigned(bq.touch_i2c_addr),
      unsigned(bq.touch_fw_ver),
      bq.touch_err,
      pwr.ina226_present ? 1u : 0u,
      unsigned(pwr.die_id),
      (long)pwr.vbus_mv,
      (long)pwr.shunt_uv,
      (long)pwr.current_ma,
      bq.pi4ioe2_ok ? 1u : 0u,
      bq.es8388_ok ? 1u : 0u,
      bq.es7210_ok ? 1u : 0u,
      unsigned(bq.es7210_id),
      bq.bmi270_ok ? 1u : 0u,
      unsigned(bq.bmi270_chip_id),
      bq.rx8130_ok ? 1u : 0u,
      (unsigned long)free_spiram,
      (unsigned long)free_internal);
  (void)writeStrUsb(line);
}

void sendFaceReport() {
  const FaceTrackSnapshot ft = g_face.snapshot(nowMs());
  char line[256] = {};
  std::snprintf(
      line,
      sizeof(line),
      "OK FACE det_ready=%u frames=%lu infer_ms=%lu faces=%u tracking=%u "
      "gaze=%.3f,%.3f conf=%.3f mouth=\"unavailable_5pt_only\"\n",
      ft.detector_ready ? 1u : 0u,
      (unsigned long)ft.frames_processed,
      (unsigned long)ft.infer_ms,
      ft.face_count,
      ft.tracking ? 1u : 0u,
      double(ft.gaze_x),
      double(ft.gaze_y),
      double(ft.primary_face.valid ? ft.primary_face.confidence : 0.0f));
  (void)writeStrUsb(line);
}

void handleMicCommand(const char* args) {
  if (args && *args) {
    while (*args && std::isspace(static_cast<unsigned char>(*args))) ++args;
    if (*args == '1' || std::strncmp(args, "ON", 2) == 0) {
      const bool ok = g_board.setMicEnabled(true);
      if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_ui.micListening = ok;
        g_sound_wake.setSensitivity(ok ? opentoy_sound::Sensitivity::Normal
                                       : opentoy_sound::Sensitivity::Off);
        ++g_ui.epoch;
        xSemaphoreGive(g_ui_mutex);
      }
      // Sample two quick volatile blocks so immediate query reflects real hardware samples.
      if (ok) {
        for (int i = 0; i < 3; ++i) {
          vTaskDelay(pdMS_TO_TICKS(20));
          const auto blk = g_board.sampleMicVolatile();
          if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
            g_last_pcm = blk;
            g_last_listen = g_sound_wake.update(blk, nowMs(), g_ui.persona, false);
            xSemaphoreGive(g_ui_mutex);
          }
        }
      }
    } else if (*args == '0' || std::strncmp(args, "OFF", 3) == 0) {
      (void)g_board.setMicEnabled(false);
      if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_ui.micListening = false;
        g_sound_wake.setSensitivity(opentoy_sound::Sensitivity::Off);
        g_last_pcm = opentoy_sound::PcmBlockSummary{};
        g_last_listen = opentoy_sound::ListeningExpression{};
        ++g_ui.epoch;
        xSemaphoreGive(g_ui_mutex);
      }
    }
  }

  opentoy_sound::PcmBlockSummary pcm{};
  opentoy_sound::ListeningExpression lis{};
  bool mic_open = g_board.qualification().mic_open;
  if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
    pcm = g_last_pcm;
    lis = g_last_listen;
    xSemaphoreGive(g_ui_mutex);
  }
  char line[220] = {};
  std::snprintf(
      line,
      sizeof(line),
      "OK MIC es7210=%u open=%u valid=%u rms=%.4f peak=%.4f clip=%u floor=%.4f awake=%u alert=%.3f\n",
      g_board.qualification().es7210_ok ? 1u : 0u,
      mic_open ? 1u : 0u,
      pcm.valid ? 1u : 0u,
      double(pcm.rms),
      double(pcm.peak),
      pcm.clipped ? 1u : 0u,
      double(lis.noise_floor),
      lis.awake ? 1u : 0u,
      double(lis.alertness));
  (void)writeStrUsb(line);
}

void handleModeCommand(const char* arg) {
  while (arg && *arg && std::isspace(static_cast<unsigned char>(*arg))) ++arg;
  if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    if (arg && (std::strncmp(arg, "COPY", 4) == 0 || *arg == '1')) {
      g_ui.setMode(Tab5AppMode::CopyFace);
    } else if (arg && (std::strncmp(arg, "EYES", 4) == 0 || *arg == '2')) {
      g_ui.setMode(Tab5AppMode::Eyes1);
    } else if (arg && (std::strncmp(arg, "CAM", 3) == 0 || *arg == '0')) {
      g_ui.setMode(Tab5AppMode::CameraEye);
    }
    const Tab5AppMode m = g_ui.mode;
    const unsigned pg = unsigned(g_ui.page);
    const bool hdr = g_ui.header;
    xSemaphoreGive(g_ui_mutex);
    char line[128] = {};
    std::snprintf(
        line, sizeof(line), "OK MODE mode=%s page=%u header=%u\n", modeShortName(m), pg, hdr ? 1u : 0u);
    (void)writeStrUsb(line);
  }
}

bool readLineFromUsb(char* line_buf, size_t cap, size_t& pos, TickType_t wait_ticks) {
  uint8_t ch = 0;
  const int n = usb_serial_jtag_read_bytes(&ch, 1, wait_ticks);
  if (n <= 0) return false;

  if (ch == '\n' || ch == '\r') {
    if (pos == 0) return false;
    line_buf[pos] = '\0';
    pos = 0;
    return true;
  }
  if (pos + 1 < cap) {
    line_buf[pos++] = char(ch);
  }
  return false;
}

bool pollStopCommand(char* line_buf, size_t cap, size_t& pos) {
  while (true) {
    if (readLineFromUsb(line_buf, cap, pos, 0)) {
      if (std::strcmp(line_buf, "STOP") == 0) return true;
    } else {
      return false;
    }
  }
}

void handleSnap(int quality) {
  if (quality < 10) quality = 10;
  if (quality > 95) quality = 95;
  CapturedFrame frame;
  if (!g_cam.captureFreshJpeg(uint8_t(quality), kDefaultCaptureTimeoutMs, frame)) {
    const SensorStatus st = g_cam.status();
    char err_line[128] = {};
    std::snprintf(err_line,
                  sizeof(err_line),
                  "ERR SNAP stage=%u err=%d seq=%lu\n",
                  st.stage,
                  st.error,
                  (unsigned long)(st.latest_seq));
    (void)writeStrUsb(err_line);
    return;
  }
  if (!sendFramePacket(frame, true)) {
    const SensorStatus st = g_cam.status();
    char err_line[128] = {};
    std::snprintf(err_line,
                  sizeof(err_line),
                  "ERR TX stage=%u err=%d seq=%lu\n",
                  st.stage,
                  st.error,
                  (unsigned long)(st.latest_seq));
    (void)writeStrUsb(err_line);
  }
}

void handleSnapRgb(int scale_div) {
  if (scale_div < 1) scale_div = 1;
  if (scale_div > 8) scale_div = 8;
  CapturedFrame frame;
  if (!g_cam.captureFreshRgb565(unsigned(scale_div), kDefaultCaptureTimeoutMs, frame)) {
    const SensorStatus st = g_cam.status();
    char err_line[128] = {};
    std::snprintf(err_line,
                  sizeof(err_line),
                  "ERR SNAP_RGB stage=%u err=%d seq=%lu\n",
                  st.stage,
                  st.error,
                  (unsigned long)(st.latest_seq));
    (void)writeStrUsb(err_line);
    return;
  }
  (void)sendFramePacket(frame, true);
}

void handleBurst(int count, int interval_ms, int quality, char* line_buf, size_t cap, size_t& pos) {
  if (count < 1) count = 1;
  if (count > 30) count = 30;
  if (interval_ms < 50) interval_ms = 50;
  if (interval_ms > 2000) interval_ms = 2000;
  if (quality < 10) quality = 10;
  if (quality > 95) quality = 95;

  for (int i = 0; i < count; ++i) {
    if (pollStopCommand(line_buf, cap, pos)) {
      (void)writeStrUsb("OK STOP\n");
      return;
    }

    const uint32_t t_start = nowMs();
    CapturedFrame frame;
    if (!g_cam.captureFreshJpeg(uint8_t(quality), kDefaultCaptureTimeoutMs, frame)) {
      const SensorStatus st = g_cam.status();
      char err_line[128] = {};
      std::snprintf(
          err_line, sizeof(err_line), "ERR BURST idx=%d stage=%u err=%d\n", i, st.stage, st.error);
      (void)writeStrUsb(err_line);
      return;
    }
    const bool is_last = (i + 1 == count);
    if (!sendFramePacket(frame, is_last)) return;
    if (!is_last) {
      while (uint32_t(nowMs() - t_start) < uint32_t(interval_ms)) {
        if (pollStopCommand(line_buf, cap, pos)) {
          (void)writeStrUsb("OK STOP\n");
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
  }
}

void sendLog() {
  char snapshot[kLogRingBytes + 1] = {};
  size_t len = 0;

  portENTER_CRITICAL_SAFE(&g_log_mux);
  len = g_log_len;
  const size_t start = (len == kLogRingBytes) ? g_log_head : 0;
  for (size_t i = 0; i < len; ++i) {
    snapshot[i] = g_log_ring[(start + i) % kLogRingBytes];
  }
  snapshot[len] = '\0';
  portEXIT_CRITICAL_SAFE(&g_log_mux);

  char hdr[48] = {};
  std::snprintf(hdr, sizeof(hdr), "OK LOG len=%u\n", unsigned(len));
  (void)writeStrUsb(hdr);
  if (len > 0) {
    (void)writeStrUsb(snapshot);
    if (snapshot[len - 1] != '\n') {
      (void)writeStrUsb("\n");
    }
  }
  (void)writeStrUsb("END_LOG\n");
}

// Dedicated UI + touch + opt-in microphone service task (runs at 25 Hz on Core 0).
void uiTaskEntry(void*) {
  uint16_t* fb640x360 = static_cast<uint16_t*>(
      heap_caps_calloc(1, size_t(kUiWidth) * size_t(kUiHeight) * 2u, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  uint16_t* prev320x180 = static_cast<uint16_t*>(
      heap_caps_calloc(1, 320u * 180u * 2u, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!fb640x360 || !prev320x180) {
    vTaskDelete(nullptr);
    return;
  }

  uint32_t last_bat_ms = 0;
  bool applied_dim = false;
  bool applied_mic = false;

  while (true) {
    const uint32_t now = nowMs();

    // Refresh INA226 power/battery every 2000 ms.
    if (last_bat_ms == 0 || uint32_t(now - last_bat_ms) >= 2000u) {
      last_bat_ms = now;
      const PowerDiagnostics pwr = g_cam.readPowerDiagnostics();
      BatterySample bat;
      if (pwr.ina226_present) {
        bat.kind = BatteryReadingKind::MeasuredMillivolts2S;
        bat.value = pwr.vbus_mv;
        bat.sampled_ms = now;
        bat.valid = (pwr.vbus_mv >= kMinValid2SMillivolts && pwr.vbus_mv <= kMaxValid2SMillivolts);
      }
      if (xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_cached_bat = bat;
        g_cached_pwr = pwr;
        xSemaphoreGive(g_ui_mutex);
      }
    }

    bool flip_180 = false;
    bool want_dim = false;
    bool want_mic = false;
    bool mirror_gaze = true;
    if (xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      flip_180 = g_ui.flipLandscape;
      want_dim = g_ui.dimBacklight;
      want_mic = g_ui.micListening;
      mirror_gaze = g_ui.mirrorGaze;
      xSemaphoreGive(g_ui_mutex);
    }

    g_face.setMirror(mirror_gaze);
    if (want_dim != applied_dim) {
      applied_dim = want_dim;
      (void)g_board.setBacklightPercent(applied_dim ? 25 : 75);
    }
    if (want_mic != applied_mic) {
      applied_mic = want_mic;
      const bool ok = g_board.setMicEnabled(want_mic);
      g_sound_wake.setSensitivity((want_mic && ok) ? opentoy_sound::Sensitivity::Normal
                                                   : opentoy_sound::Sensitivity::Off);
    }

    const UiTouchPoint tp = g_board.pollTouch(flip_180);
    const SensorStatus cst = g_cam.status();
    const FaceTrackSnapshot ft = g_face.snapshot(now);
    const BoardQualification& bq = g_board.qualification();
    const bool have_prev = g_face.copyPreview320x180(prev320x180);

    opentoy_sound::PcmBlockSummary pcm_blk{};
    if (bq.mic_open) {
      pcm_blk = g_board.sampleMicVolatile();
    }

    Tab5UiState ui_snap;
    BatterySample bat_snap;
    PowerDiagnostics pwr_snap;
    opentoy_sound::ListeningExpression listen_snap{};
    if (xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
      g_ui.update(tp, now, bq.touch_ready);
      if (bq.mic_open) {
        g_last_pcm = pcm_blk;
        g_last_listen = g_sound_wake.update(
            pcm_blk,
            now,
            g_ui.persona,
            false,
            g_ui.pokeMask != 0,
            g_ui.mode == Tab5AppMode::CopyFace && ft.tracking);
      }
      ui_snap = g_ui;
      bat_snap = g_cached_bat;
      pwr_snap = g_cached_pwr;
      listen_snap = g_last_listen;
      xSemaphoreGive(g_ui_mutex);
    }

    Tab5LiveTelemetry tel;
    tel.cam_ready = cst.ready;
    tel.cam_streaming = cst.streaming;
    tel.ae_warmed = cst.ae_warmed;
    tel.sensor_id = cst.sensor_id;
    tel.frame_seq = cst.latest_seq;
    tel.encode_us = cst.last_encode_us;
    tel.detector_ready = ft.detector_ready;
    tel.face_tracking = ft.tracking;
    tel.face_count = ft.face_count;
    tel.infer_ms = ft.infer_ms;
    tel.gaze_x = ft.gaze_x;
    tel.gaze_y = ft.gaze_y;
    tel.primary_face = ft.primary_face;
    tel.panel_name = bq.panel_name;
    tel.display_ready = bq.display_ready;
    tel.touch_ready = bq.touch_ready;
    tel.ina226_ok = pwr_snap.ina226_present;
    tel.vbus_mv = pwr_snap.vbus_mv;
    tel.current_ma = pwr_snap.current_ma;
    tel.es8388_ok = bq.es8388_ok;
    tel.es7210_ok = bq.es7210_ok;
    tel.bmi270_ok = bq.bmi270_ok;
    tel.mic_listening = bq.mic_open;
    tel.sound_awake = listen_snap.awake;
    tel.mic_rms = g_last_pcm.rms;

    if (bq.display_ready) {
      renderTab5Frame(
          fb640x360, ui_snap, bat_snap, tel, have_prev ? prev320x180 : nullptr, now);
      (void)g_board.presentLandscape640x360(fb640x360, ui_snap.flipLandscape);
    }

    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

}  // namespace

extern "C" void app_main(void) {
  esp_log_level_set("*", ESP_LOG_WARN);
  (void)esp_log_set_vprintf(&ringLogVprintf);

  usb_serial_jtag_driver_config_t usj_cfg = {
      .tx_buffer_size = kUsjTxBufBytes,
      .rx_buffer_size = kUsjRxBufBytes,
  };
  (void)usb_serial_jtag_driver_install(&usj_cfg);

  // Keep USB command & SNAP/BURST JPEG encoding at priority 5 so background UI/face-detect never starve freshness.
  vTaskPrioritySet(nullptr, 5);

  g_ui_mutex = xSemaphoreCreateMutex();

  if (g_cam.begin()) {
    (void)g_board.begin(g_cam.i2cBus());
    if (g_cam.startStream()) {
      (void)g_face.start(&g_cam);
    }
  }

  (void)xTaskCreatePinnedToCore(uiTaskEntry, "tab5_ui", 8192, nullptr, 3, nullptr, 0);

  char line_buf[128] = {};
  size_t pos = 0;

  while (true) {
    if (!readLineFromUsb(line_buf, sizeof(line_buf), pos, pdMS_TO_TICKS(25))) {
      continue;
    }

    size_t len = std::strlen(line_buf);
    while (len > 0 && std::isspace(static_cast<unsigned char>(line_buf[len - 1]))) {
      line_buf[--len] = '\0';
    }
    if (len == 0) continue;

    if (std::strcmp(line_buf, "PING") == 0 ||
        std::strcmp(line_buf, "?") == 0 ||
        std::strcmp(line_buf, "STATUS") == 0) {
      sendStatus();
    } else if (std::strcmp(line_buf, "HW") == 0) {
      sendHardwareReport();
    } else if (std::strcmp(line_buf, "FACE") == 0) {
      sendFaceReport();
    } else if (std::strncmp(line_buf, "MIC", 3) == 0) {
      handleMicCommand(line_buf + 3);
    } else if (std::strncmp(line_buf, "MODE", 4) == 0) {
      handleModeCommand(line_buf + 4);
    } else if (std::strcmp(line_buf, "MENU") == 0) {
      if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_ui.openMenu();
        xSemaphoreGive(g_ui_mutex);
      }
      (void)writeStrUsb("OK MENU page=1\n");
    } else if (std::strcmp(line_buf, "BACK") == 0) {
      if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_ui.back();
        const unsigned pg = unsigned(g_ui.page);
        xSemaphoreGive(g_ui_mutex);
        char resp[48] = {};
        std::snprintf(resp, sizeof(resp), "OK BACK page=%u\n", pg);
        (void)writeStrUsb(resp);
      }
    } else if (std::strcmp(line_buf, "PERF") == 0) {
      if (g_ui_mutex && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_ui.togglePerformance();
        const bool hdr = g_ui.header;
        xSemaphoreGive(g_ui_mutex);
        char resp[48] = {};
        std::snprintf(resp, sizeof(resp), "OK PERF header=%u\n", hdr ? 1u : 0u);
        (void)writeStrUsb(resp);
      }
    } else if (std::strcmp(line_buf, "LOG") == 0) {
      sendLog();
    } else if (std::strcmp(line_buf, "STOP") == 0) {
      (void)writeStrUsb("OK STOP\n");
    } else if (std::strncmp(line_buf, "SNAP_RGB", 8) == 0) {
      int scale_div = 4;
      if (len > 8) scale_div = std::atoi(line_buf + 8);
      handleSnapRgb(scale_div);
    } else if (std::strncmp(line_buf, "SNAP", 4) == 0) {
      int quality = 82;
      if (len > 4) quality = std::atoi(line_buf + 4);
      handleSnap(quality);
    } else if (std::strncmp(line_buf, "BURST", 5) == 0) {
      int count = 10;
      int interval_ms = 200;
      int quality = 80;
      const int parsed = std::sscanf(line_buf + 5, "%d %d %d", &count, &interval_ms, &quality);
      if (parsed < 1) count = 10;
      handleBurst(count, interval_ms, quality, line_buf, sizeof(line_buf), pos);
    } else {
      (void)writeStrUsb("ERR CMD\n");
    }
  }
}
