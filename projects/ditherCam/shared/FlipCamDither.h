// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "FlipCamTypes.h"

namespace flipcam {

inline float contrastGain(uint8_t contrast) {
  switch (static_cast<ContrastLevel>(contrast)) {
    case ContrastLevel::C_LOW: return 0.72f;
    case ContrastLevel::C_HIGH: return 2.0f;  // High-contrast default per FAST-MONO.md
    default: return 1.0f;
  }
}

inline const uint8_t* getContrastLut(uint8_t contrast) {
  static uint8_t luts[3][256];
  static bool inited = false;
  if (!inited) {
    const float gains[3] = {0.72f, 1.0f, 2.0f};
    for (int c = 0; c < 3; ++c) {
      for (int v = 0; v < 256; ++v) {
        float adj = (static_cast<float>(v) / 255.0f - 0.5f) * gains[c] + 0.5f;
        if (adj < 0.0f) adj = 0.0f;
        if (adj > 1.0f) adj = 1.0f;
        luts[c][v] = static_cast<uint8_t>(adj * 255.0f + 0.5f);
      }
    }
    inited = true;
  }
  const int idx = (contrast <= 2) ? static_cast<int>(contrast) : 1;
  return luts[idx];
}

inline bool quantizeLuma1BitFast(uint8_t adjLuma, int x, int y, uint8_t look) {
  // 4x4 Bayer thresholds scaled to 0..255: ((b + 0.5) / 16) * 255
  static constexpr uint8_t kBayerThresh[16] = {
      8, 136, 40, 168,
      200, 72, 232, 104,
      56, 184, 24, 152,
      248, 120, 216, 88,
  };
  if (look == static_cast<uint8_t>(LookMode::THRESHOLD)) {
    return adjLuma >= 128u;
  }
  return adjLuma > kBayerThresh[(y & 3) * 4 + (x & 3)];
}

inline bool quantizeLuma1Bit(uint8_t luma, int x, int y, uint8_t look, uint8_t contrast) {
  const uint8_t* lut = getContrastLut(contrast);
  return quantizeLuma1BitFast(lut[luma], x, y, look);
}

// Rotates src (srcW x srcH) by 90 degrees clockwise (rotW=srcH, rotH=srcW),
// center-crops to 4:3 landscape, and samples into dstGray (dstW x dstH).
// When rotate180 is true (rear orientation), adds 180 deg rotation after selfie baseline (not mirror).
// Example: src 800x600 -> 90CW 600x800 -> center 4:3 crop 600x450 (exact 1:1, zero upsample).
inline void rotate90CwCrop43Gray(const uint8_t* src, int srcW, int srcH,
                                 uint8_t* dstGray, int dstW, int dstH,
                                 bool mirror = false, bool rotate180 = false) {
  if (!src || !dstGray || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
  const int rotW = srcH;
  const int rotH = srcW;

  // Center 4:3 crop within (rotW x rotH)
  int cropW = rotW;
  int cropH = (rotW * 3) / 4;
  if (cropH > rotH) {
    cropH = rotH;
    cropW = (rotH * 4) / 3;
  }
  const int offX = (rotW - cropW) / 2;
  const int offY = (rotH - cropH) / 2;

  for (int dy = 0; dy < dstH; ++dy) {
    const int oy = rotate180 ? (dstH - 1 - dy) : dy;
    const int ry = offY + (oy * cropH) / dstH;
    // 90CW inverse map: sx = ry, sy = srcH - 1 - rx
    const int sx = ry;
    uint8_t* outRow = dstGray + dy * dstW;
    for (int dx = 0; dx < dstW; ++dx) {
      const int ox = rotate180 ? (dstW - 1 - dx) : dx;
      const int mx = mirror ? (dstW - 1 - ox) : ox;
      const int rx = offX + (mx * cropW) / dstW;
      const int sy = srcH - 1 - rx;
      outRow[dx] = src[sy * srcW + sx];
    }
  }
}

// Fast integer-LUT fused 90CW + center 4:3 crop + contrast LUT + 4x4 Bayer dither
// directly into a packed MSB-first 1-bit plane (dstW divisible by 8; 1 = light/white, 0 = dark/ink).
inline void dither90CwCrop43ToMonoPlane(const uint8_t* src, int srcW, int srcH,
                                        uint8_t* dstPlane, int dstW, int dstH,
                                        uint8_t look, uint8_t contrast, bool mirror) {
  if (!src || !dstPlane || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
  const int rotW = srcH;
  const int rotH = srcW;
  int cropW = rotW;
  int cropH = (rotW * 3) / 4;
  if (cropH > rotH) {
    cropH = rotH;
    cropW = (rotH * 4) / 3;
  }
  const int offX = (rotW - cropW) / 2;
  const int offY = (rotH - cropH) / 2;

  // Precompute sy[dx] coordinate lookup (zero division in inner loop!)
  int syLut[400];
  const int clampedW = dstW > 400 ? 400 : dstW;
  for (int dx = 0; dx < clampedW; ++dx) {
    const int mx = mirror ? (dstW - 1 - dx) : dx;
    const int rx = offX + (mx * cropW) / dstW;
    syLut[dx] = (srcH - 1 - rx) * srcW;
  }

  const uint8_t* lut = getContrastLut(contrast);
  const int rowBytes = (dstW + 7) / 8;

  for (int dy = 0; dy < dstH; ++dy) {
    const int sx = offY + (dy * cropH) / dstH;
    uint8_t* outRow = dstPlane + dy * rowBytes;
    for (int bx = 0; bx < rowBytes; ++bx) {
      const int dxBase = bx << 3;
      uint8_t acc = 0;
      const int rem = clampedW - dxBase;
      const int bits = rem >= 8 ? 8 : rem;
      for (int bit = 0; bit < bits; ++bit) {
        const int dx = dxBase + bit;
        const uint8_t luma = lut[src[syLut[dx] + sx]];
        if (quantizeLuma1BitFast(luma, dx, dy, look)) {
          acc |= static_cast<uint8_t>(0x80u >> bit);
        }
      }
      outRow[bx] = acc;
    }
  }
}

// Fast 4:3 letterboxed OLED 1-bit renderer from already-rotated/cropped 4:3 grayscale (or raw 90CW source)
template <typename SetPxFn>
inline void blitGray90Cw43LetterboxOled(const uint8_t* src, int srcW, int srcH,
                                        int dstX, int dstY, int dstW, int dstH,
                                        SetPxFn setPx, uint8_t look, uint8_t contrast, bool mirror) {
  if (!src || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
  const int rotW = srcH;
  const int rotH = srcW;
  int cropW = rotW;
  int cropH = (rotW * 3) / 4;
  if (cropH > rotH) {
    cropH = rotH;
    cropW = (rotH * 4) / 3;
  }
  const int offX = (rotW - cropW) / 2;
  const int offY = (rotH - cropH) / 2;

  // Exact 4:3 letterbox inside dstW x dstH (e.g. 68x51 centered inside 128x53)
  int boxH = (dstH / 3) * 3;
  if (boxH < 3) boxH = dstH;
  int boxW = (boxH * 4) / 3;
  if (boxW > dstW) {
    boxW = (dstW / 4) * 4;
    if (boxW < 4) boxW = dstW;
    boxH = (boxW * 3) / 4;
  }
  const int padX = (dstW - boxW) / 2;
  const int padY = (dstH - boxH) / 2;
  const uint8_t* lut = getContrastLut(contrast);

  for (int y = 0; y < boxH; ++y) {
    const int ry = offY + (y * cropH) / boxH;
    const int sx = ry;
    for (int x = 0; x < boxW; ++x) {
      const int mx = mirror ? (boxW - 1 - x) : x;
      const int rx = offX + (mx * cropW) / boxW;
      const int sy = srcH - 1 - rx;
      const uint8_t adj = lut[src[sy * srcW + sx]];
      const bool lit = quantizeLuma1BitFast(adj, x, y, look);
      setPx(dstX + padX + x, dstY + padY + y, lit);
    }
  }
}

// Aspect-preserving center-crop blit from ALREADY-ORIENTED 8-bit grayscale (used for saved 600x450 photos)
template <typename SetPxFn>
inline void blitGrayAspectCrop1Bit(const uint8_t* src, int srcW, int srcH,
                                   int dstX, int dstY, int dstW, int dstH,
                                   SetPxFn setPx, uint8_t look, uint8_t contrast,
                                   bool mirror, bool invertOutput) {
  if (!src || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
  const uint8_t* lut = getContrastLut(contrast);

  const float srcAspect = static_cast<float>(srcW) / static_cast<float>(srcH);
  const float dstAspect = static_cast<float>(dstW) / static_cast<float>(dstH);
  int cropW = srcW;
  int cropH = srcH;
  if (srcAspect > dstAspect) {
    cropW = static_cast<int>(static_cast<float>(srcH) * dstAspect + 0.5f);
    if (cropW < 1) cropW = 1;
    if (cropW > srcW) cropW = srcW;
  } else {
    cropH = static_cast<int>(static_cast<float>(srcW) / dstAspect + 0.5f);
    if (cropH < 1) cropH = 1;
    if (cropH > srcH) cropH = srcH;
  }
  const int offX = (srcW - cropW) / 2;
  const int offY = (srcH - cropH) / 2;

  for (int y = 0; y < dstH; ++y) {
    const int sy = offY + (y * cropH) / dstH;
    const uint8_t* rowPtr = src + sy * srcW;
    for (int x = 0; x < dstW; ++x) {
      const int rx = mirror ? (dstW - 1 - x) : x;
      const int sx = offX + (rx * cropW) / dstW;
      const bool light = quantizeLuma1BitFast(lut[rowPtr[sx]], x, y, look);
      const bool pxVal = invertOutput ? !light : light;
      setPx(dstX + x, dstY + y, pxVal);
    }
  }
}

// Aspect-preserving letterbox fit from ALREADY-ORIENTED 8-bit grayscale (no crop, no stretch).
// For 4:3 source in dstW x dstH (e.g. 300x225 or 600x450 into 384x206 gallery box), fits 272x204 exact 4:3.
template <typename SetPxFn>
inline void blitGrayFitAspect1Bit(const uint8_t* src, int srcW, int srcH,
                                  int dstX, int dstY, int dstW, int dstH,
                                  SetPxFn setPx, uint8_t look, uint8_t contrast,
                                  bool mirror, bool invertOutput) {
  if (!src || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
  const uint8_t* lut = getContrastLut(contrast);

  int fitW = dstW;
  int fitH = dstH;
  if (srcW * 3 == srcH * 4) {
    // Exact 4:3 integer box
    fitH = (dstH / 3) * 3;
    if (fitH < 3) fitH = dstH;
    fitW = (fitH * 4) / 3;
    if (fitW > dstW) {
      fitW = (dstW / 4) * 4;
      if (fitW < 4) fitW = dstW;
      fitH = (fitW * 3) / 4;
    }
  } else {
    fitW = (fitH * srcW) / srcH;
    if (fitW > dstW) {
      fitW = dstW;
      fitH = (fitW * srcH) / srcW;
    }
  }
  if (fitW < 1) fitW = 1;
  if (fitH < 1) fitH = 1;
  const int padX = (dstW - fitW) / 2;
  const int padY = (dstH - fitH) / 2;

  for (int y = 0; y < fitH; ++y) {
    const int sy = (y * srcH) / fitH;
    const uint8_t* rowPtr = src + sy * srcW;
    for (int x = 0; x < fitW; ++x) {
      const int rx = mirror ? (fitW - 1 - x) : x;
      const int sx = (rx * srcW) / fitW;
      const bool light = quantizeLuma1BitFast(lut[rowPtr[sx]], x, y, look);
      const bool pxVal = invertOutput ? !light : light;
      setPx(dstX + padX + x, dstY + padY + y, pxVal);
    }
  }
}

// Explicit non-private synthetic asymmetric pattern generator for TESTMODE verification.
// Top-left bright marker block + gradient + frame bar makes 90CW rotation unambiguously testable!
inline void generateSyntheticGray(uint8_t* dst, int w, int h, uint32_t seq) {
  if (!dst || w <= 0 || h <= 0) return;
  const int barX = static_cast<int>((seq * 11u) % static_cast<uint32_t>(w));
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint8_t val = static_cast<uint8_t>((x * 255) / std::max(1, w - 1));
      // Asymmetric bright corner marker in top-left (x < w/4, y < h/4)
      if (x < w / 4 && y < h / 4) {
        val = 250;
      } else if (x > (3 * w) / 4 && y > (3 * h) / 4) {
        val = 15;  // Dark bottom-right corner marker
      }
      if (std::abs(x - barX) < 6) {
        val = 30;
      }
      dst[y * w + x] = val;
    }
  }
}

}  // namespace flipcam
