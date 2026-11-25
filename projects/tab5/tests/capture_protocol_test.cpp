// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "../adapter/Tab5Battery.h"
#include "../adapter/Tab5CameraAdapter.h"
#include "../adapter/Tab5CaptureProtocol.h"

using namespace opentoy::tab5;

namespace {

std::vector<uint8_t> makeSampleJpeg() {
  return {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00, 0x01, 0xFF, 0xD9};
}

void testBatteryFormatting() {
  char buf[32] = {};
  BatterySample unavailable;
  assert(!formatBatteryStatus(unavailable, 1000, buf, sizeof(buf)));
  assert(std::strcmp(buf, "BAT --") == 0);

  BatterySample valid_2s;
  valid_2s.kind = BatteryReadingKind::MeasuredMillivolts2S;
  valid_2s.value = 7424;
  valid_2s.sampled_ms = 5000;
  valid_2s.valid = true;
  assert(formatBatteryStatus(valid_2s, 5500, buf, sizeof(buf)));
  assert(std::strcmp(buf, "BAT 7.42V") == 0);

  // Stale reading (> 5000 ms) must report neutral BAT --, never stale voltage or empty.
  assert(!formatBatteryStatus(valid_2s, 12000, buf, sizeof(buf)));
  assert(std::strcmp(buf, "BAT --") == 0);

  // Out-of-range voltage (USB-only / bogus) must report BAT --.
  BatterySample low_rail = valid_2s;
  low_rail.value = 4900;
  assert(!formatBatteryStatus(low_rail, 5100, buf, sizeof(buf)));
  assert(std::strcmp(buf, "BAT --") == 0);

  // Wrap-around timestamp safety.
  BatterySample wrap = valid_2s;
  wrap.sampled_ms = 0xFFFFFF00u;
  assert(formatBatteryStatus(wrap, 0x00000100u, buf, sizeof(buf)));
  assert(std::strcmp(buf, "BAT 7.42V") == 0);
}

void testFreshnessAndJpegGuards() {
  const auto jpg = makeSampleJpeg();
  CapturedFrame frame;
  frame.data = jpg.data();
  frame.byte_length = jpg.size();
  frame.width = 1280;
  frame.height = 720;
  frame.format = PixelFormat::Jpeg;
  frame.sequence = 10;
  frame.captured_ms = 2000;
  frame.sensor_id = kSc202csSensorId;
  frame.live_hardware = true;
  frame.ae_warmed = true;

  assert(hasValidJpegMarkers(jpg.data(), jpg.size()));
  assert(isFreshFrame(9, frame, 2100));
  // Stale sequence (<= seq_before) must fail.
  assert(!isFreshFrame(10, frame, 2100));
  assert(!isFreshFrame(11, frame, 2100));
  // Stale age (> 250 ms) must fail.
  assert(!isFreshFrame(9, frame, 2251));

  // Missing EOI marker must fail.
  std::vector<uint8_t> bad_jpg = jpg;
  bad_jpg.back() = 0x00;
  assert(!hasValidJpegMarkers(bad_jpg.data(), bad_jpg.size()));
}

void testProtocolRoundTripAndCorruption() {
  const auto jpg = makeSampleJpeg();
  FrameHeader hdr;
  hdr.payload_len = uint32_t(jpg.size());
  hdr.payload_crc32 = crc32Bytes(jpg.data(), jpg.size());
  hdr.sequence = 42;
  hdr.captured_ms = 10000;
  hdr.sent_ms = 10045;
  hdr.width = 1280;
  hdr.height = 720;
  hdr.sensor_id = kSc202csSensorId;
  hdr.format = uint8_t(PixelFormat::Jpeg);
  hdr.flags = kFlagLiveHardware | kFlagAeWarmed;

  uint8_t raw_hdr[kHeaderBytes] = {};
  encodeFrameHeader(hdr, raw_hdr);

  // Construct stream prefixed by ROM boot noise before magic.
  std::vector<uint8_t> stream = {'E', 'S', 'P', '-', 'R', 'O', 'M', '\r', '\n'};
  stream.insert(stream.end(), kPacketMagic, kPacketMagic + sizeof(kPacketMagic));
  stream.insert(stream.end(), raw_hdr, raw_hdr + sizeof(raw_hdr));
  stream.insert(stream.end(), jpg.begin(), jpg.end());
  stream.insert(stream.end(), kPacketTrailer, kPacketTrailer + sizeof(kPacketTrailer));

  const size_t magic_off = findMagicOffset(stream.data(), stream.size());
  assert(magic_off == 9);
  const uint8_t* pkt = stream.data() + magic_off + sizeof(kPacketMagic);

  FrameHeader decoded;
  assert(decodeFrameHeader(pkt, decoded));
  assert(decoded.payload_len == jpg.size());
  assert(decoded.sequence == 42);
  assert(decoded.sensor_id == kSc202csSensorId);

  const uint8_t* payload = pkt + kHeaderBytes;
  assert(validatePacketPayload(decoded, payload, decoded.payload_len, 41));
  assert(!validatePacketPayload(decoded, payload, decoded.payload_len, 42));  // seq not strictly newer
  assert(std::memcmp(payload + decoded.payload_len, kPacketTrailer, sizeof(kPacketTrailer)) == 0);

  // Header bit flip must be rejected.
  uint8_t bad_hdr[kHeaderBytes] = {};
  std::memcpy(bad_hdr, raw_hdr, kHeaderBytes);
  bad_hdr[8] ^= 0x01;
  FrameHeader rejected;
  assert(!decodeFrameHeader(bad_hdr, rejected));

  // Payload bit flip must be rejected.
  auto bad_jpg = jpg;
  bad_jpg[5] ^= 0x80;
  assert(!validatePacketPayload(decoded, bad_jpg.data(), bad_jpg.size(), 41));

  // Truncated RGB565 payload must be rejected.
  FrameHeader rgb_hdr = hdr;
  rgb_hdr.format = uint8_t(PixelFormat::Rgb565Le);
  rgb_hdr.width = 320;
  rgb_hdr.height = 180;
  std::vector<uint8_t> short_rgb(100, 0x5A);
  rgb_hdr.payload_len = uint32_t(short_rgb.size());
  rgb_hdr.payload_crc32 = crc32Bytes(short_rgb.data(), short_rgb.size());
  assert(!validatePacketPayload(rgb_hdr, short_rgb.data(), short_rgb.size(), 41));

  // Valid full RGB565 payload must pass.
  std::vector<uint8_t> valid_rgb(320 * 180 * 2, 0x5A);
  rgb_hdr.payload_len = uint32_t(valid_rgb.size());
  rgb_hdr.payload_crc32 = crc32Bytes(valid_rgb.data(), valid_rgb.size());
  assert(validatePacketPayload(rgb_hdr, valid_rgb.data(), valid_rgb.size(), 41));
}

}  // namespace

int main() {
  testBatteryFormatting();
  testFreshnessAndJpegGuards();
  testProtocolRoundTripAndCorruption();
  std::cout << "PASS Tab5 battery, freshness, JPEG/RGB565 framing, and CRC32 protocol tests\n";
  return 0;
}
