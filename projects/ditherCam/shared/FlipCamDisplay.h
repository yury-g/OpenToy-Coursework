// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "FlipCamCrc.h"
#include "FlipCamDither.h"
#include "FlipCamFont5x7.h"
#include "FlipCamStream.h"
#include "FlipCamUiState.h"

namespace flipcam {

// Exact ST7305 400x300 1-bit framebuffer matching reference/Display.h
struct RlcdFrameBuffer {
  static constexpr int kWidth = 400;
  static constexpr int kHeight = 300;
  static constexpr int kBytes = 15000;
  uint8_t bytes[kBytes]{};

  void clear(bool white = true) {
    std::fill(bytes, bytes + kBytes, white ? 0xFF : 0x00);
  }

  // Matches reference/Display.h: pixel(buffer, x, y, white)
  void setPixel(int x, int y, bool white) {
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return;
    const unsigned ry = static_cast<unsigned>(kHeight - 1 - y);
    const unsigned index = static_cast<unsigned>(x / 2) * 75u + ry / 4u;
    const unsigned bit = 7u - ((ry % 4u) * 2u + static_cast<unsigned>(x % 2));
    if (white) bytes[index] |= static_cast<uint8_t>(1u << bit);
    else bytes[index] &= static_cast<uint8_t>(~(1u << bit));
  }

  bool getPixel(int x, int y) const {
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return false;
    const unsigned ry = static_cast<unsigned>(kHeight - 1 - y);
    const unsigned index = static_cast<unsigned>(x / 2) * 75u + ry / 4u;
    const unsigned bit = 7u - ((ry % 4u) * 2u + static_cast<unsigned>(x % 2));
    return (bytes[index] & static_cast<uint8_t>(1u << bit)) != 0;
  }

  uint32_t crc32() const { return crc32Compute(bytes, kBytes); }
};

// SSD1306 128x64 1-bit framebuffer in native SSD1306 page-vertical layout (1024 bytes)
struct OledFrameBuffer {
  static constexpr int kWidth = 128;
  static constexpr int kHeight = 64;
  static constexpr int kBytes = 1024;
  uint8_t bytes[kBytes]{};

  void clear(bool lit = false) {
    std::fill(bytes, bytes + kBytes, lit ? 0xFF : 0x00);
  }

  void setPixel(int x, int y, bool lit) {
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return;
    const int idx = (y / 8) * kWidth + x;
    const uint8_t mask = static_cast<uint8_t>(1u << (y & 7));
    if (lit) bytes[idx] |= mask;
    else bytes[idx] &= static_cast<uint8_t>(~mask);
  }

  bool getPixel(int x, int y) const {
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return false;
    const int idx = (y / 8) * kWidth + x;
    return (bytes[idx] & static_cast<uint8_t>(1u << (y & 7))) != 0;
  }

  uint32_t crc32() const { return crc32Compute(bytes, kBytes); }
};

struct RlcdRenderContext {
  const FlipCamUiState* ui = nullptr;
  const ButtonController* buttons = nullptr;
  const uint8_t* liveMonoPlane = nullptr;  // Fast pre-dithered 4:3 (304x228) 1-bit stream (1 = white, 0 = ink)
  const uint8_t* previewGray = nullptr;    // Optional grayscale fallback
  int previewW = 0;
  int previewH = 0;
  const uint8_t* galleryGray = nullptr;
  int galleryW = 0;
  int galleryH = 0;
  const MediaEntry* currentStill = nullptr;
  float freeMiB = 0.0f;
  bool batteryPresent = false;
  int batteryPct = -1;
  uint32_t nowMs = 0;
  bool testMode = false;
  bool peerCamOk = false;
  bool peerMicOk = false;
};

struct OledRenderContext {
  const uint8_t* previewGray = nullptr;
  int previewW = 0;
  int previewH = 0;
  bool linkConnected = false;
  bool recording = false;
  uint32_t recElapsedMs = 0;
  bool cameraOk = false;
  bool micOk = false;
  bool d1Pressed = false;
  float freeMiB = 0.0f;
  uint8_t look = 0;
  uint8_t contrast = 1;
  bool mirror = false;
  uint8_t orientation = 0;  // 0=selfie, 1=rear (mode indication only; OLED preview/text orientation unchanged)
  uint32_t blinkUntilMs = 0;
  uint32_t nowMs = 0;
};

inline void blitLiveMono400x300(RlcdFrameBuffer& fb, const uint8_t* liveMonoPlane, bool rotate180) {
  constexpr int srcW = static_cast<int>(kFmbViewW);
  constexpr int srcH = static_cast<int>(kFmbViewH);
  constexpr int rowBytes = srcW / 8;

  uint8_t srcOff[RlcdFrameBuffer::kWidth];
  uint8_t srcMask[RlcdFrameBuffer::kWidth];
  for (int dx = 0; dx < RlcdFrameBuffer::kWidth; ++dx) {
    const int ox = rotate180 ? (RlcdFrameBuffer::kWidth - 1 - dx) : dx;
    const int sx = (ox * srcW) / RlcdFrameBuffer::kWidth;
    srcOff[dx] = static_cast<uint8_t>(sx >> 3);
    srcMask[dx] = static_cast<uint8_t>(0x80u >> (sx & 7));
  }

  const uint8_t* rowPtr[RlcdFrameBuffer::kHeight];
  for (int dy = 0; dy < RlcdFrameBuffer::kHeight; ++dy) {
    const int oy = rotate180 ? (RlcdFrameBuffer::kHeight - 1 - dy) : dy;
    const int sy = (oy * srcH) / RlcdFrameBuffer::kHeight;
    rowPtr[dy] = liveMonoPlane + sy * rowBytes;
  }

  for (int bx = 0; bx < RlcdFrameBuffer::kWidth / 2; ++bx) {
    const int x0 = bx * 2;
    const int x1 = x0 + 1;
    const uint8_t off0 = srcOff[x0], m0 = srcMask[x0];
    const uint8_t off1 = srcOff[x1], m1 = srcMask[x1];
    uint8_t* dst = fb.bytes + bx * 75;
    for (int q = 0; q < 75; ++q) {
      const int yBase = RlcdFrameBuffer::kHeight - 1 - (q * 4);
      const uint8_t* r0 = rowPtr[yBase];
      const uint8_t* r1 = rowPtr[yBase - 1];
      const uint8_t* r2 = rowPtr[yBase - 2];
      const uint8_t* r3 = rowPtr[yBase - 3];
      uint8_t b = 0;
      if (r0[off0] & m0) b |= 0x80u;
      if (r0[off1] & m1) b |= 0x40u;
      if (r1[off0] & m0) b |= 0x20u;
      if (r1[off1] & m1) b |= 0x10u;
      if (r2[off0] & m0) b |= 0x08u;
      if (r2[off1] & m1) b |= 0x04u;
      if (r3[off0] & m0) b |= 0x02u;
      if (r3[off1] & m1) b |= 0x01u;
      dst[q] = b;
    }
  }
}

inline void renderRlcdFrame(RlcdFrameBuffer& fb, const RlcdRenderContext& ctx) {
  // ST7305: white=true is light pixel, white=false is BLACK background (matching OLED polarity).
  // Image-first, UI-over order across full 400x300 screen (no outer rect, reticle, or reserved bars).
  auto setLit = [&fb](int x, int y, bool lit) { fb.setPixel(x, y, lit); };

  if (!ctx.ui || !ctx.buttons) {
    fb.clear(false);
    return;
  }
  const FlipCamUiState& ui = *ctx.ui;
  const UiContextHints hints = ui.contextHints();
  const ButtonChannel& snap = ctx.buttons->snap();
  const ButtonChannel& browse = ctx.buttons->browse();
  const bool isRear = (ui.settings.orientation == static_cast<uint8_t>(OrientationMode::REAR));

  bool hasImage = false;
  if (ui.mode == AppMode::LIVE) {
    if (ctx.liveMonoPlane) {
      blitLiveMono400x300(fb, ctx.liveMonoPlane, isRear);
      hasImage = true;
    } else if (ctx.previewGray && ctx.previewW > 0 && ctx.previewH > 0) {
      const uint8_t* lut = getContrastLut(ui.settings.contrast);
      const int rotW = ctx.previewH;
      const int rotH = ctx.previewW;
      int cropW = rotW;
      int cropH = (rotW * 3) / 4;
      if (cropH > rotH) { cropH = rotH; cropW = (rotH * 4) / 3; }
      const int offX = (rotW - cropW) / 2;
      const int offY = (rotH - cropH) / 2;
      for (int y = 0; y < fb.kHeight; ++y) {
        const int oy = isRear ? (fb.kHeight - 1 - y) : y;
        const int sx = offY + (oy * cropH) / fb.kHeight;
        for (int x = 0; x < fb.kWidth; ++x) {
          const int ox = isRear ? (fb.kWidth - 1 - x) : x;
          const int mx = (ui.settings.mirror != 0) ? (fb.kWidth - 1 - ox) : ox;
          const int rx = offX + (mx * cropW) / fb.kWidth;
          const int sy = ctx.previewH - 1 - rx;
          const bool light = quantizeLuma1BitFast(lut[ctx.previewGray[sy * ctx.previewW + sx]], x, y, ui.settings.look);
          fb.setPixel(x, y, light);
        }
      }
      hasImage = true;
    } else {
      fb.clear(false);
    }

    if (hasImage && ctx.nowMs < ui.blinkUntilMs) {
      for (int i = 0; i < fb.kBytes; ++i) {
        fb.bytes[i] ^= 0xFFu;
      }
    }

    if (!hasImage) {
      if (ui.linkState == LinkState::CONNECTED) {
        drawText5x7(fb.kWidth, fb.kHeight, setLit, "WAITING FOR CAMERA FRAME...",
                    200, 146, 1, true, TextAlign::CENTER);
      } else {
        const char* msg = (ui.linkState == LinkState::CONNECTING)
                              ? "RECONNECTING TO CAMERA..."
                              : "PEER CAMERA OFFLINE";
        drawText5x7(fb.kWidth, fb.kHeight, setLit, msg, 200, 138, 1, true, TextAlign::CENTER);
        drawText5x7(fb.kWidth, fb.kHeight, setLit, "TAP BROWSE TO VIEW PHOTOS",
                    200, 156, 1, true, TextAlign::CENTER);
      }
    } else if (ui.linkState != LinkState::CONNECTED) {
      fillRect1Bit(fb.kWidth, fb.kHeight, setLit, 72, 128, 256, 44, false);
      strokeRect1Bit(fb.kWidth, fb.kHeight, setLit, 72, 128, 256, 44, true, 1);
      const char* msg = (ui.linkState == LinkState::CONNECTING)
                            ? "RECONNECTING TO CAMERA..."
                            : (ui.linkState == LinkState::STALE)
                                  ? "LINK STALE - WAITING..."
                                  : "LINK LOST - PREVIEW PAUSED";
      drawText5x7(fb.kWidth, fb.kHeight, setLit, msg, 200, 138, 1, true, TextAlign::CENTER);
      drawText5x7(fb.kWidth, fb.kHeight, setLit, "TAP BROWSE TO VIEW PHOTOS",
                  200, 154, 1, true, TextAlign::CENTER);
    }
  } else {
    // GALLERY: fill 400x300 for 4:3 photos, letterbox/pillarbox non-4:3 older photos without distortion.
    // Never rerotate already-saved photos.
    fb.clear(false);
    if (!ctx.currentStill || ui.stillCount <= 0) {
      drawText5x7(fb.kWidth, fb.kHeight, setLit, "NO STILL PHOTOS", 200, 138, 2, true, TextAlign::CENTER);
      drawText5x7(fb.kWidth, fb.kHeight, setLit, "TAP SNAP TO RETURN LIVE", 200, 164, 1, true, TextAlign::CENTER);
    } else if (ctx.galleryGray && ctx.galleryW > 0 && ctx.galleryH > 0) {
      blitGrayFitAspect1Bit(ctx.galleryGray, ctx.galleryW, ctx.galleryH,
                            0, 0, fb.kWidth, fb.kHeight, setLit,
                            ctx.currentStill->look, ctx.currentStill->contrast,
                            ctx.currentStill->mirror != 0, false);
    } else {
      drawText5x7(fb.kWidth, fb.kHeight, setLit, "PHOTO PREVIEW UNAVAILABLE",
                  200, 146, 1, true, TextAlign::CENTER);
    }
  }

  // UI OVERLAY LAYER (always drawn AFTER image so labels remain legible on light & dark regions)
  char snapLbl[28];
  std::snprintf(snapLbl, sizeof(snapLbl), "SNAP:%s", hints.snapTap);
  drawBadge5x7(fb.kWidth, fb.kHeight, setLit, snapLbl, 4, 4, snap.active,
               snap.holdProgress, TextAlign::LEFT, 4, 4, ui.mode == AppMode::LIVE);

  char browseLbl[28];
  std::snprintf(browseLbl, sizeof(browseLbl), "BROWSE:%s", hints.browseTap);
  drawBadge5x7(fb.kWidth, fb.kHeight, setLit, browseLbl, fb.kWidth - 4, 4, browse.active,
               0.0f, TextAlign::RIGHT, 4, 4, false);

  // Top-center compact pill: orientation & compact fault/status (LIVE) or photo index (GALLERY)
  char centerLbl[36];
  if (ui.mode == AppMode::LIVE) {
    const char* orientStr = isRear ? "REAR" : "SELFIE";
    if (ui.cameraFault) std::snprintf(centerLbl, sizeof(centerLbl), "%s CAM ERR", orientStr);
    else if (ui.codecFault) std::snprintf(centerLbl, sizeof(centerLbl), "%s SPK ERR", orientStr);
    else if (ui.storageFull) std::snprintf(centerLbl, sizeof(centerLbl), "%s FULL", orientStr);
    else if (!ui.storageMounted) std::snprintf(centerLbl, sizeof(centerLbl), "%s NO FS", orientStr);
    else if (ctx.testMode) std::snprintf(centerLbl, sizeof(centerLbl), "%s TEST", orientStr);
    else std::snprintf(centerLbl, sizeof(centerLbl), "%s", orientStr);
  } else {
    if (ctx.currentStill && ui.stillCount > 0) {
      std::snprintf(centerLbl, sizeof(centerLbl), "%d/%d%s",
                    ui.stillIndex + 1, ui.stillCount,
                    (ui.stillIndex + 1 >= ui.stillCount) ? " OLDEST" : "");
    } else {
      std::snprintf(centerLbl, sizeof(centerLbl), "GALLERY");
    }
  }
  drawPill5x7(fb.kWidth, fb.kHeight, setLit, centerLbl, 200, 5, TextAlign::CENTER, 4, 3, true, false);

  // Gallery compact bottom-left metadata pill (only when viewing a photo)
  if (ui.mode == AppMode::GALLERY && ctx.currentStill && ui.stillCount > 0) {
    char galInfo[48];
    std::snprintf(galInfo, sizeof(galInfo), "%s %uK %ux%u",
                  ctx.currentStill->title,
                  static_cast<unsigned>((ctx.currentStill->sizeBytes + 1023u) / 1024u),
                  ctx.currentStill->width, ctx.currentStill->height);
    drawPill5x7(fb.kWidth, fb.kHeight, setLit, galInfo, 4, fb.kHeight - 17,
                TextAlign::LEFT, 4, 3, true, false);
  }

  // High-contrast toast pill above bottom edge
  if (ui.toast[0] != '\0' && ctx.nowMs < ui.toastUntilMs) {
    const int tw = measureText5x7(ui.toast, 1);
    const int pw = std::max(120, std::min(300, tw + 20));
    const int px = (fb.kWidth - pw) / 2;
    fillRect1Bit(fb.kWidth, fb.kHeight, setLit, px, 254, pw, 18, true);
    strokeRect1Bit(fb.kWidth, fb.kHeight, setLit, px, 254, pw, 18, false, 1);
    drawText5x7(fb.kWidth, fb.kHeight, setLit, ui.toast, 200, 260, 1, false, TextAlign::CENTER);
  }

  // Bottom-right battery icon with 2px dark local backing (no full-width bar)
  fillRect1Bit(fb.kWidth, fb.kHeight, setLit, fb.kWidth - 24, fb.kHeight - 16, 21, 13, false);
  drawBatteryIcon(fb.kWidth, fb.kHeight, setLit, fb.kWidth - 22, fb.kHeight - 14,
                  ctx.batteryPresent, ctx.batteryPct, true, false);
}

inline void renderOledFrame(OledFrameBuffer& fb, const OledRenderContext& ctx) {
  auto setLit = [&fb](int x, int y, bool lit) { fb.setPixel(x, y, lit); };
  fb.clear(false);

  // Live thumbnail area y=11..63 (128x53), letterboxed 4:3 matching 90CW RLCD + saved 600x450 photo
  if (ctx.previewGray && ctx.previewW > 0 && ctx.previewH > 0) {
    blitGray90Cw43LetterboxOled(ctx.previewGray, ctx.previewW, ctx.previewH,
                                0, 11, 128, 53, setLit,
                                ctx.look, ctx.contrast, ctx.mirror);
  } else {
    drawText5x7(fb.kWidth, fb.kHeight, setLit,
                ctx.cameraOk ? "WAITING FRAME" : "CAM FAULT",
                64, 34, 1, true, TextAlign::CENTER);
  }

  if (ctx.nowMs < ctx.blinkUntilMs) {
    for (int y = 11; y < 64; ++y) {
      for (int x = 0; x < 128; ++x) {
        fb.setPixel(x, y, !fb.getPixel(x, y));
      }
    }
  }

  // Top 11px status header bar
  fillRect1Bit(fb.kWidth, fb.kHeight, setLit, 0, 0, 128, 11, false);
  fillRect1Bit(fb.kWidth, fb.kHeight, setLit, 0, 10, 128, 1, true);

  if (!ctx.cameraOk) {
    drawText5x7(fb.kWidth, fb.kHeight, setLit, "CAM ERR", 2, 2, 1, true, TextAlign::LEFT);
  } else if (!ctx.linkConnected) {
    drawText5x7(fb.kWidth, fb.kHeight, setLit, "NO LINK", 2, 2, 1, true, TextAlign::LEFT);
  } else if (ctx.orientation == static_cast<uint8_t>(OrientationMode::REAR)) {
    drawText5x7(fb.kWidth, fb.kHeight, setLit, ctx.mirror ? "REAR M" : "REAR", 2, 2, 1, true, TextAlign::LEFT);
  } else {
    drawText5x7(fb.kWidth, fb.kHeight, setLit, ctx.mirror ? "SELFIE" : "LIVE", 2, 2, 1, true, TextAlign::LEFT);
  }

  char memStr[12];
  std::snprintf(memStr, sizeof(memStr), "%.1fM", ctx.freeMiB);
  drawText5x7(fb.kWidth, fb.kHeight, setLit, memStr, 107, 2, 1, true, TextAlign::RIGHT);

  // XIAO has NO battery measurement hardware: honest empty outline with '?'
  drawBatteryIcon(fb.kWidth, fb.kHeight, setLit, 110, 1, false, -1, true, false);

  if (ctx.d1Pressed) {
    fillRect1Bit(fb.kWidth, fb.kHeight, setLit, 0, 60, 128, 4, true);
  }
}

}  // namespace flipcam
