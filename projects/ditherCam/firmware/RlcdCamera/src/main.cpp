// SPDX-License-Identifier: MIT
// Waveshare ESP32-S3-RLCD-4.2 Firmware Entrypoint (SoftAP + Gallery + Audio + UI)
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "FlipCamButtons.h"
#include "FlipCamCrc.h"
#include "FlipCamDisplay.h"
#include "FlipCamJpegDecode.h"
#include "FlipCamLittleFsBackend.h"
#include "FlipCamPairConfig.h"
#include "FlipCamProtocol.h"
#include "FlipCamStorage.h"
#include "FlipCamStream.h"
#include "FlipCamTypes.h"
#include "FlipCamUiState.h"
#include "FlipCamUsbProtocol.h"

#include "AudioCodec.h"
#include "BatteryAdc.h"
#include "BoardPanel.h"
#include "WebGallery.h"

namespace {

constexpr int kPinKeySnap = 18;    // Active LOW
constexpr int kPinBootBrowse = 0;  // Active LOW

SemaphoreHandle_t g_stateMutex = nullptr;
SemaphoreHandle_t g_fsMutex = nullptr;

flipcam::ButtonController g_buttons;
flipcam::FlipCamUiState g_ui;
flipcam::LittleFsBackend g_fsBackend;
flipcam::MediaStore g_store(g_fsBackend);
flipcam::RlcdFrameBuffer g_fb;
uint32_t g_lastFbCrc = 0;

WebServer g_web(FLIPCAM_WEB_PORT);

// PSRAM/DRAM buffers for fast 1-bit stream and gallery still
constexpr size_t kGrayCap = 400u * 300u;
uint32_t g_previewFramesOk = 0;
uint32_t g_previewFramesErr = 0;
uint32_t g_lastPreviewMs = 0;

uint8_t* g_liveMonoPlane = nullptr;
uint8_t* g_liveMonoScratch = nullptr;
uint8_t* g_renderMonoSnap = nullptr;
bool g_hasLiveMono = false;
uint32_t g_rxFramesOk = 0;
uint32_t g_rxFramesErr = 0;
uint32_t g_rxSeq = 0;
uint32_t g_lastFrameAgeMs = 0;
uint32_t g_rxFpsX10 = 0;
uint64_t g_rxBytesTotal = 0;
uint32_t g_streamConnects = 0;
uint32_t g_rxLockMisses = 0;
uint32_t g_lastHttpInfoMs = 0;
uint32_t g_maxHttpInfoMs = 0;
uint32_t g_lastCapHttpMs = 0;

uint32_t g_presentNewFrames = 0;
uint32_t g_presentFpsX10 = 0;
uint32_t g_lastPresentedSeq = 0;
uint32_t g_presentWinStartMs = 0;
uint32_t g_presentWinCount = 0;
uint32_t g_settingsGen = 1;
uint32_t g_peerSyncedGen = 0;
volatile bool g_peerConfigDirty = true;

uint8_t* g_galleryGray = nullptr;
uint8_t* g_galleryScratchGray = nullptr;
int g_galleryW = 0;
int g_galleryH = 0;
uint32_t g_loadedGalleryStillId = 0;
flipcam::MediaEntry g_cachedCurrentStill{};
bool g_hasCachedCurrentStill = false;
uint64_t g_cachedFreeBytes = 0;

// Peer snapshot info (from XIAO /peer/info)
bool g_peerCamOk = false;
bool g_peerMicOk = false;
char g_peerSensor[24] = "unknown";
uint16_t g_peerPid = 0;
char g_peerPreviewRes[16] = "";
char g_peerStillRes[16] = "";
char g_peerOrientation[16] = "unknown";
uint32_t g_peerReconnects = 0;

// Worker queues / flags (guarded by g_stateMutex)
volatile uint32_t g_pendingCaptureReqId = 0;
volatile bool g_pendingCaptureSynth = false;
uint8_t g_pendingCaptureOrientation = 0;
uint32_t g_nextReqId = 1;
uint32_t g_bootNonce = 0;
TaskHandle_t g_netTaskHandle = nullptr;

volatile bool g_recStartPending = false;
volatile bool g_recStopPending = false;
volatile bool g_recActiveWorker = false;

volatile uint32_t g_playReqClipId = 0;
volatile bool g_playStopReq = false;

volatile bool g_rebootRequested = false;
volatile bool g_forceRender = true;
char g_lastError[48] = "";

// Serial USB line buffer
char g_usbBuf[160];
size_t g_usbLen = 0;

String peerUrl(const char* path) {
  char buf[96];
  std::snprintf(buf, sizeof(buf), "http://%d.%d.%d.%d:%d%s",
                FLIPCAM_XIAO_IP_A, FLIPCAM_XIAO_IP_B, FLIPCAM_XIAO_IP_C, FLIPCAM_XIAO_IP_D,
                FLIPCAM_PEER_PORT, path);
  return String(buf);
}

void setLastError(const char* msg) {
  std::snprintf(g_lastError, sizeof(g_lastError), "%s", msg ? msg : "");
}

// Must be called WITHOUT g_stateMutex or g_fsMutex held (lock order: g_fsMutex -> g_stateMutex).
void refreshGalleryBitmapIfNeeded() {
  flipcam::AppMode mode = flipcam::AppMode::LIVE;
  int stillIdx = 0;
  uint32_t loadedId = 0;
  int curW = 0;
  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    mode = g_ui.mode;
    stillIdx = g_ui.stillIndex;
    loadedId = g_loadedGalleryStillId;
    curW = g_galleryW;
    xSemaphoreGive(g_stateMutex);
  }
  if (mode != flipcam::AppMode::GALLERY || !g_galleryGray || !g_galleryScratchGray) return;

  flipcam::MediaEntry entryCopy{};
  bool hasEntry = false;
  std::vector<uint8_t> jpg;
  uint64_t freeB = 0;

  if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    freeB = g_store.status().freeBytes;
    const flipcam::MediaEntry* e = g_store.getStillNewestFirst(stillIdx);
    if (e) {
      hasEntry = true;
      entryCopy = *e;
      if (e->id != loadedId || curW == 0) {
        char path[64];
        if (flipcam::buildSafeMediaPath(flipcam::MediaStore::kBaseDir, e->filename, path, sizeof(path))) {
          g_fsBackend.readAll(path, jpg, flipcam::kMaxStillJpegBytes);
        }
      }
    }
    xSemaphoreGive(g_fsMutex);
  } else {
    return;
  }

  if (!hasEntry) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      g_loadedGalleryStillId = 0;
      g_galleryW = 0;
      g_galleryH = 0;
      g_hasCachedCurrentStill = false;
      g_cachedFreeBytes = freeB;
      xSemaphoreGive(g_stateMutex);
    }
    return;
  }

  if (entryCopy.id == loadedId && curW > 0) return;

  int decW = 0, decH = 0;
  const bool decoded = !jpg.empty() &&
                       flipcam::decodeJpegToGray(jpg.data(), jpg.size(), g_galleryScratchGray, kGrayCap,
                                                 flipcam::RlcdFrameBuffer::kWidth,
                                                 flipcam::RlcdFrameBuffer::kHeight, decW, decH,
                                                 nullptr, true);
  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    if (decoded) {
      std::swap(g_galleryGray, g_galleryScratchGray);
      g_galleryW = decW;
      g_galleryH = decH;
    } else {
      g_galleryW = 0;
      g_galleryH = 0;
    }
    g_loadedGalleryStillId = entryCopy.id;
    g_cachedCurrentStill = entryCopy;
    g_hasCachedCurrentStill = true;
    g_cachedFreeBytes = freeB;
    xSemaphoreGive(g_stateMutex);
  }
}

// Caller MUST already hold g_stateMutex! Never re-acquires g_stateMutex.
void queueCaptureLocked(bool synthetic) {
  if (!g_ui.canStartCapture()) {
    g_ui.showToast(g_ui.captureBlockReason(), millis(), 1800);
    return;
  }
  if (g_pendingCaptureReqId == 0) {
    g_ui.captureBusy = true;
    g_pendingCaptureReqId = g_nextReqId++;
    g_pendingCaptureSynth = synthetic;
    g_pendingCaptureOrientation = g_ui.settings.orientation;
  }
}

void queueCapture(bool synthetic) {
  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    queueCaptureLocked(synthetic);
    xSemaphoreGive(g_stateMutex);
  }
}

// Caller MUST already hold g_stateMutex!
void applyUiCommandLocked(flipcam::UiCommand cmd) {
  if (cmd == flipcam::UiCommand::CAPTURE_STILL) {
    queueCaptureLocked(false);
  }
}

// ---------------------------------------------------------------------------
// Peer HTTP Worker Task (Core 0)
// ---------------------------------------------------------------------------
bool fetchPeerInfo(WiFiClient& client) {
  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(450);
  http.setTimeout(500);
  if (!http.begin(client, peerUrl("/peer/info"))) return false;
  http.addHeader("X-FlipCam-Pair", FLIPCAM_PAIR_TOKEN);
  const int code = http.GET();
  bool ok = false;
  if (code == 200) {
    String payload = http.getString();
    JsonDocument doc;
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      g_peerCamOk = doc["cam_ok"] | false;
      g_peerMicOk = doc["mic_ok"] | false;
      g_peerPid = doc["pid"] | 0;
      std::snprintf(g_peerSensor, sizeof(g_peerSensor), "%s", doc["sensor"] | "unknown");
      std::snprintf(g_peerPreviewRes, sizeof(g_peerPreviewRes), "%s", doc["preview_res"] | "");
      std::snprintf(g_peerStillRes, sizeof(g_peerStillRes), "%s", doc["still_res"] | "");
      std::snprintf(g_peerOrientation, sizeof(g_peerOrientation), "%s", doc["orientation"] | "unknown");
      ok = true;
    }
  }
  http.end();
  return ok;
}

bool pushPeerConfig(WiFiClient& client) {
  uint8_t look = 0, contrast = 2, mirror = 0, orientation = 0;
  uint32_t snapGen = 0;
  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
    look = g_ui.settings.look;
    contrast = g_ui.settings.contrast;
    mirror = g_ui.settings.mirror;
    orientation = g_ui.settings.orientation;
    snapGen = g_settingsGen;
    xSemaphoreGive(g_stateMutex);
  }
  char path[128];
  std::snprintf(path, sizeof(path),
                "/peer/config?look=%s&contrast=%s&mirror=%u&orientation=%s",
                flipcam::lookName(look), flipcam::contrastName(contrast),
                static_cast<unsigned>(mirror), flipcam::orientationName(orientation));
  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(400);
  http.setTimeout(500);
  if (!http.begin(client, peerUrl(path))) return false;
  http.addHeader("X-FlipCam-Pair", FLIPCAM_PAIR_TOKEN);
  const int code = http.POST("");
  http.end();
  if (code == 200) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
      g_peerSyncedGen = snapGen;
      g_peerConfigDirty = (g_peerSyncedGen != g_settingsGen);
      if (!g_peerConfigDirty) {
        std::snprintf(g_peerOrientation, sizeof(g_peerOrientation), "%s",
                      flipcam::orientationName(orientation));
      }
      xSemaphoreGive(g_stateMutex);
    }
    return true;
  }
  return false;
}

bool readExactTimeout(WiFiClient& client, uint8_t* dst, size_t len, uint32_t timeoutMs) {
  size_t got = 0;
  const uint32_t t0 = millis();
  while (got < len && (millis() - t0) < timeoutMs) {
    const int avail = client.available();
    if (avail > 0) {
      const int r = client.read(dst + got, std::min<size_t>(avail, len - got));
      if (r > 0) got += static_cast<size_t>(r);
    } else if (!client.connected()) {
      break;
    } else {
      delay(1);
    }
  }
  return got == len;
}

bool receiveStreamFrame(WiFiClient& streamClient, std::vector<uint8_t>& rxBuf,
                        flipcam::FmbFrameHeader& outHdr) {
  if (!readExactTimeout(streamClient, reinterpret_cast<uint8_t*>(&outHdr), sizeof(outHdr), 250)) {
    return false;
  }
  if (!flipcam::validateFmbHeader(outHdr)) {
    return false;
  }
  rxBuf.resize(outHdr.payloadLen);
  if (!readExactTimeout(streamClient, rxBuf.data(), outHdr.payloadLen, 350)) {
    return false;
  }
  if (!g_liveMonoScratch) return false;
  return flipcam::decodeFmbPayload(outHdr, rxBuf.data(), outHdr.payloadLen,
                                   g_liveMonoScratch, flipcam::kFmbRawPlaneBytes);
}

void executePeerCapture(WiFiClient& client, uint32_t reqId, bool synthetic, uint8_t orientation) {
  char path[112];
  std::snprintf(path, sizeof(path), "/peer/capture?nonce=%u&id=%u&synth=%d&orient=%s",
                static_cast<unsigned>(g_bootNonce), static_cast<unsigned>(reqId),
                synthetic ? 1 : 0, flipcam::orientationName(orientation));

  const uint32_t capT0Ms = millis();
  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(600);
  http.setTimeout(2500);
  if (!http.begin(client, peerUrl(path))) {
    g_lastCapHttpMs = millis() - capT0Ms;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      g_ui.captureBusy = false;
      g_ui.showToast("CAPTURE HTTP ERR", millis(), 1800);
      xSemaphoreGive(g_stateMutex);
    }
    return;
  }
  http.addHeader("X-FlipCam-Pair", FLIPCAM_PAIR_TOKEN);
  const char* hdrKeys[] = {"X-Width", "X-Height", "X-Capture-Crc32", "X-Synthetic"};
  http.collectHeaders(hdrKeys, 4);
  const int code = http.POST("");

  bool saved = false;
  flipcam::MediaEntry entry{};
  int snapStillCount = 0;
  bool snapStorageFull = false;
  uint64_t snapFreeBytes = 0;
  char faultCopy[40] = "CAPTURE FAILED";

  if (code == 200) {
    const int len = http.getSize();
    if (len > static_cast<int>(flipcam::kMinJpegBytes) &&
        len <= static_cast<int>(flipcam::kMaxStillJpegBytes)) {
      std::vector<uint8_t> jpg(static_cast<size_t>(len));
      WiFiClient* stream = http.getStreamPtr();
      size_t got = 0;
      const uint32_t t0 = millis();
      while (got < jpg.size() && (millis() - t0) < 2200) {
        const int avail = stream->available();
        if (avail > 0) {
          const int r = stream->read(jpg.data() + got, std::min<size_t>(avail, jpg.size() - got));
          if (r > 0) got += static_cast<size_t>(r);
        } else if (!stream->connected()) {
          break;
        } else {
          delay(2);
        }
      }
      if (got == jpg.size()) {
        const uint16_t w = static_cast<uint16_t>(http.header("X-Width").toInt());
        const uint16_t h = static_cast<uint16_t>(http.header("X-Height").toInt());
        uint8_t look = 0, contrast = 1, mirror = 0;
        bool sound = false;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
          look = g_ui.settings.look;
          contrast = g_ui.settings.contrast;
          mirror = g_ui.settings.mirror;
          sound = g_ui.settings.sound != 0;
          xSemaphoreGive(g_stateMutex);
        }
        if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          saved = g_store.saveStill(jpg.data(), jpg.size(), w ? w : 800, h ? h : 600,
                                    look, contrast, mirror, synthetic, entry);
          snapStillCount = g_store.stillCount();
          snapStorageFull = g_store.isFullForMinCapture();
          snapFreeBytes = g_store.status().freeBytes;
          if (!saved && g_store.status().fault[0]) {
            std::snprintf(faultCopy, sizeof(faultCopy), "%s", g_store.status().fault);
          }
          xSemaphoreGive(g_fsMutex);
        }
        if (saved) {
          rlcd_hw::audioPlayShutterClick(sound);
        }
      }
    }
  }
  http.end();
  g_lastCapHttpMs = millis() - capT0Ms;

  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    g_ui.captureBusy = false;
    if (saved) {
      g_ui.setStillCount(snapStillCount);
      g_ui.storageFull = snapStorageFull;
      g_cachedFreeBytes = snapFreeBytes;
      char toast[40];
      std::snprintf(toast, sizeof(toast), "SAVED %ux%u %uK",
                    entry.width, entry.height,
                    static_cast<unsigned>((entry.sizeBytes + 1023u) / 1024u));
      g_ui.showToast(toast, millis(), 1800);
    } else {
      g_ui.showToast(faultCopy, millis(), 2000);
      setLastError(faultCopy);
    }
    xSemaphoreGive(g_stateMutex);
  }
}

void netWorkerTask(void*) {
  WiFiClient httpClient;
  WiFiClient streamClient;
  std::vector<uint8_t> rxBuf;
  rxBuf.reserve(flipcam::kFmbMaxPayloadBytes);
  uint32_t lastInfoMs = 0;
  uint32_t lastConfigAttemptMs = 0;
  uint32_t lastConnectAttemptMs = 0;
  uint32_t rxWinStartMs = millis();
  uint32_t rxWinCount = 0;
  bool wasConnected = false;

  for (;;) {
    const int stations = WiFi.softAPgetStationNum();
    const uint32_t now = millis();

    if (stations <= 0) {
      if (streamClient.connected()) streamClient.stop();
      if (wasConnected) {
        wasConnected = false;
        g_peerReconnects++;
      }
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        g_ui.linkState = flipcam::LinkState::DISCONNECTED;
        g_hasLiveMono = false;
        g_rxFpsX10 = 0;
        g_presentFpsX10 = 0;
        g_peerSyncedGen = 0;
        g_peerConfigDirty = true;
        std::snprintf(g_peerOrientation, sizeof(g_peerOrientation), "unknown");
        if (g_ui.captureBusy || g_pendingCaptureReqId != 0 || g_ui.recording || g_recStartPending) {
          g_ui.captureBusy = false;
          g_pendingCaptureReqId = 0;
          g_ui.recording = false;
          g_recStartPending = false;
          g_recStopPending = false;
          g_ui.showToast("LINK LOST", now, 1800);
        }
        xSemaphoreGive(g_stateMutex);
      }
      delay(120);
      continue;
    }

    // Use healthy FMB1 stream frames as link heartbeat; only poll /peer/info on initial connect
    // or when the binary stream has stalled/disconnected (>=3.5s) to prevent 3s HTTP stalls.
    const bool streamHealthy = streamClient.connected() && (g_lastPreviewMs > 0) && ((now - g_lastPreviewMs) < 3500u);
    if ((!wasConnected || !streamHealthy) && (lastInfoMs == 0 || (now - lastInfoMs >= 3000))) {
      lastInfoMs = now;
      const uint32_t httpT0 = millis();
      const bool ok = fetchPeerInfo(httpClient);
      const uint32_t httpDt = millis() - httpT0;
      g_lastHttpInfoMs = httpDt;
      if (httpDt > g_maxHttpInfoMs) g_maxHttpInfoMs = httpDt;
      if (ok) {
        if (!wasConnected) {
          wasConnected = true;
          g_peerConfigDirty = true;
        }
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
          g_ui.linkState = flipcam::LinkState::CONNECTED;
          g_ui.cameraFault = !g_peerCamOk;
          g_ui.micFault = false;
          xSemaphoreGive(g_stateMutex);
        }
      }
    }

    if (g_peerConfigDirty && (now - lastConfigAttemptMs >= 400)) {
      lastConfigAttemptMs = now;
      pushPeerConfig(httpClient);
    }

    // Photos-only build: prioritize still capture requests above preview stream
    uint32_t capReq = 0;
    bool capSynth = false;
    uint8_t capOrient = 0;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      capReq = g_pendingCaptureReqId;
      capSynth = g_pendingCaptureSynth;
      capOrient = g_pendingCaptureOrientation;
      g_pendingCaptureReqId = 0;
      xSemaphoreGive(g_stateMutex);
    }
    if (capReq > 0) {
      if (g_peerConfigDirty) {
        lastConfigAttemptMs = millis();
        pushPeerConfig(httpClient);
      }
      executePeerCapture(httpClient, capReq, capSynth, capOrient);
      continue;
    }

    // Fast persistent binary 1-bit preview stream ('FMB1' over TCP port 8081)
    bool inLive = false;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      inLive = (g_ui.mode == flipcam::AppMode::LIVE) && !g_ui.playing;
      xSemaphoreGive(g_stateMutex);
    }
    if (!inLive) {
      if (streamClient.connected()) streamClient.stop();
      g_rxFpsX10 = 0;
      delay(40);
      continue;
    }

    if (!streamClient.connected()) {
      g_rxFpsX10 = 0;
      if (now - lastConnectAttemptMs >= 400) {
        lastConnectAttemptMs = now;
        const IPAddress peerIp(FLIPCAM_XIAO_IP_A, FLIPCAM_XIAO_IP_B, FLIPCAM_XIAO_IP_C, FLIPCAM_XIAO_IP_D);
        if (streamClient.connect(peerIp, FLIPCAM_STREAM_PORT, 500)) {
          streamClient.setNoDelay(true);
          streamClient.printf("AUTH %s\n", FLIPCAM_PAIR_TOKEN);
          g_streamConnects++;
          g_peerConfigDirty = true;
          rxWinStartMs = millis();
          rxWinCount = 0;
        }
      }
      delay(15);
      continue;
    }

    if (streamClient.available() <= 0) {
      if (g_lastPreviewMs > 0 && (now - g_lastPreviewMs) > 3000) {
        g_rxFpsX10 = 0;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
          g_ui.linkState = flipcam::LinkState::STALE;
          xSemaphoreGive(g_stateMutex);
        }
      }
      delay(2);
      continue;
    }

    flipcam::FmbFrameHeader hdr{};
    if (receiveStreamFrame(streamClient, rxBuf, hdr)) {
      const uint32_t rxNow = millis();
      rxWinCount++;
      if (rxNow - rxWinStartMs >= 1000) {
        g_rxFpsX10 = (rxWinCount * 10000u) / std::max<uint32_t>(1u, rxNow - rxWinStartMs);
        rxWinStartMs = rxNow;
        rxWinCount = 0;
      }
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        std::swap(g_liveMonoPlane, g_liveMonoScratch);
        g_hasLiveMono = true;
        g_rxFramesOk++;
        g_previewFramesOk = g_rxFramesOk;
        g_rxSeq = hdr.seq;
        g_lastFrameAgeMs = hdr.ageMs;
        g_rxBytesTotal += sizeof(hdr) + hdr.payloadLen;
        g_lastPreviewMs = rxNow;
        g_ui.linkState = flipcam::LinkState::CONNECTED;
        g_forceRender = true;
        xSemaphoreGive(g_stateMutex);
      } else {
        g_rxLockMisses++;
      }
    } else {
      g_rxFramesErr++;
      g_previewFramesErr = g_rxFramesErr;
      streamClient.stop();
      g_rxFpsX10 = 0;
      delay(10);
    }
  }
}

// ---------------------------------------------------------------------------
// Local Web Server (Port 80, Offline Gallery/Settings/Export)
// ---------------------------------------------------------------------------
void setupWebServer() {
  g_web.on("/", HTTP_GET, []() {
    g_web.send_P(200, "text/html", rlcd_hw::kWebGalleryHtml);
  });

  g_web.on("/api/status", HTTP_GET, []() {
    JsonDocument doc;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      doc["photos_only"] = true;
      doc["link"] = flipcam::linkStateName(g_ui.linkState);
      doc["mode"] = flipcam::modeName(g_ui.mode);
      doc["still_count"] = g_ui.stillCount;
      doc["codec_ok"] = rlcd_hw::audioStatus().codecOk;
      doc["peer_cam_ok"] = g_peerCamOk;
      doc["peer_orientation"] = g_peerOrientation;
      doc["peer_config_dirty"] = static_cast<bool>(g_peerConfigDirty);
      auto st = doc["settings"].to<JsonObject>();
      st["look"] = flipcam::lookName(g_ui.settings.look);
      st["contrast"] = flipcam::contrastName(g_ui.settings.contrast);
      st["mirror"] = g_ui.settings.mirror;
      st["sound"] = g_ui.settings.sound;
      st["orientation"] = flipcam::orientationName(g_ui.settings.orientation);
      xSemaphoreGive(g_stateMutex);
    }
    if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      g_store.refreshSpace();
      doc["fs_mounted"] = g_store.status().mounted;
      doc["fs_total"] = g_store.status().totalBytes;
      doc["fs_used"] = g_store.status().usedBytes;
      doc["fs_free"] = g_store.status().freeBytes;
      doc["still_count"] = g_store.stillCount();
      xSemaphoreGive(g_fsMutex);
    }
    String out;
    serializeJson(doc, out);
    g_web.send(200, "application/json", out);
  });

  g_web.on("/api/list", HTTP_GET, []() {
    JsonDocument doc;
    auto arr = doc["items"].to<JsonArray>();
    if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      for (const auto& e : g_store.entries()) {
        if (e.type != static_cast<uint8_t>(flipcam::MediaType::STILL)) continue;
        auto o = arr.add<JsonObject>();
        o["id"] = e.id;
        o["type"] = "still";
        o["title"] = e.title;
        o["width"] = e.width;
        o["height"] = e.height;
        o["bytes"] = e.sizeBytes;
        o["crc32"] = e.crc32;
        o["synthetic"] = e.synthetic != 0;
      }
      xSemaphoreGive(g_fsMutex);
    }
    String out;
    serializeJson(doc, out);
    g_web.send(200, "application/json", out);
  });

  g_web.on("/api/capture", HTTP_POST, []() {
    queueCapture(false);
    g_web.send(200, "application/json", "{\"ok\":true}");
  });

  g_web.on("/api/rec_start", HTTP_POST, []() {
    g_web.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });

  g_web.on("/api/rec_stop", HTTP_POST, []() {
    g_web.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });

  g_web.on("/api/play", HTTP_POST, []() {
    g_web.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });

  g_web.on("/api/delete", HTTP_POST, []() {
    uint32_t id = 0;
    if (!flipcam::parsePositiveId(g_web.arg("id").c_str(), id)) {
      g_web.send(400, "application/json", "{\"error\":\"bad id\"}");
      return;
    }
    bool ok = false;
    int snapStillCount = 0;
    bool snapStorageFull = false;
    if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
      ok = g_store.deleteById(id);
      snapStillCount = g_store.stillCount();
      snapStorageFull = g_store.isFullForMinCapture();
      xSemaphoreGive(g_fsMutex);
    }
    if (ok && xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      g_ui.setStillCount(snapStillCount);
      g_ui.storageFull = snapStorageFull;
      g_loadedGalleryStillId = 0;
      xSemaphoreGive(g_stateMutex);
    }
    g_web.send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"not found\"}");
  });

  g_web.on("/api/settings", HTTP_POST, []() {
    bool ok = true;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      const flipcam::FlipCamSettings prev = g_ui.settings;
      if (g_web.hasArg("look")) ok &= g_ui.updateSettingsField("look", g_web.arg("look").c_str());
      if (g_web.hasArg("contrast")) ok &= g_ui.updateSettingsField("contrast", g_web.arg("contrast").c_str());
      if (g_web.hasArg("mirror")) ok &= g_ui.updateSettingsField("mirror", g_web.arg("mirror").c_str());
      if (g_web.hasArg("sound")) ok &= g_ui.updateSettingsField("sound", g_web.arg("sound").c_str());
      if (g_web.hasArg("orientation")) ok &= g_ui.updateSettingsField("orientation", g_web.arg("orientation").c_str());
      if (ok && g_ui.settings != prev) {
        g_settingsGen++;
        g_peerConfigDirty = true;
        g_forceRender = true;
      } else if (ok) {
        g_forceRender = true;
      }
      xSemaphoreGive(g_stateMutex);
    }
    g_web.send(ok ? 200 : 400, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"bad param\"}");
  });

  g_web.on("/api/media", HTTP_GET, []() {
    uint32_t id = 0;
    if (!flipcam::parsePositiveId(g_web.arg("id").c_str(), id)) {
      g_web.send(400, "text/plain", "bad id");
      return;
    }
    String fmt = g_web.arg("fmt");
    if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
      g_web.send(503, "text/plain", "busy");
      return;
    }
    const flipcam::MediaEntry* e = g_store.findById(id);
    char path[64];
    if (!e || !flipcam::buildSafeMediaPath(flipcam::MediaStore::kBaseDir, e->filename, path, sizeof(path))) {
      xSemaphoreGive(g_fsMutex);
      g_web.send(404, "text/plain", "not found");
      return;
    }
    File f = LittleFS.open(flipcam::stripBasePrefix(path), FILE_READ);
    if (!f) {
      xSemaphoreGive(g_fsMutex);
      g_web.send(404, "text/plain", "open fail");
      return;
    }

    if (e->type != static_cast<uint8_t>(flipcam::MediaType::STILL) || fmt == "wav") {
      f.close();
      xSemaphoreGive(g_fsMutex);
      g_web.send(410, "text/plain", "UNSUPPORTED_PHOTOS_ONLY");
      return;
    }
    g_web.streamFile(f, "image/jpeg");
    f.close();
    xSemaphoreGive(g_fsMutex);
  });

  g_web.on("/api/clip_meta", HTTP_GET, []() {
    g_web.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });

  g_web.on("/api/clip_frame", HTTP_GET, []() {
    g_web.send(410, "application/json", "{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
  });

  g_web.begin();
}

// ---------------------------------------------------------------------------
// USB Serial Command Handler (HWCDC)
// ---------------------------------------------------------------------------
void sendUsbStatus(bool extended) {
  JsonDocument doc;
  doc["board"] = "rlcd";
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
    doc["recovered_tmp"] = ss.recoveredTempDeleted;
    doc["rejected_corrupt"] = ss.rejectedCorruptCount;
    doc["prod_captures"] = g_store.prodCaptures();
    doc["synth_captures"] = g_store.synthCaptures();

    auto med = doc["media"].to<JsonObject>();
    med["count"] = static_cast<uint32_t>(g_store.stillCount());
    med["still_count"] = g_store.stillCount();
    const flipcam::MediaEntry* latest = g_store.getStillNewestFirst(0);
    med["latest_id"] = latest ? latest->id : 0;
    med["latest_crc32"] = latest ? latest->crc32 : 0;
    med["latest_bytes"] = latest ? latest->sizeBytes : 0;
    med["latest_w"] = latest ? latest->width : 0;
    med["latest_h"] = latest ? latest->height : 0;
    xSemaphoreGive(g_fsMutex);
  }

  if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    doc["photos_only"] = true;
    doc["link"] = flipcam::linkStateName(g_ui.linkState);
    doc["ap_stations"] = WiFi.softAPgetStationNum();
    doc["reconnects"] = g_peerReconnects;
    doc["mode"] = flipcam::modeName(g_ui.mode);
    doc["orientation"] = flipcam::orientationName(g_ui.settings.orientation);
    doc["peer_orientation"] = g_peerOrientation;
    doc["peer_config_dirty"] = static_cast<bool>(g_peerConfigDirty);
    doc["peer_orientation_stale"] =
        (g_ui.linkState == flipcam::LinkState::CONNECTED) &&
        (g_peerConfigDirty ||
         std::strcmp(g_peerOrientation, flipcam::orientationName(g_ui.settings.orientation)) != 0);
    doc["test_mode"] = g_buttons.isTestMode();

    auto btns = doc["buttons"].to<JsonObject>();
    btns["gpio18_snap_raw"] = digitalRead(kPinKeySnap);
    btns["gpio0_browse_raw"] = digitalRead(kPinBootBrowse);
    btns["snap_phys_press"] = g_buttons.snap().physPressCount;
    btns["snap_phys_hold"] = g_buttons.snap().physHoldCount;
    btns["snap_inj_press"] = g_buttons.snap().injPressCount;
    btns["snap_inj_hold"] = g_buttons.snap().injHoldCount;
    btns["snap_hold_progress"] = g_buttons.snap().holdProgress;
    btns["browse_phys_press"] = g_buttons.browse().physPressCount;
    btns["browse_phys_tap"] = g_buttons.browse().physTapCount;
    btns["browse_inj_press"] = g_buttons.browse().injPressCount;
    btns["browse_inj_tap"] = g_buttons.browse().injTapCount;
    btns["boot_suppressed"] = g_buttons.snap().bootSuppressed || g_buttons.browse().bootSuppressed;

    auto cam = doc["camera"].to<JsonObject>();
    cam["peer_cam_ok"] = g_peerCamOk;
    cam["sensor"] = g_peerSensor;
    cam["pid"] = g_peerPid;
    cam["preview_res"] = g_peerPreviewRes;
    cam["still_res"] = g_peerStillRes;
    cam["frames_ok"] = g_rxFramesOk;
    cam["frames_err"] = g_rxFramesErr;

    auto strm = doc["stream"].to<JsonObject>();
    strm["port"] = FLIPCAM_STREAM_PORT;
    strm["protocol"] = "FMB1";
    strm["rx_frames"] = g_rxFramesOk;
    strm["rx_err"] = g_rxFramesErr;
    strm["rx_seq"] = g_rxSeq;
    strm["rx_fps_x10"] = g_rxFpsX10;
    strm["rx_bytes"] = g_rxBytesTotal;
    strm["last_age_ms"] = g_lastFrameAgeMs;
    strm["reconnects"] = g_streamConnects;
    strm["rx_lock_miss"] = g_rxLockMisses;
    strm["http_info_last_ms"] = g_lastHttpInfoMs;
    strm["http_info_max_ms"] = g_maxHttpInfoMs;
    strm["cap_http_last_ms"] = g_lastCapHttpMs;

    auto disp = doc["display"].to<JsonObject>();
    disp["ok"] = true;
    disp["rlcd_frames"] = rlcd_hw::panelTransfers();
    disp["rlcd_new_frames"] = g_presentNewFrames;
    disp["present_fps_x10"] = g_presentFpsX10;
    disp["fb_crc32"] = g_lastFbCrc;

    xSemaphoreGive(g_stateMutex);
  }

  const auto ast = rlcd_hw::audioStatus();
  auto aud = doc["audio"].to<JsonObject>();
  aud["codec_ok"] = ast.codecOk;
  aud["es8311_id1"] = ast.chipId1;
  aud["es8311_id2"] = ast.chipId2;
  aud["i2s_tx_bytes"] = ast.i2sTxBytes;
  aud["fault"] = ast.fault;

  const auto bat = rlcd_hw::batteryRead();
  auto b = doc["battery"].to<JsonObject>();
  b["present"] = bat.present;
  b["mv"] = bat.mv;
  b["pct"] = bat.pct;

  if (extended) {
    doc["last_error"] = g_lastError;
    doc["panel_max_us"] = rlcd_hw::panelMaxTransferUs();
  }
  doc["loop_stack_hwm"] = static_cast<uint32_t>(uxTaskGetStackHighWaterMark(nullptr));
  doc["net_stack_hwm"] = g_netTaskHandle ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(g_netTaskHandle)) : 0;

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
    case flipcam::UsbCmdKind::CAPTURE:
      queueCapture(false);
      Serial.println("{\"ok\":true,\"cmd\":\"CAPTURE\"}");
      break;
    case flipcam::UsbCmdKind::CAPTURE_SYNTH:
      if (!g_buttons.isTestMode()) {
        Serial.println("{\"ok\":false,\"error\":\"TESTMODE_REQUIRED\"}");
      } else {
        queueCapture(true);
        Serial.println("{\"ok\":true,\"cmd\":\"CAPTURE SYNTH\"}");
      }
      break;
    case flipcam::UsbCmdKind::RECORD_START:
    case flipcam::UsbCmdKind::RECORD_STOP:
    case flipcam::UsbCmdKind::PLAY:
    case flipcam::UsbCmdKind::STOP:
      Serial.println("{\"ok\":false,\"error\":\"UNSUPPORTED_PHOTOS_ONLY\"}");
      break;
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
    case flipcam::UsbCmdKind::DELETE_ID: {
      bool ok = false;
      int snapStillCount = 0;
      bool snapStorageFull = false;
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        ok = g_store.deleteById(cmd.num1);
        snapStillCount = g_store.stillCount();
        snapStorageFull = g_store.isFullForMinCapture();
        xSemaphoreGive(g_fsMutex);
      }
      if (ok && xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_ui.setStillCount(snapStillCount);
        g_ui.storageFull = snapStorageFull;
        g_loadedGalleryStillId = 0;
        xSemaphoreGive(g_stateMutex);
      }
      Serial.printf("{\"ok\":%s,\"cmd\":\"DELETE\",\"id\":%u}\n",
                    ok ? "true" : "false", static_cast<unsigned>(cmd.num1));
      break;
    }
    case flipcam::UsbCmdKind::TONE: {
      const bool ok = rlcd_hw::audioPlayTone(cmd.num1, cmd.num2, true);
      Serial.printf("{\"ok\":%s,\"cmd\":\"TONE\",\"freq\":%u,\"ms\":%u,\"i2s_tx_bytes\":%llu}\n",
                    ok ? "true" : "false", static_cast<unsigned>(cmd.num1),
                    static_cast<unsigned>(cmd.num2),
                    static_cast<unsigned long long>(rlcd_hw::audioStatus().i2sTxBytes));
      break;
    }
    case flipcam::UsbCmdKind::SET_SETTING: {
      bool ok = false;
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        const flipcam::FlipCamSettings prev = g_ui.settings;
        ok = g_ui.updateSettingsField(cmd.arg1, cmd.arg2);
        if (ok) {
          if (g_ui.settings != prev) {
            g_settingsGen++;
            g_peerConfigDirty = true;
          }
          g_forceRender = true;
        }
        xSemaphoreGive(g_stateMutex);
      }
      Serial.printf("{\"ok\":%s,\"cmd\":\"SET\"}\n", ok ? "true" : "false");
      break;
    }
    case flipcam::UsbCmdKind::GET_SETTINGS:
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.printf("{\"look\":\"%s\",\"contrast\":\"%s\",\"mirror\":%u,\"sound\":%u,\"orientation\":\"%s\"}\n",
                      flipcam::lookName(g_ui.settings.look),
                      flipcam::contrastName(g_ui.settings.contrast),
                      g_ui.settings.mirror, g_ui.settings.sound,
                      flipcam::orientationName(g_ui.settings.orientation));
        xSemaphoreGive(g_stateMutex);
      }
      break;
    case flipcam::UsbCmdKind::SCREENSHOT: {
      std::vector<uint8_t> snap(flipcam::RlcdFrameBuffer::kBytes);
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        std::memcpy(snap.data(), g_fb.bytes, snap.size());
        xSemaphoreGive(g_stateMutex);
      } else {
        Serial.println("{\"ok\":false,\"cmd\":\"SCREENSHOT\",\"error\":\"BUSY\"}");
        break;
      }
      const uint32_t crc = flipcam::crc32Compute(snap.data(), snap.size());
      const std::string b64 = flipcam::encodeBase64(snap.data(), snap.size());
      Serial.printf("SCREENSHOT 400 300 %u %08x\n", static_cast<unsigned>(snap.size()),
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
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_buttons.setTestMode(true, millis());
        xSemaphoreGive(g_stateMutex);
      }
      Serial.println("{\"ok\":true,\"test_mode\":true}");
      break;
    case flipcam::UsbCmdKind::TESTMODE_OFF:
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_buttons.setTestMode(false, millis());
        xSemaphoreGive(g_stateMutex);
      }
      Serial.println("{\"ok\":true,\"test_mode\":false}");
      break;
    case flipcam::UsbCmdKind::INJECT: {
      if (!g_buttons.isTestMode()) {
        Serial.println("{\"ok\":false,\"error\":\"TESTMODE_REQUIRED\"}");
        break;
      }
      flipcam::ButtonId id = flipcam::ButtonId::SNAP;
      if (flipcam::eqUpper(cmd.arg1, "BROWSE")) id = flipcam::ButtonId::BROWSE;
      else if (!flipcam::eqUpper(cmd.arg1, "SNAP")) {
        Serial.println("{\"ok\":false,\"error\":\"BAD_BUTTON\"}");
        break;
      }
      bool ok = false;
      flipcam::ButtonEvents ev{};
      const uint32_t now = millis();
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (flipcam::eqUpper(cmd.arg2, "DOWN")) {
          ok = g_buttons.injectDown(id, now);
        } else if (flipcam::eqUpper(cmd.arg2, "UP")) {
          ok = g_buttons.injectUp(id, now, ev);
        } else if (flipcam::eqUpper(cmd.arg2, "CANCEL")) {
          ok = g_buttons.injectCancel(id);
        } else if (flipcam::eqUpper(cmd.arg2, "TAP")) {
          ok = g_buttons.injectDown(id, now) && g_buttons.injectUp(id, now + 40, ev);
        } else if (flipcam::eqUpper(cmd.arg2, "HOLD")) {
          if (g_buttons.injectDown(id, now)) {
            ev = g_buttons.update(false, false, now + flipcam::ButtonController::kHoldMs + 50);
            flipcam::ButtonEvents upEv{};
            ok = g_buttons.injectUp(id, now + flipcam::ButtonController::kHoldMs + 60, upEv);
            ev.snapTap |= upEv.snapTap;
            ev.snapHold |= upEv.snapHold;
            ev.browseTap |= upEv.browseTap;
            ev.browseHold |= upEv.browseHold;
          }
        }
        const auto prevOrient = g_ui.settings.orientation;
        const auto uiCmd = g_ui.applyButtonEvents(ev, now);
        applyUiCommandLocked(uiCmd);
        if (g_ui.settings.orientation != prevOrient) {
          g_settingsGen++;
          g_peerConfigDirty = true;
        }
        if (ok || ev.snapTap || ev.snapHold || ev.browseTap) {
          g_forceRender = true;
        }
        xSemaphoreGive(g_stateMutex);
      }
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
      bool busy = false;
      if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        busy = g_ui.recording || g_ui.captureBusy || g_ui.playing;
        xSemaphoreGive(g_stateMutex);
      }
      bool ok = false;
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
        ok = g_store.guardedFormat(cmd.arg1, busy);
        xSemaphoreGive(g_fsMutex);
      }
      if (ok && xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_ui.storageMounted = g_store.status().mounted;
        g_ui.storageFull = g_store.isFullForMinCapture();
        g_ui.setStillCount(g_store.stillCount());
        g_loadedGalleryStillId = 0;
        xSemaphoreGive(g_stateMutex);
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

}  // namespace

void setup() {
  Serial.begin(115200);
  g_stateMutex = xSemaphoreCreateMutex();
  g_fsMutex = xSemaphoreCreateMutex();

  pinMode(kPinKeySnap, INPUT_PULLUP);
  pinMode(kPinBootBrowse, INPUT_PULLUP);
  const bool snapAtBoot = (digitalRead(kPinKeySnap) == LOW);
  const bool browseAtBoot = (digitalRead(kPinBootBrowse) == LOW);
  // RLCD enables SNAP ~1s hold (Selfie/Rear180 toggle) and keeps BROWSE hold disabled
  g_buttons.init(snapAtBoot, browseAtBoot, millis(), true, false);

  rlcd_hw::panelBegin();
  rlcd_hw::batteryBegin();
  rlcd_hw::audioBegin();

  g_galleryGray = static_cast<uint8_t*>(heap_caps_malloc(kGrayCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  g_galleryScratchGray = static_cast<uint8_t*>(heap_caps_malloc(kGrayCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  g_liveMonoPlane = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!g_liveMonoPlane) {
    g_liveMonoPlane = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_8BIT));
  }
  g_liveMonoScratch = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!g_liveMonoScratch) {
    g_liveMonoScratch = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_8BIT));
  }
  g_renderMonoSnap = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!g_renderMonoSnap) {
    g_renderMonoSnap = static_cast<uint8_t*>(heap_caps_malloc(flipcam::kFmbRawPlaneBytes, MALLOC_CAP_8BIT));
  }

  if (g_store.beginNoAutoFormat()) {
    g_ui.storageMounted = true;
    g_ui.storageFull = g_store.isFullForMinCapture();
    g_ui.setStillCount(g_store.stillCount());
    g_cachedFreeBytes = g_store.status().freeBytes;
    flipcam::FlipCamSettings loaded{};
    if (g_store.loadSettings(loaded)) {
      g_ui.settings = loaded;
    }
  } else {
    g_ui.storageMounted = false;
  }

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  const IPAddress localIp(FLIPCAM_RLCD_IP_A, FLIPCAM_RLCD_IP_B, FLIPCAM_RLCD_IP_C, FLIPCAM_RLCD_IP_D);
  const IPAddress mask(255, 255, 255, 0);
  WiFi.softAPConfig(localIp, localIp, mask);
  WiFi.softAP(FLIPCAM_WIFI_SSID, FLIPCAM_WIFI_PSK, FLIPCAM_WIFI_CHANNEL, 0, 2);
  g_presentWinStartMs = millis();

  g_bootNonce = esp_random();
  setupWebServer();

  xTaskCreatePinnedToCore(netWorkerTask, "rlcd_net", 12288, nullptr, 1, &g_netTaskHandle, 0);
}

void loop() {
  static uint32_t lastBtnMs = 0;
  static uint32_t lastRenderMs = 0;
  static uint32_t lastSettingsMs = 0;

  pollUsbSerial();
  g_web.handleClient();

  if (g_rebootRequested) {
    if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
      g_store.saveSettingsIfChanged(g_ui.settings);
      LittleFS.end();
      xSemaphoreGive(g_fsMutex);
    }
    delay(100);
    ESP.restart();
  }

  const uint32_t now = millis();
  if (now - lastBtnMs >= 5) {
    lastBtnMs = now;
    const bool snapRaw = (digitalRead(kPinKeySnap) == LOW);
    const bool browseRaw = (digitalRead(kPinBootBrowse) == LOW);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      const bool prevSnapAct = g_buttons.snap().active;
      const bool prevBrowseAct = g_buttons.browse().active;
      const int prevProgressStep = static_cast<int>(g_buttons.snap().holdProgress * 20.0f);
      const auto prevMode = g_ui.mode;
      const int prevIdx = g_ui.stillIndex;
      const uint8_t prevOrient = g_ui.settings.orientation;
      const auto ev = g_buttons.update(snapRaw, browseRaw, now);
      const auto cmd1 = g_ui.applyButtonEvents(ev, now);
      applyUiCommandLocked(cmd1);
      const auto cmd2 = g_ui.tick(now);
      applyUiCommandLocked(cmd2);
      if (g_ui.settings.orientation != prevOrient) {
        g_settingsGen++;
        g_peerConfigDirty = true;
      }
      const int curProgressStep = static_cast<int>(g_buttons.snap().holdProgress * 20.0f);
      if (g_buttons.snap().active != prevSnapAct || g_buttons.browse().active != prevBrowseAct ||
          curProgressStep != prevProgressStep ||
          g_ui.mode != prevMode || g_ui.stillIndex != prevIdx ||
          g_ui.settings.orientation != prevOrient ||
          ev.snapTap || ev.snapHold || ev.browseTap) {
        g_forceRender = true;
      }
      xSemaphoreGive(g_stateMutex);
    }
    refreshGalleryBitmapIfNeeded();
  }

  // Persist settings ONLY when dirty and debounced every 500ms
  if (now - lastSettingsMs >= 500) {
    lastSettingsMs = now;
    flipcam::FlipCamSettings copy{};
    bool dirty = false;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      dirty = g_ui.settingsDirty;
      if (dirty) {
        copy = g_ui.settings;
        g_ui.settingsDirty = false;
      }
      xSemaphoreGive(g_stateMutex);
    }
    if (dirty && xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      g_store.saveSettingsIfChanged(copy);
      xSemaphoreGive(g_fsMutex);
    }
  }

  if (g_forceRender || (now - lastRenderMs >= 120)) {
    lastRenderMs = now;
    g_forceRender = false;
    static uint32_t lastHousekeepMs = 0;
    static rlcd_hw::BatteryReading cachedBat{};
    if (lastHousekeepMs == 0 || (now - lastHousekeepMs >= 500)) {
      lastHousekeepMs = now;
      cachedBat = rlcd_hw::batteryRead();
      if (xSemaphoreTake(g_fsMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        const uint64_t fb = g_store.status().freeBytes;
        xSemaphoreGive(g_fsMutex);
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
          g_cachedFreeBytes = fb;
          xSemaphoreGive(g_stateMutex);
        }
      }
    }

    flipcam::FlipCamUiState uiSnap{};
    flipcam::ButtonController btnSnap{};
    flipcam::MediaEntry stillSnap{};
    bool hasStillSnap = false;
    bool hasMonoSnap = false;
    bool isNewSeq = false;
    uint32_t seqToMark = 0;
    uint64_t freeSnap = 0;
    bool peerCamSnap = false;
    bool peerMicSnap = false;
    const uint8_t* galPtr = nullptr;
    int galW = 0, galH = 0;

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      uiSnap = g_ui;
      btnSnap = g_buttons;
      if (g_hasLiveMono && g_liveMonoPlane && g_renderMonoSnap) {
        std::memcpy(g_renderMonoSnap, g_liveMonoPlane, flipcam::kFmbRawPlaneBytes);
        hasMonoSnap = true;
      }
      isNewSeq = (uiSnap.mode == flipcam::AppMode::LIVE && hasMonoSnap &&
                  g_rxSeq > 0 && g_rxSeq != g_lastPresentedSeq);
      seqToMark = g_rxSeq;
      hasStillSnap = g_hasCachedCurrentStill;
      if (hasStillSnap) stillSnap = g_cachedCurrentStill;
      freeSnap = g_cachedFreeBytes;
      peerCamSnap = g_peerCamOk;
      peerMicSnap = g_peerMicOk;
      galPtr = g_galleryGray;
      galW = g_galleryW;
      galH = g_galleryH;
      xSemaphoreGive(g_stateMutex);

      // Render & CRC32 completely outside g_stateMutex so Core 0 FMB1 receiver never blocks
      flipcam::RlcdRenderContext rctx{};
      rctx.ui = &uiSnap;
      rctx.buttons = &btnSnap;
      rctx.liveMonoPlane = hasMonoSnap ? g_renderMonoSnap : nullptr;
      rctx.galleryGray = galPtr;
      rctx.galleryW = galW;
      rctx.galleryH = galH;
      rctx.currentStill = hasStillSnap ? &stillSnap : nullptr;
      rctx.freeMiB = static_cast<float>(freeSnap) / 1048576.0f;
      rctx.batteryPresent = cachedBat.present;
      rctx.batteryPct = cachedBat.pct;
      rctx.nowMs = now;
      rctx.testMode = btnSnap.isTestMode();
      rctx.peerCamOk = peerCamSnap;
      rctx.peerMicOk = peerMicSnap;
      flipcam::renderRlcdFrame(g_fb, rctx);
      g_lastFbCrc = g_fb.crc32();
      rlcd_hw::panelPresent(g_fb);
      if (isNewSeq) {
        g_lastPresentedSeq = seqToMark;
        g_presentNewFrames++;
        g_presentWinCount++;
        const uint32_t presNow = millis();
        if (presNow - g_presentWinStartMs >= 1000) {
          g_presentFpsX10 = (g_presentWinCount * 10000u) / std::max<uint32_t>(1u, presNow - g_presentWinStartMs);
          g_presentWinStartMs = presNow;
          g_presentWinCount = 0;
        }
      } else if (g_lastPreviewMs > 0 && (now - g_lastPreviewMs) > 1500) {
        g_presentFpsX10 = 0;
      }
    }
  }
  delay(2);
}
