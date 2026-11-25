// SPDX-License-Identifier: MIT
#pragma once
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "FlipCamCrc.h"

namespace flipcam {

// ---------------------------------------------------------------------------
// Safe ID and Path Traversal Guards
// ---------------------------------------------------------------------------
inline bool isValidMediaName(const char* name) {
  if (!name || !name[0]) return false;
  size_t len = 0;
  for (; name[len] != '\0'; ++len) {
    if (len >= 31) return false;
    const unsigned char c = static_cast<unsigned char>(name[len]);
    const bool ok = std::isalnum(c) || c == '_' || c == '-' || c == '.';
    if (!ok) return false;
  }
  if (std::strstr(name, "..") != nullptr) return false;
  if (name[0] == '.') return false;
  return true;
}

inline bool buildSafeMediaPath(const char* baseDir, const char* filename, char* out, size_t outCap) {
  if (!baseDir || !out || outCap < 16) return false;
  if (!isValidMediaName(filename)) return false;
  const int n = std::snprintf(out, outCap, "%s/%s", baseDir, filename);
  return n > 0 && static_cast<size_t>(n) < outCap;
}

inline bool parsePositiveId(const char* s, uint32_t& outId) {
  if (!s || !s[0]) return false;
  uint64_t acc = 0;
  for (size_t i = 0; s[i] != '\0'; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    acc = acc * 10u + static_cast<uint64_t>(s[i] - '0');
    if (acc > 0xFFFFFFFFull) return false;
  }
  if (acc == 0) return false;
  outId = static_cast<uint32_t>(acc);
  return true;
}

// ---------------------------------------------------------------------------
// JPEG Sanity Bounds
// ---------------------------------------------------------------------------
constexpr size_t kMinJpegBytes = 64;
constexpr size_t kMaxStillJpegBytes = 512u * 1024u;
constexpr size_t kMaxPreviewJpegBytes = 96u * 1024u;

inline bool isValidJpegBuffer(const uint8_t* data, size_t len, size_t maxBytes = kMaxStillJpegBytes) {
  if (!data || len < kMinJpegBytes || len > maxBytes) return false;
  if (data[0] != 0xFF || data[1] != 0xD8) return false;
  // Require EOI marker at or near end (some sensors append 0..2 padding bytes, check last 4 bytes)
  for (size_t i = len - 2; i + 4 >= len && i >= 2; --i) {
    if (data[i] == 0xFF && data[i + 1] == 0xD9) return true;
  }
  return false;
}

// Parse actual JPEG frame width/height from SOF marker (independent of stale driver fb->width)
inline bool parseJpegDimensions(const uint8_t* data, size_t len, uint16_t& outW, uint16_t& outH) {
  outW = 0;
  outH = 0;
  if (!isValidJpegBuffer(data, len)) return false;
  size_t i = 2;
  while (i + 1 < len) {
    while (i < len && data[i] != 0xFF) ++i;
    while (i < len && data[i] == 0xFF) ++i;
    if (i >= len) return false;
    const uint8_t marker = data[i++];
    if (marker == 0x00 || (marker >= 0xD0 && marker <= 0xD9) || marker == 0x01) {
      continue;
    }
    if (marker == 0xDA) {
      break;  // Start of Scan reached before SOF
    }
    if (i + 2 > len) return false;
    const uint16_t segLen = static_cast<uint16_t>((data[i] << 8) | data[i + 1]);
    if (segLen < 2 || i + segLen > len) return false;
    const bool isSof = ((marker >= 0xC0 && marker <= 0xC3) ||
                        (marker >= 0xC5 && marker <= 0xC7) ||
                        (marker >= 0xC9 && marker <= 0xCB) ||
                        (marker >= 0xCD && marker <= 0xCF));
    if (isSof) {
      if (segLen < 7 || i + 7 > len) return false;
      outH = static_cast<uint16_t>((data[i + 3] << 8) | data[i + 4]);
      outW = static_cast<uint16_t>((data[i + 5] << 8) | data[i + 6]);
      return outW > 0 && outH > 0;
    }
    i += segLen;
  }
  return false;
}

// ---------------------------------------------------------------------------
// FCV1 Bounded Video + Audio Clip Container
// ---------------------------------------------------------------------------
constexpr uint32_t kFcvMagic = 0x31564346u;    // 'FCV1' little-endian
constexpr uint32_t kChunkFrame = 0x454D5246u;  // 'FRME'
constexpr uint32_t kChunkAudio = 0x49445541u;  // 'AUDI'
constexpr uint16_t kFcvVersion = 1;
constexpr uint16_t kFcvFlagHasAudio = 0x0001u;
constexpr uint16_t kFcvFlagSynthetic = 0x0002u;

constexpr uint32_t kMaxClipDurationMs = 10000u;
constexpr uint32_t kMaxClipFileBytes = 2u * 1024u * 1024u;
constexpr uint32_t kMaxFrameChunkBytes = 128u * 1024u;
constexpr uint32_t kMaxAudioChunkBytes = 32u * 1024u;

#pragma pack(push, 1)
struct FcvFileHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t flags;
  uint32_t sampleRate;
  uint16_t channels;
  uint16_t bitsPerSample;
  uint32_t durationMs;
  uint32_t frameCount;
  uint32_t audioBytes;
  uint32_t headerCrc32;
};

struct FcvChunkHeader {
  uint32_t chunkMagic;
  uint32_t timestampMs;
  uint32_t payloadLen;
  uint32_t payloadCrc32;
};
#pragma pack(pop)

static_assert(sizeof(FcvFileHeader) == 32, "FcvFileHeader must be 32 bytes");
static_assert(sizeof(FcvChunkHeader) == 16, "FcvChunkHeader must be 16 bytes");

inline void finalizeFcvHeader(FcvFileHeader& hdr) {
  hdr.magic = kFcvMagic;
  hdr.version = kFcvVersion;
  hdr.headerCrc32 = crc32Compute(reinterpret_cast<const uint8_t*>(&hdr),
                                 sizeof(FcvFileHeader) - sizeof(uint32_t));
}

inline bool validateFcvHeader(const FcvFileHeader& hdr) {
  if (hdr.magic != kFcvMagic || hdr.version != kFcvVersion) return false;
  if (hdr.channels != 1 || hdr.bitsPerSample != 16) return false;
  if (hdr.sampleRate < 8000u || hdr.sampleRate > 48000u) return false;
  if (hdr.durationMs > kMaxClipDurationMs) return false;
  const uint32_t expectedCrc = crc32Compute(reinterpret_cast<const uint8_t*>(&hdr),
                                            sizeof(FcvFileHeader) - sizeof(uint32_t));
  return hdr.headerCrc32 == expectedCrc;
}

inline bool validateFcvChunkHeader(const FcvChunkHeader& ch, size_t remainingFileBytes) {
  if (ch.chunkMagic != kChunkFrame && ch.chunkMagic != kChunkAudio) return false;
  if (ch.timestampMs > kMaxClipDurationMs) return false;
  if (ch.payloadLen == 0) return false;
  const uint32_t maxLen = (ch.chunkMagic == kChunkFrame) ? kMaxFrameChunkBytes : kMaxAudioChunkBytes;
  if (ch.payloadLen > maxLen) return false;
  if (static_cast<size_t>(ch.payloadLen) > remainingFileBytes) return false;
  return true;
}

// Validates a complete in-memory FCV1 buffer (used by host tests & verification)
struct FcvSummary {
  bool valid = false;
  uint32_t frames = 0;
  uint32_t audioChunks = 0;
  uint32_t audioBytes = 0;
  uint32_t durationMs = 0;
  bool hasAudio = false;
};

inline FcvSummary validateFcvBuffer(const uint8_t* buf, size_t len) {
  FcvSummary out{};
  if (!buf || len < sizeof(FcvFileHeader) || len > kMaxClipFileBytes) return out;
  FcvFileHeader hdr{};
  std::memcpy(&hdr, buf, sizeof(hdr));
  if (!validateFcvHeader(hdr)) return out;

  size_t off = sizeof(FcvFileHeader);
  uint32_t lastTs = 0;
  while (off < len) {
    if (len - off < sizeof(FcvChunkHeader)) return out;
    FcvChunkHeader ch{};
    std::memcpy(&ch, buf + off, sizeof(ch));
    off += sizeof(FcvChunkHeader);
    if (!validateFcvChunkHeader(ch, len - off)) return out;
    if (ch.timestampMs < lastTs) return out;
    lastTs = ch.timestampMs;

    const uint8_t* payload = buf + off;
    const uint32_t actualCrc = crc32Compute(payload, ch.payloadLen);
    if (actualCrc != ch.payloadCrc32) return out;

    if (ch.chunkMagic == kChunkFrame) {
      if (!isValidJpegBuffer(payload, ch.payloadLen, kMaxFrameChunkBytes)) return out;
      out.frames++;
    } else {
      if ((ch.payloadLen & 1u) != 0u) return out;  // 16-bit PCM must be even byte count
      out.audioChunks++;
      out.audioBytes += ch.payloadLen;
    }
    off += ch.payloadLen;
  }

  if (out.frames != hdr.frameCount || out.audioBytes != hdr.audioBytes) return out;
  if (out.frames == 0) return out;
  out.durationMs = hdr.durationMs;
  out.hasAudio = (hdr.flags & kFcvFlagHasAudio) != 0 && out.audioBytes > 0;
  out.valid = true;
  return out;
}

// ---------------------------------------------------------------------------
// FCAV Live Peer Poll Packet (XIAO -> RLCD during recording)
// ---------------------------------------------------------------------------
constexpr uint32_t kFcavMagic = 0x56414346u;  // 'FCAV'

#pragma pack(push, 1)
struct FcavPacketHeader {
  uint32_t magic;
  uint32_t seq;
  uint32_t timestampMs;
  uint32_t jpegLen;
  uint32_t pcmLen;
  uint32_t payloadCrc32;  // CRC32 over jpegBytes followed by pcmBytes
};
#pragma pack(pop)

static_assert(sizeof(FcavPacketHeader) == 24, "FcavPacketHeader must be 24 bytes");

inline bool validateFcavPacket(const uint8_t* buf, size_t len, FcavPacketHeader& outHdr) {
  if (!buf || len < sizeof(FcavPacketHeader)) return false;
  std::memcpy(&outHdr, buf, sizeof(outHdr));
  if (outHdr.magic != kFcavMagic) return false;
  if (outHdr.jpegLen > kMaxFrameChunkBytes || outHdr.pcmLen > kMaxAudioChunkBytes) return false;
  if ((outHdr.pcmLen & 1u) != 0u) return false;
  if (len != sizeof(FcavPacketHeader) + static_cast<size_t>(outHdr.jpegLen) +
                 static_cast<size_t>(outHdr.pcmLen)) {
    return false;
  }
  const uint32_t crc = crc32Compute(buf + sizeof(FcavPacketHeader),
                                    outHdr.jpegLen + outHdr.pcmLen);
  return crc == outHdr.payloadCrc32;
}

// ---------------------------------------------------------------------------
// Standard 44-byte RIFF/WAVE PCM Header
// ---------------------------------------------------------------------------
inline void buildWavHeader(uint8_t out[44], uint32_t sampleRate, uint16_t channels,
                           uint16_t bitsPerSample, uint32_t pcmDataBytes) {
  const uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8u);
  const uint16_t blockAlign = static_cast<uint16_t>(channels * (bitsPerSample / 8u));
  const uint32_t chunkSize = 36u + pcmDataBytes;

  auto put32 = [](uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
  };
  auto put16 = [](uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
  };

  std::memcpy(out + 0, "RIFF", 4);
  put32(out + 4, chunkSize);
  std::memcpy(out + 8, "WAVE", 4);
  std::memcpy(out + 12, "fmt ", 4);
  put32(out + 16, 16u);
  put16(out + 20, 1u);  // PCM
  put16(out + 22, channels);
  put32(out + 24, sampleRate);
  put32(out + 28, byteRate);
  put16(out + 32, blockAlign);
  put16(out + 34, bitsPerSample);
  std::memcpy(out + 36, "data", 4);
  put32(out + 40, pcmDataBytes);
}

}  // namespace flipcam
