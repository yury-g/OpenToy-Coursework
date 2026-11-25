// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include "FlipCamCrc.h"

namespace flipcam {

// Persistent Binary 1-Bit Preview Stream Protocol ('FMB1')
// Exact 4:3 landscape mono frame (304x228) centered inside RLCD 384x232 viewport (8, 44).
constexpr uint32_t kFmbMagic = 0x31424D46u;  // 'FMB1'
constexpr uint16_t kFmbViewW = 304;
constexpr uint16_t kFmbViewH = 228;
constexpr size_t kFmbRawPlaneBytes = (kFmbViewW / 8) * kFmbViewH;  // 8,664 bytes
constexpr size_t kFmbMaxPayloadBytes = 15000u;

constexpr int kRlcdViewportX = 8;
constexpr int kRlcdViewportY = 44;
constexpr int kRlcdViewportW = 384;
constexpr int kRlcdViewportH = 232;
constexpr int kFmbDrawOffsetX = (kRlcdViewportW - static_cast<int>(kFmbViewW)) / 2;  // 40 px
constexpr int kFmbDrawOffsetY = (kRlcdViewportH - static_cast<int>(kFmbViewH)) / 2;  // 2 px

enum class FmbCodec : uint8_t {
  RAW_1BIT = 0,
  PACKBITS_RLE_1BIT = 1,
};

#pragma pack(push, 1)
struct FmbFrameHeader {
  uint32_t magic;
  uint16_t width;
  uint16_t height;
  uint8_t codec;
  uint8_t flags;
  uint16_t reserved;
  uint32_t seq;
  uint32_t ageMs;
  uint32_t payloadLen;
  uint32_t payloadCrc32;
  uint32_t headerCrc32;
};
#pragma pack(pop)

static_assert(sizeof(FmbFrameHeader) == 32, "FmbFrameHeader must be 32 bytes");

inline void finalizeFmbHeader(FmbFrameHeader& hdr) {
  hdr.magic = kFmbMagic;
  hdr.headerCrc32 = crc32Compute(reinterpret_cast<const uint8_t*>(&hdr),
                                 sizeof(FmbFrameHeader) - sizeof(uint32_t));
}

inline bool validateFmbHeader(const FmbFrameHeader& hdr) {
  if (hdr.magic != kFmbMagic) return false;
  if (hdr.width != kFmbViewW || hdr.height != kFmbViewH) return false;
  if (hdr.codec != static_cast<uint8_t>(FmbCodec::RAW_1BIT) &&
      hdr.codec != static_cast<uint8_t>(FmbCodec::PACKBITS_RLE_1BIT)) {
    return false;
  }
  if (hdr.payloadLen == 0 || hdr.payloadLen > kFmbMaxPayloadBytes) return false;
  if (hdr.codec == static_cast<uint8_t>(FmbCodec::RAW_1BIT) &&
      hdr.payloadLen != kFmbRawPlaneBytes) {
    return false;
  }
  const uint32_t expected = crc32Compute(reinterpret_cast<const uint8_t*>(&hdr),
                                         sizeof(FmbFrameHeader) - sizeof(uint32_t));
  return hdr.headerCrc32 == expected;
}

// Bounded PackBits RLE encoder:
// - control byte n in [0..127]: copy next (n + 1) literal bytes
// - control byte n in [-127..-1] (int8_t): repeat next byte (1 - n) times
// - -128 (0x80): reserved/no-op (rejected if malformed)
inline size_t packBitsCompress(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstCap) {
  if (!src || !dst || srcLen == 0) return 0;
  size_t i = 0;
  size_t out = 0;
  while (i < srcLen) {
    // Check run >= 3 bytes
    size_t run = 1;
    while (i + run < srcLen && run < 128 && src[i + run] == src[i]) {
      ++run;
    }
    if (run >= 3) {
      if (out + 2 > dstCap) return 0;
      dst[out++] = static_cast<uint8_t>(static_cast<int8_t>(1 - static_cast<int>(run)));
      dst[out++] = src[i];
      i += run;
    } else {
      const size_t litStart = i;
      size_t litLen = 0;
      while (i < srcLen && litLen < 128) {
        size_t r = 1;
        while (i + r < srcLen && r < 3 && src[i + r] == src[i]) ++r;
        if (r >= 3) break;
        ++i;
        ++litLen;
      }
      if (out + 1 + litLen > dstCap) return 0;
      dst[out++] = static_cast<uint8_t>(litLen - 1);
      std::memcpy(dst + out, src + litStart, litLen);
      out += litLen;
    }
  }
  return out;
}

inline bool packBitsDecompress(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t expectedDstLen) {
  if (!src || !dst || srcLen == 0) return false;
  size_t i = 0;
  size_t out = 0;
  while (i < srcLen) {
    const int8_t ctrl = static_cast<int8_t>(src[i++]);
    if (ctrl >= 0) {
      const size_t n = static_cast<size_t>(ctrl) + 1u;
      if (i + n > srcLen || out + n > expectedDstLen) return false;
      std::memcpy(dst + out, src + i, n);
      i += n;
      out += n;
    } else if (ctrl != -128) {
      const size_t n = static_cast<size_t>(1 - ctrl);
      if (i >= srcLen || out + n > expectedDstLen) return false;
      const uint8_t val = src[i++];
      std::memset(dst + out, val, n);
      out += n;
    } else {
      return false;  // reject invalid 0x80 control byte
    }
  }
  return out == expectedDstLen;
}

// Encodes raw 11,136-byte 1-bit plane using RLE if smaller, otherwise falls back to RAW_1BIT.
inline void encodeFmbFrame(const uint8_t* rawPlane, uint32_t seq, uint32_t ageMs, uint8_t flags,
                           FmbFrameHeader& outHdr, std::vector<uint8_t>& outPayload) {
  outPayload.resize(kFmbRawPlaneBytes);
  const size_t rleLen = packBitsCompress(rawPlane, kFmbRawPlaneBytes, outPayload.data(), kFmbRawPlaneBytes);
  outHdr = FmbFrameHeader{};
  outHdr.width = kFmbViewW;
  outHdr.height = kFmbViewH;
  outHdr.seq = seq;
  outHdr.ageMs = ageMs;
  outHdr.flags = flags;
  if (rleLen > 0 && rleLen < kFmbRawPlaneBytes) {
    outPayload.resize(rleLen);
    outHdr.codec = static_cast<uint8_t>(FmbCodec::PACKBITS_RLE_1BIT);
  } else {
    outPayload.assign(rawPlane, rawPlane + kFmbRawPlaneBytes);
    outHdr.codec = static_cast<uint8_t>(FmbCodec::RAW_1BIT);
  }
  outHdr.payloadLen = static_cast<uint32_t>(outPayload.size());
  outHdr.payloadCrc32 = crc32Compute(outPayload.data(), outPayload.size());
  finalizeFmbHeader(outHdr);
}

inline bool decodeFmbPayload(const FmbFrameHeader& hdr, const uint8_t* payload, size_t payloadLen,
                             uint8_t* outRawPlane, size_t outCap) {
  if (!validateFmbHeader(hdr) || !payload || !outRawPlane || outCap < kFmbRawPlaneBytes) {
    return false;
  }
  if (payloadLen != hdr.payloadLen) return false;
  if (crc32Compute(payload, payloadLen) != hdr.payloadCrc32) return false;
  if (hdr.codec == static_cast<uint8_t>(FmbCodec::RAW_1BIT)) {
    std::memcpy(outRawPlane, payload, kFmbRawPlaneBytes);
    return true;
  }
  return packBitsDecompress(payload, payloadLen, outRawPlane, kFmbRawPlaneBytes);
}

}  // namespace flipcam
