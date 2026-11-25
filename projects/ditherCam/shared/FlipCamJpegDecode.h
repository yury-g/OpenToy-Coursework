// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include "FlipCamProtocol.h"
#include "FlipCamSoftJpgd.h"

#ifdef ESP32
#include <esp32s3/rom/tjpgd.h>
#endif

namespace flipcam {

struct JpegGrayDecodeCtx {
  const uint8_t* src = nullptr;
  size_t srcLen = 0;
  size_t pos = 0;
  uint8_t* dstGray = nullptr;
  int dstW = 0;
  int dstH = 0;
  int maxW = 0;
  int maxH = 0;
};

inline uint32_t softJpgdInputFunc(void* dev, uint8_t* buff, uint32_t nbyte) {
  auto* ctx = static_cast<JpegGrayDecodeCtx*>(dev);
  if (!ctx) return 0;
  const size_t rem = (ctx->pos < ctx->srcLen) ? (ctx->srcLen - ctx->pos) : 0;
  const size_t n = std::min<size_t>(nbyte, rem);
  if (buff && n > 0) {
    std::memcpy(buff, ctx->src + ctx->pos, n);
  }
  ctx->pos += n;
  return static_cast<uint32_t>(n);
}

inline uint32_t softJpgdOutputGrayFunc(void* dev, void* bitmap, FC_JRECT* rect) {
  auto* ctx = static_cast<JpegGrayDecodeCtx*>(dev);
  if (!ctx || !ctx->dstGray || !bitmap || !rect) return 0;
  const uint8_t* rgb = static_cast<const uint8_t*>(bitmap);
  const uint32_t rw = rect->right - rect->left + 1u;
  for (uint32_t y = rect->top; y <= rect->bottom; ++y) {
    const uint32_t rowOff = (y - rect->top) * rw;
    if (y >= static_cast<uint32_t>(ctx->dstH)) continue;
    uint8_t* dstRow = ctx->dstGray + y * static_cast<uint32_t>(ctx->dstW);
    for (uint32_t x = rect->left; x <= rect->right; ++x) {
      if (x >= static_cast<uint32_t>(ctx->dstW)) continue;
      const uint8_t* p = rgb + (rowOff + (x - rect->left)) * 3u;
      // BT.601 integer luma: (77*R + 150*G + 29*B) >> 8
      dstRow[x] = static_cast<uint8_t>((77u * p[0] + 150u * p[1] + 29u * p[2]) >> 8);
    }
  }
  return 1;
}

// Software JPEG decode supporting single-component monochrome (Nf=1) as well as 3-component JPEGs.
// Used on host tests and as RLCD gallery fallback when ESP32-S3 ROM TJpgDec rejects monochrome inputs.
inline bool decodeJpegToGraySoft(const uint8_t* jpeg, size_t len, uint8_t* dstBuf, size_t dstCap,
                                 int maxW, int maxH, int& outW, int& outH,
                                 std::vector<uint8_t>* scratchWork = nullptr) {
  outW = 0;
  outH = 0;
  if (!isValidJpegBuffer(jpeg, len, kMaxStillJpegBytes) || !dstBuf || dstCap == 0 ||
      maxW <= 0 || maxH <= 0) {
    return false;
  }

  static constexpr size_t kSoftWorkSize = 6144;
  std::vector<uint8_t> localWork;
  std::vector<uint8_t>& work = scratchWork ? *scratchWork : localWork;
  if (work.size() < kSoftWorkSize) work.resize(kSoftWorkSize);

  FC_JDEC jdec{};
  JpegGrayDecodeCtx ctx{};
  ctx.src = jpeg;
  ctx.srcLen = len;
  ctx.dstGray = dstBuf;
  ctx.maxW = maxW;
  ctx.maxH = maxH;

  if (fc_jd_prepare(&jdec, softJpgdInputFunc, work.data(), kSoftWorkSize, &ctx) != FC_JDR_OK) {
    return false;
  }

  uint_fast8_t scale = 0;
  while (scale < 3) {
    const int sw = static_cast<int>(jdec.width >> scale);
    const int sh = static_cast<int>(jdec.height >> scale);
    if (sw <= maxW && sh <= maxH && static_cast<size_t>(sw) * static_cast<size_t>(sh) <= dstCap) {
      break;
    }
    scale++;
  }

  ctx.dstW = static_cast<int>(jdec.width >> scale);
  ctx.dstH = static_cast<int>(jdec.height >> scale);
  if (ctx.dstW <= 0 || ctx.dstH <= 0 ||
      static_cast<size_t>(ctx.dstW) * static_cast<size_t>(ctx.dstH) > dstCap) {
    return false;
  }

  if (fc_jd_decomp(&jdec, softJpgdOutputGrayFunc, scale) != FC_JDR_OK) {
    return false;
  }
  outW = ctx.dstW;
  outH = ctx.dstH;
  return true;
}

#ifdef ESP32

inline UINT tjpgdInputFunc(JDEC* jd, BYTE* buff, UINT nbyte) {
  auto* ctx = static_cast<JpegGrayDecodeCtx*>(jd->device);
  const size_t rem = (ctx->pos < ctx->srcLen) ? (ctx->srcLen - ctx->pos) : 0;
  const size_t n = std::min<size_t>(nbyte, rem);
  if (buff && n > 0) {
    std::memcpy(buff, ctx->src + ctx->pos, n);
  }
  ctx->pos += n;
  return static_cast<UINT>(n);
}

inline UINT tjpgdOutputGrayFunc(JDEC* jd, void* bitmap, JRECT* rect) {
  auto* ctx = static_cast<JpegGrayDecodeCtx*>(jd->device);
  if (!ctx || !ctx->dstGray || !bitmap || !rect) return 0;
  const uint8_t* rgb = static_cast<const uint8_t*>(bitmap);
  const int rw = rect->right - rect->left + 1;
  for (int y = rect->top; y <= rect->bottom; ++y) {
    const int rowOff = (y - rect->top) * rw;
    if (y >= ctx->dstH) continue;
    uint8_t* dstRow = ctx->dstGray + y * ctx->dstW;
    for (int x = rect->left; x <= rect->right; ++x) {
      if (x >= ctx->dstW) continue;
      const uint8_t* p = rgb + (rowOff + (x - rect->left)) * 3;
      // BT.601 integer luma: (77*R + 150*G + 29*B) >> 8
      dstRow[x] = static_cast<uint8_t>((77u * p[0] + 150u * p[1] + 29u * p[2]) >> 8);
    }
  }
  return 1;
}

// Decodes a JPEG buffer into 8-bit grayscale via fast ESP32-S3 ROM TJpgDec, automatically choosing
// scale (0..3) so decoded dimensions fit within maxW x maxH. When allowSoftFallback is true (RLCD gallery),
// transparently falls back to FlipCamSoftJpgd for monochrome (1-component) JPEGs unsupported by ROM.
inline bool decodeJpegToGray(const uint8_t* jpeg, size_t len, uint8_t* dstBuf, size_t dstCap,
                             int maxW, int maxH, int& outW, int& outH,
                             std::vector<uint8_t>* scratchWork = nullptr,
                             bool allowSoftFallback = false) {
  outW = 0;
  outH = 0;
  if (!isValidJpegBuffer(jpeg, len, kMaxStillJpegBytes) || !dstBuf || dstCap == 0 ||
      maxW <= 0 || maxH <= 0) {
    return false;
  }

  static constexpr size_t kWorkSize = 4096;
  std::vector<uint8_t> localWork;
  std::vector<uint8_t>& work = scratchWork ? *scratchWork : localWork;
  if (work.size() < kWorkSize) work.resize(kWorkSize);
  JDEC jdec{};
  JpegGrayDecodeCtx ctx{};
  ctx.src = jpeg;
  ctx.srcLen = len;
  ctx.dstGray = dstBuf;
  ctx.maxW = maxW;
  ctx.maxH = maxH;

  bool romOk = (jd_prepare(&jdec, tjpgdInputFunc, work.data(), kWorkSize, &ctx) == JDR_OK);
  if (romOk) {
    uint8_t scale = 0;
    while (scale < 3) {
      const int sw = static_cast<int>(jdec.width >> scale);
      const int sh = static_cast<int>(jdec.height >> scale);
      if (sw <= maxW && sh <= maxH && static_cast<size_t>(sw) * static_cast<size_t>(sh) <= dstCap) {
        break;
      }
      scale++;
    }

    ctx.dstW = static_cast<int>(jdec.width >> scale);
    ctx.dstH = static_cast<int>(jdec.height >> scale);
    if (ctx.dstW <= 0 || ctx.dstH <= 0 ||
        static_cast<size_t>(ctx.dstW) * static_cast<size_t>(ctx.dstH) > dstCap ||
        jd_decomp(&jdec, tjpgdOutputGrayFunc, scale) != JDR_OK) {
      romOk = false;
    } else {
      outW = ctx.dstW;
      outH = ctx.dstH;
      return true;
    }
  }

  if (allowSoftFallback) {
    return decodeJpegToGraySoft(jpeg, len, dstBuf, dstCap, maxW, maxH, outW, outH, scratchWork);
  }
  return false;
}

#else

inline bool decodeJpegToGray(const uint8_t* jpeg, size_t len, uint8_t* dstBuf, size_t dstCap,
                             int maxW, int maxH, int& outW, int& outH,
                             std::vector<uint8_t>* scratchWork = nullptr,
                             bool /*allowSoftFallback*/ = false) {
  return decodeJpegToGraySoft(jpeg, len, dstBuf, dstCap, maxW, maxH, outW, outH, scratchWork);
}

#endif  // ESP32

}  // namespace flipcam
