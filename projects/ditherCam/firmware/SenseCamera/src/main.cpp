// SPDX-License-Identifier: MIT
// Seeed XIAO ESP32S3 Sense on Expansion Base Firmware (Photos-Only + Fast 1-Bit Stream)
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "FlipCamButtons.h"
#include "FlipCamCrc.h"
#include "FlipCamDisplay.h"
#include "FlipCamDither.h"
#include "FlipCamJpegDecode.h"
#include "FlipCamLittleFsBackend.h"
#include "FlipCamPairConfig.h"
#include "FlipCamProtocol.h"
#include "FlipCamStorage.h"
#include "FlipCamStream.h"
#include "FlipCamTypes.h"
#include "FlipCamUsbProtocol.h"

#include "CameraMgr.h"
#include "OledDisplay.h"

namespace {

constexpr int kPinD1Button = 2;   // Expansion Base user button D1 (GPIO2, active LOW)
constexpr int kPinD3Buzzer = 4;   // Expansion Base passive buzzer D3 (GPIO4)

SemaphoreHandle_t g_stateMutex = nullptr;
SemaphoreHandle_t g_fsMutex = nullptr;
SemaphoreHandle_t g_slotMutex = nullptr;
SemaphoreHandle_t g_oledFbMutex = nullptr;

TaskHandle_t g_camTaskHandle = nullptr;
TaskHandle_t g_oledTaskHandle = nullptr;
TaskHandle_t g_streamTaskHandle = nullptr;

flipcam::ButtonController g_d1Button;
flipcam::LittleFsBackend g_fsBackend;
flipcam::MediaStore g_store(g_fsBackend);
flipcam::FlipCamSettings g_settings{};
bool g_settingsDirty = false;

flipcam::OledFrameBuffer g_oledFb;
uint32_t g_lastOledCrc = 0;

WebServer g_peerServer(FLIPCAM_PEER_PORT);
WiFiServer g_streamServer(FLIPCAM_STREAM_PORT);

// Latest-frame 1-deep slots (drop old frames; never queue lag)
constexpr size_t kDecGrayCap = 160u * 120u;
uint8_t* g_decScratch = nullptr;
uint8_t* g_monoSlotPlane = nullptr;
uint8_t* g_oledSlotGray = nullptr;
int g_oledSlotW = 0;
int g_oledSlotH = 0;
uint32_t g_monoSeq = 0;
uint32_t g_monoCaptureMs = 0;
uint8_t g_monoFlags = 0;

std::vector<uint8_t> g_cachedPreview;
uint16_t g_cachedPreviewW = 0;
uint16_t g_cachedPreviewH = 0;

uint32_t g_oledBlinkUntilMs = 0;
uint32_t g_wifiReconnects = 0;
uint32_t g_lastWifiAttemptMs = 0;
uint32_t g_wifiBackoffMs = 500;
bool g_wasWifiConnected = false;

// Measured honest counters & sliding-window rates
uint32_t g_monoFramesEncoded = 0;
uint32_t g_camGrabUsAvg = 0;
uint32_t g_monoEncodeUsAvg = 0;
uint32_t g_streamFramesSent = 0;
uint64_t g_streamBytesSent = 0;
uint32_t g_streamRleHits = 0;
uint32_t g_streamRawFallbacks = 0;
uint32_t g_streamFpsX10 = 0;
uint32_t g_oledFpsX10 = 0;

uint32_t g_synthSeq = 1;
volatile bool g_rebootRequested = false;

char g_usbBuf[160];
size_t g_usbLen = 0;

void buzzerChirp(bool soundEnabled, uint32_t freqHz = 1047, uint32_t ms = 45) {
  if (soundEnabled) {
    tone(kPinD3Buzzer, freqHz, ms);
  }
}

bool checkPairAuth() {
  if (!g_peerServer.hasHeader("X-FlipCam-Pair")) return false;
  return g_peerServer.header("X-FlipCam-Pair") == FLIPCAM_PAIR_TOKEN;
}

bool performLocalSpoolCapture(bool synthetic) {
  flipcam::FlipCamSettings snap{};
  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    snap = g_settings;
    xSemaphoreGive(g_stateMutex);
  }
  std::vector<uint8_t> jpg;
  uint16_t w = 0, h = 0;
  uint32_t crc = 0;
  if (!xiao_hw::cameraCaptureStill(0, 0, synthetic, g_synthSeq++, snap.orientation, jpg, w, h, crc)) {
    return false;
  }
  bool saved = false;
  flipcam::MediaEntry entry{};
  if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    saved = g_store.saveStill(jpg.data(), jpg.size(), w, h,
                              snap.look, snap.contrast, snap.mirror,
                              synthetic, entry);
    xSemaphoreGive(g_fsMutex);
  }
  if (saved) {
    g_oledBlinkUntilMs = millis() + 130;
    buzzerChirp(snap.sound != 0, 1047, 45);
  }
  return saved;
}

// ---------------------------------------------------------------------------
// Producer Task (Core 0): QVGA JPEG (320x240) -> 90CW+4:3+LUT+Dither (304x228)
// ---------------------------------------------------------------------------
void camProducerTask(void*) {
  std::vector<uint8_t> jpg;
  jpg.reserve(32 * 1024);
  std::vector<uint8_t> decodeWork(4096);
  uint8_t* localMono = static_cast<uint8_t*>(
      heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  for (;;) {
    if (!xiao_hw::cameraInfo().ok || !g_decScratch || !localMono) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }
    const uint32_t tGrab0Us = micros();
    uint16_t w = 0, h = 0;
    if (!xiao_hw::cameraGrabPreview(jpg, w, h)) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }
    const uint32_t grabUs = micros() - tGrab0Us;

    const uint32_t t0Us = micros();
    int decW = 0, decH = 0;
    if (!flipcam::decodeJpegToGray(jpg.data(), jpg.size(), g_decScratch, kDecGrayCap,
                                   160, 120, decW, decH, &decodeWork)) {
      vTaskDelay(pdMS_TO_TICKS(4));
      continue;
    }

    uint8_t look = 0, contrast = 2, mirror = 0, orientation = 0;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      look = g_settings.look;
      contrast = g_settings.contrast;
      mirror = g_settings.mirror;
      orientation = g_settings.orientation;
      g_cachedPreview.assign(jpg.begin(), jpg.end());
      g_cachedPreviewW = w;
      g_cachedPreviewH = h;
      xSemaphoreGive(g_stateMutex);
    }

    // Keep transmitted FMB1 mono plane in Selfie baseline (RLCD applies Rear 180 locally)
    flipcam::dither90CwCrop43ToMonoPlane(g_decScratch, decW, decH, localMono,
                                         flipcam::kFmbViewW, flipcam::kFmbViewH,
                                         look, contrast, mirror != 0);
    const uint32_t elapsedUs = micros() - t0Us;

    if (xSemaphoreTake(g_slotMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      std::memcpy(g_monoSlotPlane, localMono, flipcam::kFmbRawPlaneBytes);
      if (g_oledSlotGray) {
        std::memcpy(g_oledSlotGray, g_decScratch, static_cast<size_t>(decW) * static_cast<size_t>(decH));
        g_oledSlotW = decW;
        g_oledSlotH = decH;
      }
      g_monoCaptureMs = millis();
      g_monoFlags = static_cast<uint8_t>((mirror ? 1u : 0u) | ((contrast & 3u) << 1) |
                                         ((look & 1u) << 3) | ((orientation & 1u) << 4));
      g_monoSeq++;
      g_monoFramesEncoded++;
      g_camGrabUsAvg = g_camGrabUsAvg ? ((g_camGrabUsAvg * 7u + grabUs) / 8u) : grabUs;
      g_monoEncodeUsAvg = g_monoEncodeUsAvg ? ((g_monoEncodeUsAvg * 7u + elapsedUs) / 8u) : elapsedUs;
      xSemaphoreGive(g_slotMutex);
      if (g_streamTaskHandle) xTaskNotifyGive(g_streamTaskHandle);
      if (g_oledTaskHandle) xTaskNotifyGive(g_oledTaskHandle);
    }
    // Actual blocking yield so Core 0 streamTask, Wi-Fi/lwIP, and IDLE0 never starve
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// ---------------------------------------------------------------------------
// OLED Task (Core 1): Dedicated non-blocking 128x64 I2C0 display updater
// ---------------------------------------------------------------------------
void oledTask(void*) {
  uint32_t lastSeq = 0;
  uint32_t lastBlinkRendered = 0;
  uint32_t winStartMs = millis();
  uint32_t winFrames = 0;
  uint8_t* grayCopy = static_cast<uint8_t*>(
      heap_caps_malloc(kDecGrayCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  flipcam::OledFrameBuffer localFb;

  for (;;) {
    uint32_t curSeq = 0;
    int gw = 0, gh = 0;
    bool copied = false;
    if (xSemaphoreTake(g_slotMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      curSeq = g_monoSeq;
      if (curSeq != lastSeq && grayCopy && g_oledSlotGray && g_oledSlotW > 0 && g_oledSlotH > 0) {
        gw = g_oledSlotW;
        gh = g_oledSlotH;
        std::memcpy(grayCopy, g_oledSlotGray, static_cast<size_t>(gw) * static_cast<size_t>(gh));
        copied = true;
      }
      xSemaphoreGive(g_slotMutex);
    }

    const uint32_t now = millis();
    const bool blinkActive = (now < g_oledBlinkUntilMs);
    if (copied || !xiao_hw::cameraInfo().ok || blinkActive != (lastBlinkRendered != 0)) {
      lastBlinkRendered = blinkActive ? 1u : 0u;
      flipcam::OledRenderContext octx{};
      octx.linkConnected = (WiFi.status() == WL_CONNECTED);
      octx.cameraOk = xiao_hw::cameraInfo().ok;
      octx.d1Pressed = g_d1Button.snap().active;
      octx.freeMiB = static_cast<float>(g_store.status().freeBytes) / 1048576.0f;
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        octx.look = g_settings.look;
        octx.contrast = g_settings.contrast;
        octx.mirror = g_settings.mirror != 0;
        octx.orientation = g_settings.orientation;
        xSemaphoreGive(g_stateMutex);
      }
      octx.blinkUntilMs = g_oledBlinkUntilMs;
      octx.nowMs = now;
      octx.previewGray = copied ? grayCopy : nullptr;
      octx.previewW = gw;
      octx.previewH = gh;

      flipcam::renderOledFrame(localFb, octx);
      const uint32_t crc = localFb.crc32();
      if (xSemaphoreTake(g_oledFbMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        g_oledFb = localFb;
        g_lastOledCrc = crc;
        xSemaphoreGive(g_oledFbMutex);
      }
      if (xiao_hw::oledPresent(localFb) && copied) {
        lastSeq = curSeq;
        winFrames++;
      }
      if (now - winStartMs >= 1000) {
        g_oledFpsX10 = (winFrames * 10000u) / std::max<uint32_t>(1u, now - winStartMs);
        winStartMs = now;
        winFrames = 0;
      }
    }
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(copied ? 10 : 25));
  }
}

// ---------------------------------------------------------------------------
// Persistent Binary TCP Preview Stream Task (Core 0, port 8081)
// ---------------------------------------------------------------------------
void streamTask(void*) {
  g_streamServer.begin();
  g_streamServer.setNoDelay(true);
  std::vector<uint8_t> rawCopy(flipcam::kFmbRawPlaneBytes);
  std::vector<uint8_t> payload;
  payload.reserve(flipcam::kFmbRawPlaneBytes);

  for (;;) {
    WiFiClient client = g_streamServer.accept();
    if (!client) {
      delay(20);
      continue;
    }
    client.setNoDelay(true);

    // Bounded 1s AUTH handshake
    char authLine[96]{};
    size_t authLen = 0;
    const uint32_t authDeadline = millis() + 1000u;
    bool gotLine = false;
    while (client.connected() && millis() < authDeadline) {
      if (client.available() > 0) {
        const int ch = client.read();
        if (ch == '\n') {
          gotLine = true;
          break;
        }
        if (ch != '\r' && authLen + 1 < sizeof(authLine)) {
          authLine[authLen++] = static_cast<char>(ch);
        }
      } else {
        delay(2);
      }
    }
    authLine[authLen] = '\0';
    char expectedAuth[96];
    std::snprintf(expectedAuth, sizeof(expectedAuth), "AUTH %s", FLIPCAM_PAIR_TOKEN);
    if (!gotLine || std::strcmp(authLine, expectedAuth) != 0) {
      client.stop();
      continue;
    }

    uint32_t lastSentSeq = 0;
    uint32_t winStartMs = millis();
    uint32_t winSent = 0;

    while (client.connected() && WiFi.status() == WL_CONNECTED) {
      uint32_t seq = 0, capMs = 0;
      uint8_t flags = 0;
      bool haveNew = false;
      if (xSemaphoreTake(g_slotMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (g_monoSeq != lastSentSeq && g_monoSeq > 0) {
          std::memcpy(rawCopy.data(), g_monoSlotPlane, flipcam::kFmbRawPlaneBytes);
          seq = g_monoSeq;
          capMs = g_monoCaptureMs;
          flags = g_monoFlags;
          haveNew = true;
        }
        xSemaphoreGive(g_slotMutex);
      }
      if (!haveNew) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(25));
        continue;
      }

      flipcam::FmbFrameHeader hdr{};
      const uint32_t now = millis();
      flipcam::encodeFmbFrame(rawCopy.data(), seq, now >= capMs ? (now - capMs) : 0,
                              flags, hdr, payload);

      const size_t w1 = client.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
      const size_t w2 = (w1 == sizeof(hdr)) ? client.write(payload.data(), payload.size()) : 0;
      if (w1 != sizeof(hdr) || w2 != payload.size()) {
        break;
      }
      lastSentSeq = seq;
      g_streamFramesSent++;
      g_streamBytesSent += (w1 + w2);
      if (hdr.codec == static_cast<uint8_t>(flipcam::FmbCodec::PACKBITS_RLE_1BIT)) {
        g_streamRleHits++;
      } else {
        g_streamRawFallbacks++;
      }
      winSent++;
      if (now - winStartMs >= 1000) {
        g_streamFpsX10 = (winSent * 10000u) / std::max<uint32_t>(1u, now - winStartMs);
        winStartMs = now;
        winSent = 0;
      }
    }
    client.stop();
    g_streamFpsX10 = 0;
  }
}

void setupPeerServer() {
  static const char* kPairHeader[] = {"X-FlipCam-Pair"};
  g_peerServer.collectHeaders(kPairHeader, 1);

  g_peerServer.on("/peer/info", HTTP_GET, []() {
    if (!checkPairAuth()) {
      g_peerServer.send(401, "application/json", "{\"error\":\"unauthorized\"}");
      return;
    }
    const auto ci = xiao_hw::cameraInfo();
    uint8_t orient = 0;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      orient = g_settings.orientation;
      xSemaphoreGive(g_stateMutex);
    }
    JsonDocument doc;
    doc["cam_ok"] = ci.ok;
    doc["pid"] = ci.pid;
    doc["sensor"] = ci.sensorName;
    doc["preview_res"] = ci.previewRes;
    doc["still_res"] = ci.stillRes;
    doc["stream_port"] = FLIPCAM_STREAM_PORT;
    doc["mic_ok"] = false;
    doc["photos_only"] = true;
    doc["oled_ok"] = xiao_hw::oledIsOk();
    doc["d1_raw"] = digitalRead(kPinD1Button);
    doc["orientation"] = flipcam::orientationName(orient);
    doc["fw"] = flipcam::kFwVersion;
    String out;
    serializeJson(doc, out);
    g_peerServer.sendHeader("Connection", "close");
    g_peerServer.send(200, "application/json", out);
  });

  g_peerServer.on("/peer/config", HTTP_POST, []() {
    if (!checkPairAuth()) {
      g_peerServer.send(401, "text/plain", "unauthorized");
      return;
    }
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      const flipcam::FlipCamSettings prev = g_settings;
      if (g_peerServer.hasArg("look")) {
        g_settings.look = (g_peerServer.arg("look") == "threshold") ? 1 : 0;
      }
      if (g_peerServer.hasArg("contrast")) {
        const String c = g_peerServer.arg("contrast");
        if (c == "low") g_settings.contrast = 0;
        else if (c == "normal") g_settings.contrast = 1;
        else g_settings.contrast = 2;
      }
      if (g_peerServer.hasArg("mirror")) {
        g_settings.mirror = (g_peerServer.arg("mirror").toInt() != 0) ? 1 : 0;
      }
      if (g_peerServer.hasArg("orientation")) {
        const String o = g_peerServer.arg("orientation");
        if (o == "rear" || o == "rear180" || o == "1") {
          g_settings.orientation = static_cast<uint8_t>(flipcam::OrientationMode::REAR);
        } else {
          g_settings.orientation = static_cast<uint8_t>(flipcam::OrientationMode::SELFIE);
        }
      }
      if (g_settings != prev) {
        g_settingsDirty = true;
      }
      xSemaphoreGive(g_stateMutex);
    }
    g_peerServer.sendHeader("Connection", "close");
    g_peerServer.send(200, "application/json", "{\"ok\":true}");
  });

  g_peerServer.on("/peer/preview.jpg", HTTP_GET, []() {
    if (!checkPairAuth()) {
      g_peerServer.send(401, "text/plain", "unauthorized");
      return;
    }
    std::vector<uint8_t> jpg;
    uint16_t w = 0, h = 0;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
      jpg = g_cachedPreview;
      w = g_cachedPreviewW;
      h = g_cachedPreviewH;
      xSemaphoreGive(g_stateMutex);
    }
    if (jpg.empty()) {
      g_peerServer.sendHeader("Connection", "close");
      g_peerServer.send(503, "text/plain", "no frame");
      return;
    }
    const uint32_t crc = flipcam::crc32Compute(jpg.data(), jpg.size());
    g_peerServer.sendHeader("Connection", "close");
    g_peerServer.sendHeader("X-Width", String(w));
    g_peerServer.sendHeader("X-Height", String(h));
    g_peerServer.sendHeader("X-Frame-Crc32", String(crc, HEX));
    g_peerServer.send_P(200, "image/jpeg", reinterpret_cast<const char*>(jpg.data()), jpg.size());
  });

  g_peerServer.on("/peer/capture", HTTP_POST, []() {
    if (!checkPairAuth()) {
      g_peerServer.send(401, "text/plain", "unauthorized");
      return;
    }
    const uint32_t nonce = static_cast<uint32_t>(std::strtoul(g_peerServer.arg("nonce").c_str(), nullptr, 10));
    const uint32_t reqId = static_cast<uint32_t>(std::max<long>(0, g_peerServer.arg("id").toInt()));
    const bool synth = (g_peerServer.arg("synth").toInt() != 0);
    uint8_t orient = 0;
    bool soundOn = true;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      orient = g_settings.orientation;
      soundOn = (g_settings.sound != 0);
      xSemaphoreGive(g_stateMutex);
    }
    if (g_peerServer.hasArg("orient")) {
      const String o = g_peerServer.arg("orient");
      if (o == "rear" || o == "rear180" || o == "1") {
        orient = static_cast<uint8_t>(flipcam::OrientationMode::REAR);
      } else if (o == "selfie" || o == "0") {
        orient = static_cast<uint8_t>(flipcam::OrientationMode::SELFIE);
      }
    }
    std::vector<uint8_t> jpg;
    uint16_t w = 0, h = 0;
    uint32_t crc = 0;
    if (!xiao_hw::cameraCaptureStill(nonce, reqId, synth, g_synthSeq++, orient, jpg, w, h, crc)) {
      g_peerServer.sendHeader("Connection", "close");
      g_peerServer.send(500, "text/plain", "capture failed");
      return;
    }
    g_oledBlinkUntilMs = millis() + 130;
    buzzerChirp(soundOn, 1047, 45);
    g_peerServer.sendHeader("Connection", "close");
    g_peerServer.sendHeader("X-Width", String(w));
    g_peerServer.sendHeader("X-Height", String(h));
    g_peerServer.sendHeader("X-Capture-Id", String(reqId));
    g_peerServer.sendHeader("X-Capture-Crc32", String(crc, HEX));
    g_peerServer.sendHeader("X-Synthetic", synth ? "1" : "0");
    g_peerServer.send_P(200, "image/jpeg", reinterpret_cast<const char*>(jpg.data()), jpg.size());
  });

  g_peerServer.on("/peer/rec/start", HTTP_POST, []() {
    if (!checkPairAuth()) { g_peerServer.send(401, "text/plain", "unauthorized"); return; }
    g_peerServer.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });
  g_peerServer.on("/peer/rec/poll", HTTP_GET, []() {
    if (!checkPairAuth()) { g_peerServer.send(401, "text/plain", "unauthorized"); return; }
    g_peerServer.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });
  g_peerServer.on("/peer/rec/stop", HTTP_POST, []() {
    if (!checkPairAuth()) { g_peerServer.send(401, "text/plain", "unauthorized"); return; }
    g_peerServer.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });

  g_peerServer.begin();
}

void sendUsbStatus(bool extended) {
  const auto ci = xiao_hw::cameraInfo();
  JsonDocument doc;
  doc["board"] = "xiao";
  doc["photos_only"] = true;
  doc["fw"] = flipcam::kFwVersion;
  doc["build"] = __DATE__ " " __TIME__;
  doc["pair_cfg"] = FLIPCAM_PAIR_CONFIG_IS_DUMMY ? "example-dummy" : "private";
  doc["flash_mb"] = ESP.getFlashChipSize() / (1024u * 1024u);
  doc["psram_total"] = ESP.getPsramSize();
  doc["psram_free"] = ESP.getFreePsram();
  doc["heap_free"] = ESP.getFreeHeap();
  doc["heap_min"] = ESP.getMinFreeHeap();

  const esp_partition_t* mp = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, flipcam::kMediaPartitionLabel);
  doc["media_part_found"] = (mp != nullptr);
  doc["media_part_offset"] = mp ? mp->address : 0;
  doc["media_part_size"] = mp ? mp->size : 0;

  if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    g_store.refreshSpace();
    const auto& ss = g_store.status();
    doc["fs_mounted"] = ss.mounted;
    doc["fs_total"] = ss.totalBytes;
    doc["fs_used"] = ss.usedBytes;
    doc["fs_free"] = ss.freeBytes;
    doc["fs_fault"] = ss.fault;

    auto med = doc["media"].to<JsonObject>();
    med["count"] = static_cast<uint32_t>(g_store.stillCount());
    const flipcam::MediaEntry* latest = g_store.getStillNewestFirst(0);
    med["latest_id"] = latest ? latest->id : 0;
    med["latest_crc32"] = latest ? latest->crc32 : 0;
    med["latest_bytes"] = latest ? latest->sizeBytes : 0;
    xSemaphoreGive(g_fsMutex);
  }

  doc["link"] = (WiFi.status() == WL_CONNECTED) ? "connected" : "reconnecting";
  doc["rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  doc["reconnects"] = g_wifiReconnects;
  uint8_t statusOrient = 0;
  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    statusOrient = g_settings.orientation;
    xSemaphoreGive(g_stateMutex);
  }
  doc["orientation"] = flipcam::orientationName(statusOrient);
  doc["test_mode"] = g_d1Button.isTestMode();

  auto btns = doc["buttons"].to<JsonObject>();
  btns["gpio2_d1_raw"] = digitalRead(kPinD1Button);
  btns["d1_phys_press"] = g_d1Button.snap().physPressCount;
  btns["d1_phys_tap"] = g_d1Button.snap().physTapCount;
  btns["d1_inj_press"] = g_d1Button.snap().injPressCount;
  btns["d1_inj_tap"] = g_d1Button.snap().injTapCount;
  btns["boot_suppressed"] = g_d1Button.snap().bootSuppressed;

  doc["prod_captures"] = ci.prodCaptures;
  doc["synth_captures"] = ci.synthCaptures;

  auto cam = doc["camera"].to<JsonObject>();
  cam["ok"] = ci.ok;
  cam["sensor"] = ci.sensorName;
  cam["pid"] = ci.pid;
  cam["preview_res"] = ci.previewRes;
  cam["still_res"] = ci.stillRes;
  cam["frames_ok"] = ci.framesOk;
  cam["frames_err"] = ci.framesErr;
  cam["mono_encoded"] = g_monoFramesEncoded;
  cam["grab_us"] = g_camGrabUsAvg;
  cam["mono_encode_us"] = g_monoEncodeUsAvg;
  cam["prod_captures"] = ci.prodCaptures;
  cam["synth_captures"] = ci.synthCaptures;
  cam["fault"] = ci.fault;

  auto stream = doc["stream"].to<JsonObject>();
  stream["port"] = FLIPCAM_STREAM_PORT;
  stream["frames_sent"] = g_streamFramesSent;
  stream["bytes_sent"] = g_streamBytesSent;
  stream["rle_hits"] = g_streamRleHits;
  stream["raw_fallbacks"] = g_streamRawFallbacks;
  stream["fps_x10"] = g_streamFpsX10;

  auto disp = doc["display"].to<JsonObject>();
  disp["oled_ok"] = xiao_hw::oledIsOk();
  disp["oled_frames"] = xiao_hw::oledFrameCount();
  disp["oled_fps_x10"] = g_oledFpsX10;
  disp["fb_crc32"] = g_lastOledCrc;

  auto aud = doc["audio"].to<JsonObject>();
  aud["mic_ok"] = false;
  aud["mic_disabled_photos_only"] = true;
  aud["buzzer_gpio"] = kPinD3Buzzer;

  auto bat = doc["battery"].to<JsonObject>();
  bat["supported"] = false;
  bat["present"] = false;
  bat["pct"] = -1;

  if (extended) {
    doc["cached_preview_bytes"] = static_cast<uint32_t>(g_cachedPreview.size());
  }
  doc["loop_stack_hwm"] = static_cast<uint32_t>(uxTaskGetStackHighWaterMark(nullptr));
  doc["cam_stack_hwm"] = g_camTaskHandle ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(g_camTaskHandle)) : 0;
  doc["oled_stack_hwm"] = g_oledTaskHandle ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(g_oledTaskHandle)) : 0;
  doc["stream_stack_hwm"] = g_streamTaskHandle ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(g_streamTaskHandle)) : 0;

  serializeJson(doc, Serial);
  Serial.println();
}

void handleUsbLine(const char* line) {
  const auto cmd = flipcam::parseUsbCommand(line);
  switch (cmd.kind) {
    case flipcam::UsbCmdKind::STATUS:
      sendUsbStatus(false);
      break;
    case flipcam::UsbCmdKind::DIAG:
      sendUsbStatus(true);
      break;
    case flipcam::UsbCmdKind::CAPTURE: {
      const bool ok = performLocalSpoolCapture(false);
      Serial.printf("{\"ok\":%s,\"cmd\":\"CAPTURE\"}\n", ok ? "true" : "false");
      break;
    }
    case flipcam::UsbCmdKind::CAPTURE_SYNTH: {
      if (!g_d1Button.isTestMode()) {
        Serial.println("{\"ok\":false,\"error\":\"TESTMODE_REQUIRED\"}");
        break;
      }
      const bool ok = performLocalSpoolCapture(true);
      Serial.printf("{\"ok\":%s,\"cmd\":\"CAPTURE SYNTH\"}\n", ok ? "true" : "false");
      break;
    }
    case flipcam::UsbCmdKind::LIST: {
      JsonDocument doc;
      auto arr = doc["items"].to<JsonArray>();
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
        for (const auto& e : g_store.entries()) {
          if (e.type != static_cast<uint8_t>(flipcam::MediaType::STILL)) continue;
          auto o = arr.add<JsonObject>();
          o["id"] = e.id;
          o["type"] = "still";
          o["width"] = e.width;
          o["height"] = e.height;
          o["bytes"] = e.sizeBytes;
          o["crc32"] = e.crc32;
          o["synthetic"] = e.synthetic != 0;
        }
        xSemaphoreGive(g_fsMutex);
      }
      serializeJson(doc, Serial);
      Serial.println();
      break;
    }
    case flipcam::UsbCmdKind::RECORD_START:
    case flipcam::UsbCmdKind::RECORD_STOP:
    case flipcam::UsbCmdKind::PLAY:
    case flipcam::UsbCmdKind::STOP:
      Serial.println("{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
      break;
    case flipcam::UsbCmdKind::DELETE_ID: {
      bool ok = false;
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        ok = g_store.deleteById(cmd.num1);
        xSemaphoreGive(g_fsMutex);
      }
      Serial.printf("{\"ok\":%s,\"cmd\":\"DELETE\",\"id\":%u}\n",
                    ok ? "true" : "false", static_cast<unsigned>(cmd.num1));
      break;
    }
    case flipcam::UsbCmdKind::TONE:
      tone(kPinD3Buzzer, cmd.num1, cmd.num2);
      Serial.printf("{\"ok\":true,\"cmd\":\"TONE\",\"buzzer_gpio\":%d,\"freq\":%u,\"ms\":%u}\n",
                    kPinD3Buzzer, static_cast<unsigned>(cmd.num1), static_cast<unsigned>(cmd.num2));
      break;
    case flipcam::UsbCmdKind::SET_SETTING: {
      bool ok = true;
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        flipcam::FlipCamSettings next = g_settings;
        if (std::strcmp(cmd.arg1, "look") == 0) {
          if (std::strcmp(cmd.arg2, "dither") == 0) next.look = 0;
          else if (std::strcmp(cmd.arg2, "threshold") == 0) next.look = 1;
          else ok = false;
        } else if (std::strcmp(cmd.arg1, "contrast") == 0) {
          if (std::strcmp(cmd.arg2, "low") == 0) next.contrast = 0;
          else if (std::strcmp(cmd.arg2, "normal") == 0) next.contrast = 1;
          else if (std::strcmp(cmd.arg2, "high") == 0) next.contrast = 2;
          else ok = false;
        } else if (std::strcmp(cmd.arg1, "mirror") == 0) {
          if (std::strcmp(cmd.arg2, "1") == 0 || std::strcmp(cmd.arg2, "on") == 0 || std::strcmp(cmd.arg2, "true") == 0) next.mirror = 1;
          else if (std::strcmp(cmd.arg2, "0") == 0 || std::strcmp(cmd.arg2, "off") == 0 || std::strcmp(cmd.arg2, "false") == 0) next.mirror = 0;
          else ok = false;
        } else if (std::strcmp(cmd.arg1, "sound") == 0) {
          if (std::strcmp(cmd.arg2, "1") == 0 || std::strcmp(cmd.arg2, "on") == 0 || std::strcmp(cmd.arg2, "true") == 0) next.sound = 1;
          else if (std::strcmp(cmd.arg2, "0") == 0 || std::strcmp(cmd.arg2, "off") == 0 || std::strcmp(cmd.arg2, "false") == 0) next.sound = 0;
          else ok = false;
        } else if (std::strcmp(cmd.arg1, "orientation") == 0) {
          if (std::strcmp(cmd.arg2, "selfie") == 0 || std::strcmp(cmd.arg2, "0") == 0) {
            next.orientation = static_cast<uint8_t>(flipcam::OrientationMode::SELFIE);
          } else if (std::strcmp(cmd.arg2, "rear") == 0 || std::strcmp(cmd.arg2, "rear180") == 0 || std::strcmp(cmd.arg2, "1") == 0) {
            next.orientation = static_cast<uint8_t>(flipcam::OrientationMode::REAR);
          } else {
            ok = false;
          }
        } else {
          ok = false;
        }
        if (ok && next != g_settings) {
          g_settings = next;
          g_settingsDirty = true;
        }
        xSemaphoreGive(g_stateMutex);
      } else {
        ok = false;
      }
      Serial.printf("{\"ok\":%s,\"cmd\":\"SET\"}\n", ok ? "true" : "false");
      break;
    }
    case flipcam::UsbCmdKind::GET_SETTINGS: {
      flipcam::FlipCamSettings snap{};
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        snap = g_settings;
        xSemaphoreGive(g_stateMutex);
      }
      Serial.printf("{\"look\":\"%s\",\"contrast\":\"%s\",\"mirror\":%u,\"sound\":%u,\"orientation\":\"%s\"}\n",
                    flipcam::lookName(snap.look),
                    flipcam::contrastName(snap.contrast),
                    snap.mirror, snap.sound,
                    flipcam::orientationName(snap.orientation));
      break;
    }
    case flipcam::UsbCmdKind::SCREENSHOT: {
      std::vector<uint8_t> snap(flipcam::OledFrameBuffer::kBytes);
      if (xSemaphoreTake(g_oledFbMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        std::memcpy(snap.data(), g_oledFb.bytes, snap.size());
        xSemaphoreGive(g_oledFbMutex);
      } else {
        Serial.println("{\"ok\":false,\"cmd\":\"SCREENSHOT\",\"error\":\"BUSY\"}");
        break;
      }
      const uint32_t crc = flipcam::crc32Compute(snap.data(), snap.size());
      const std::string b64 = flipcam::encodeBase64(snap.data(), snap.size());
      Serial.printf("SCREENSHOT 128 64 %u %08x\n", static_cast<unsigned>(snap.size()),
                    static_cast<unsigned>(crc));
      for (size_t off = 0; off < b64.size(); off += 120) {
        const size_t n = std::min<size_t>(120, b64.size() - off);
        Serial.write(reinterpret_cast<const uint8_t*>(b64.data() + off), n);
        Serial.println();
      }
      Serial.printf("SCREENSHOT_END %08x\n", static_cast<unsigned>(crc));
      break;
    }
    case flipcam::UsbCmdKind::TESTMODE_ON:
      g_d1Button.setTestMode(true, millis());
      Serial.println("{\"ok\":true,\"test_mode\":true}");
      break;
    case flipcam::UsbCmdKind::TESTMODE_OFF:
      g_d1Button.setTestMode(false, millis());
      Serial.println("{\"ok\":true,\"test_mode\":false}");
      break;
    case flipcam::UsbCmdKind::INJECT: {
      if (!g_d1Button.isTestMode()) {
        Serial.println("{\"ok\":false,\"error\":\"TESTMODE_REQUIRED\"}");
        break;
      }
      bool ok = false;
      flipcam::ButtonEvents ev{};
      const uint32_t now = millis();
      if (flipcam::eqUpper(cmd.arg1, "SNAP") || flipcam::eqUpper(cmd.arg1, "D1")) {
        if (flipcam::eqUpper(cmd.arg2, "DOWN")) ok = g_d1Button.injectDown(flipcam::ButtonId::SNAP, now);
        else if (flipcam::eqUpper(cmd.arg2, "UP")) ok = g_d1Button.injectUp(flipcam::ButtonId::SNAP, now, ev);
        else if (flipcam::eqUpper(cmd.arg2, "CANCEL")) ok = g_d1Button.injectCancel(flipcam::ButtonId::SNAP);
        else if (flipcam::eqUpper(cmd.arg2, "TAP")) {
          ok = g_d1Button.injectDown(flipcam::ButtonId::SNAP, now) &&
               g_d1Button.injectUp(flipcam::ButtonId::SNAP, now + 40, ev);
        } else if (flipcam::eqUpper(cmd.arg2, "HOLD")) {
          if (g_d1Button.injectDown(flipcam::ButtonId::SNAP, now)) {
            ev = g_d1Button.update(false, false, now + flipcam::ButtonController::kHoldMs + 50);
            flipcam::ButtonEvents upEv{};
            ok = g_d1Button.injectUp(flipcam::ButtonId::SNAP, now + flipcam::ButtonController::kHoldMs + 60, upEv);
            ev.snapTap |= upEv.snapTap;
          }
        }
      }
      if (ev.snapTap) performLocalSpoolCapture(false);
      Serial.printf("{\"ok\":%s,\"cmd\":\"INJECT\"}\n", ok ? "true" : "false");
      break;
    }
    case flipcam::UsbCmdKind::READ_MEDIA: {
      const size_t reqLen = static_cast<size_t>(std::strtoul(cmd.arg1, nullptr, 10));
      std::vector<uint8_t> chunk;
      size_t totalSz = 0;
      bool ok = false;
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        ok = g_store.readMediaChunk(cmd.num1, cmd.num2, reqLen, chunk, totalSz);
        xSemaphoreGive(g_fsMutex);
      }
      if (!ok) {
        Serial.println("{\"ok\":false,\"cmd\":\"READ\",\"error\":\"NOT_FOUND_OR_BOUNDS\"}");
      } else {
        const uint32_t crc = flipcam::crc32Compute(chunk.data(), chunk.size());
        const std::string b64 = flipcam::encodeBase64(chunk.data(), chunk.size());
        Serial.printf("READ_DATA %u %u %u %u %08x %s\n",
                      static_cast<unsigned>(cmd.num1), static_cast<unsigned>(cmd.num2),
                      static_cast<unsigned>(chunk.size()), static_cast<unsigned>(totalSz),
                      static_cast<unsigned>(crc), b64.c_str());
      }
      break;
    }
    case flipcam::UsbCmdKind::FORMAT_MEDIA: {
      bool ok = false;
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
        ok = g_store.guardedFormat(cmd.arg1, false);
        xSemaphoreGive(g_fsMutex);
      }
      Serial.printf("{\"ok\":%s,\"cmd\":\"FORMAT_MEDIA\",\"fault\":\"%s\"}\n",
                    ok ? "true" : "false", g_store.status().fault);
      break;
    }
    case flipcam::UsbCmdKind::REBOOT:
      Serial.println("{\"ok\":true,\"cmd\":\"REBOOT\"}");
      Serial.flush();
      g_rebootRequested = true;
      break;
    default:
      Serial.println("{\"ok\":false,\"error\":\"UNKNOWN_CMD\"}");
      break;
  }
}

void pollUsbSerial() {
  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) break;
    if (ch == '\n' || ch == '\r') {
      if (g_usbLen > 0) {
        g_usbBuf[g_usbLen] = '\0';
        handleUsbLine(g_usbBuf);
        g_usbLen = 0;
      }
    } else if (g_usbLen + 1 < sizeof(g_usbBuf)) {
      g_usbBuf[g_usbLen++] = static_cast<char>(ch);
    }
  }
}

void maintainWifiSta(uint32_t now) {
  const bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected) {
    if (!g_wasWifiConnected) {
      g_wasWifiConnected = true;
      g_wifiBackoffMs = 500;
    }
    return;
  }
  if (g_wasWifiConnected) {
    g_wasWifiConnected = false;
    g_wifiReconnects++;
  }
  if (now - g_lastWifiAttemptMs >= g_wifiBackoffMs) {
    g_lastWifiAttemptMs = now;
    WiFi.disconnect(false, false);
    WiFi.begin(FLIPCAM_WIFI_SSID, FLIPCAM_WIFI_PSK);
    g_wifiBackoffMs = std::min<uint32_t>(4000u, g_wifiBackoffMs * 2u);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  g_stateMutex = xSemaphoreCreateMutex();
  g_fsMutex = xSemaphoreCreateMutex();
  g_slotMutex = xSemaphoreCreateMutex();
  g_oledFbMutex = xSemaphoreCreateMutex();

  pinMode(kPinD1Button, INPUT_PULLUP);
  pinMode(kPinD3Buzzer, OUTPUT);
  digitalWrite(kPinD3Buzzer, LOW);
  // XIAO D1 explicitly disables long-hold so only RLCD SNAP owns the Selfie/Rear180 hold gesture
  g_d1Button.init(digitalRead(kPinD1Button) == LOW, false, millis(), false, false);

  xiao_hw::oledBegin();
  xiao_hw::cameraBegin();

  g_decScratch = static_cast<uint8_t*>(heap_caps_malloc(kDecGrayCap, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!g_decScratch) g_decScratch = static_cast<uint8_t*>(heap_caps_malloc(kDecGrayCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  g_monoSlotPlane = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  g_oledSlotGray = static_cast<uint8_t*>(heap_caps_malloc(kDecGrayCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  if (g_store.beginNoAutoFormat()) {
    flipcam::FlipCamSettings loaded{};
    if (g_store.loadSettings(loaded)) {
      g_settings = loaded;
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // Disable Wi-Fi modem sleep for >=12fps streaming latency
  WiFi.setAutoReconnect(true);
  const IPAddress localIp(FLIPCAM_XIAO_IP_A, FLIPCAM_XIAO_IP_B, FLIPCAM_XIAO_IP_C, FLIPCAM_XIAO_IP_D);
  const IPAddress gw(FLIPCAM_RLCD_IP_A, FLIPCAM_RLCD_IP_B, FLIPCAM_RLCD_IP_C, FLIPCAM_RLCD_IP_D);
  const IPAddress mask(255, 255, 255, 0);
  WiFi.config(localIp, gw, mask);
  WiFi.begin(FLIPCAM_WIFI_SSID, FLIPCAM_WIFI_PSK);
  g_lastWifiAttemptMs = millis();

  setupPeerServer();

  xTaskCreatePinnedToCore(camProducerTask, "xiao_cam", 8192, nullptr, 1, &g_camTaskHandle, 0);
  xTaskCreatePinnedToCore(streamTask, "xiao_stream", 6144, nullptr, 2, &g_streamTaskHandle, 0);
  xTaskCreatePinnedToCore(oledTask, "xiao_oled", 4096, nullptr, 1, &g_oledTaskHandle, 1);
}

void loop() {
  static uint32_t lastBtnMs = 0;
  static uint32_t lastSettingsMs = 0;

  pollUsbSerial();
  g_peerServer.handleClient();

  const uint32_t now = millis();
  maintainWifiSta(now);

  if (g_rebootRequested) {
    flipcam::FlipCamSettings snap{};
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      snap = g_settings;
      xSemaphoreGive(g_stateMutex);
    }
    if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
      g_store.saveSettingsIfChanged(snap);
      LittleFS.end();
      xSemaphoreGive(g_fsMutex);
    }
    delay(100);
    ESP.restart();
  }

  if (now - lastBtnMs >= 5) {
    lastBtnMs = now;
    const bool d1Raw = (digitalRead(kPinD1Button) == LOW);
    const auto ev = g_d1Button.update(d1Raw, false, now);
    if (ev.snapTap) {
      performLocalSpoolCapture(false);
    }
  }

  if (now - lastSettingsMs >= 500) {
    lastSettingsMs = now;
    flipcam::FlipCamSettings copy{};
    bool dirty = false;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      dirty = g_settingsDirty;
      if (dirty) {
        copy = g_settings;
        g_settingsDirty = false;
      }
      xSemaphoreGive(g_stateMutex);
    }
    if (dirty && xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      g_store.saveSettingsIfChanged(copy);
      xSemaphoreGive(g_fsMutex);
    }
  }
  delay(2);
}
