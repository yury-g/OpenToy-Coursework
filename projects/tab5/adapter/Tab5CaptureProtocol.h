// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Tab5CameraAdapter.h"

namespace opentoy {
namespace tab5 {

constexpr uint8_t kPacketMagic[8] = {0x89, 'O', 'T', 'C', 'A', 'M', '1', '\r'};
constexpr uint8_t kPacketTrailer[4] = {'E', 'N', 'D', '\n'};
constexpr size_t kHeaderMetaBytes = 32;
constexpr size_t kHeaderBytes = 36;
constexpr size_t kMaxPayloadBytes = 2u * 1024u * 1024u;

constexpr uint8_t kFlagLiveHardware = 0x01;
constexpr uint8_t kFlagAeWarmed = 0x02;
constexpr uint8_t kFlagBurstLast = 0x04;

struct FrameHeader {
  uint32_t payload_len = 0;
  uint32_t payload_crc32 = 0;
  uint32_t sequence = 0;
  uint32_t captured_ms = 0;
  uint32_t sent_ms = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t sensor_id = 0;
  uint8_t format = 0;  // 1=JPEG, 2=RGB565LE
  uint8_t flags = 0;
  uint32_t reserved = 0;
  uint32_t header_crc32 = 0;
};

inline uint32_t crc32Bytes(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  if (!data && len > 0) return 0;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      const uint32_t mask = uint32_t(-(int32_t(crc & 1u)));
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

inline void writeLe16(uint8_t* dst, uint16_t v) {
  dst[0] = uint8_t(v & 0xFFu);
  dst[1] = uint8_t((v >> 8) & 0xFFu);
}

inline void writeLe32(uint8_t* dst, uint32_t v) {
  dst[0] = uint8_t(v & 0xFFu);
  dst[1] = uint8_t((v >> 8) & 0xFFu);
  dst[2] = uint8_t((v >> 16) & 0xFFu);
  dst[3] = uint8_t((v >> 24) & 0xFFu);
}

inline uint16_t readLe16(const uint8_t* src) {
  return uint16_t(src[0]) | (uint16_t(src[1]) << 8);
}

inline uint32_t readLe32(const uint8_t* src) {
  return uint32_t(src[0]) | (uint32_t(src[1]) << 8) |
         (uint32_t(src[2]) << 16) | (uint32_t(src[3]) << 24);
}

inline void encodeFrameHeader(const FrameHeader& hdr, uint8_t out[kHeaderBytes]) {
  writeLe32(out + 0, hdr.payload_len);
  writeLe32(out + 4, hdr.payload_crc32);
  writeLe32(out + 8, hdr.sequence);
  writeLe32(out + 12, hdr.captured_ms);
  writeLe32(out + 16, hdr.sent_ms);
  writeLe16(out + 20, hdr.width);
  writeLe16(out + 22, hdr.height);
  writeLe16(out + 24, hdr.sensor_id);
  out[26] = hdr.format;
  out[27] = hdr.flags;
  writeLe32(out + 28, hdr.reserved);
  const uint32_t hcrc = crc32Bytes(out, kHeaderMetaBytes);
  writeLe32(out + 32, hcrc);
}

inline bool decodeFrameHeader(const uint8_t in[kHeaderBytes], FrameHeader& out) {
  const uint32_t expected_crc = crc32Bytes(in, kHeaderMetaBytes);
  const uint32_t got_crc = readLe32(in + 32);
  if (got_crc != expected_crc) return false;

  out.payload_len = readLe32(in + 0);
  out.payload_crc32 = readLe32(in + 4);
  out.sequence = readLe32(in + 8);
  out.captured_ms = readLe32(in + 12);
  out.sent_ms = readLe32(in + 16);
  out.width = readLe16(in + 20);
  out.height = readLe16(in + 22);
  out.sensor_id = readLe16(in + 24);
  out.format = in[26];
  out.flags = in[27];
  out.reserved = readLe32(in + 28);
  out.header_crc32 = got_crc;

  if (out.payload_len == 0 || out.payload_len > kMaxPayloadBytes) return false;
  if (out.width == 0 || out.height == 0) return false;
  if (out.format != uint8_t(PixelFormat::Jpeg) &&
      out.format != uint8_t(PixelFormat::Rgb565Le)) {
    return false;
  }
  return true;
}

inline size_t findMagicOffset(const uint8_t* stream, size_t len) {
  if (!stream || len < sizeof(kPacketMagic)) return len;
  for (size_t i = 0; i + sizeof(kPacketMagic) <= len; ++i) {
    if (std::memcmp(stream + i, kPacketMagic, sizeof(kPacketMagic)) == 0) {
      return i;
    }
  }
  return len;
}

inline bool validatePacketPayload(const FrameHeader& hdr,
                                  const uint8_t* payload,
                                  size_t len,
                                  uint32_t seq_before,
                                  uint32_t max_age_ms = kMaxFreshAgeMs) {
  if (!payload || len != hdr.payload_len) return false;
  if (crc32Bytes(payload, len) != hdr.payload_crc32) return false;
  if (uint32_t(hdr.sent_ms - hdr.captured_ms) > max_age_ms) return false;
  if (hdr.sent_ms < hdr.captured_ms && (hdr.captured_ms - hdr.sent_ms) < 0x80000000u) {
    return false;
  }
  if (hdr.format == uint8_t(PixelFormat::Jpeg)) {
    if (!hasValidJpegMarkers(payload, len)) return false;
  } else if (hdr.format == uint8_t(PixelFormat::Rgb565Le)) {
    if (len != size_t(hdr.width) * size_t(hdr.height) * 2u) return false;
  } else {
    return false;
  }

  CapturedFrame frame;
  frame.data = payload;
  frame.byte_length = len;
  frame.width = hdr.width;
  frame.height = hdr.height;
  frame.format = static_cast<PixelFormat>(hdr.format);
  frame.sequence = hdr.sequence;
  frame.captured_ms = hdr.captured_ms;
  frame.sensor_id = hdr.sensor_id;
  frame.live_hardware = (hdr.flags & kFlagLiveHardware) != 0;
  frame.ae_warmed = (hdr.flags & kFlagAeWarmed) != 0;
  return isFreshFrame(seq_before, frame, hdr.sent_ms, max_age_ms);
}

}  // namespace tab5
}  // namespace opentoy
