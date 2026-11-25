// SPDX-License-Identifier: MIT
// Host renderer invoking the actual shared FlipCam C++ display pipeline
// (FlipCamDither.h + FlipCamDisplay.h + FlipCamUiState.h + FlipCamButtons.h).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "../shared/FlipCamButtons.h"
#include "../shared/FlipCamCrc.h"
#include "../shared/FlipCamDisplay.h"
#include "../shared/FlipCamDither.h"
#include "../shared/FlipCamStream.h"
#include "../shared/FlipCamTypes.h"
#include "../shared/FlipCamUiState.h"

using namespace flipcam;

namespace {

std::vector<uint8_t> readRaw(const std::string& path, size_t expectedBytes) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "Failed to open input %s\n", path.c_str());
    std::exit(2);
  }
  std::vector<uint8_t> buf(expectedBytes);
  in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(expectedBytes));
  if (static_cast<size_t>(in.gcount()) != expectedBytes) {
    std::fprintf(stderr, "Short read on %s\n", path.c_str());
    std::exit(2);
  }
  return buf;
}

void writeRlcdPgm(const std::string& path, const RlcdFrameBuffer& fb) {
  std::ofstream out(path, std::ios::binary);
  out << "P5\n400 300\n255\n";
  for (int y = 0; y < RlcdFrameBuffer::kHeight; ++y) {
    for (int x = 0; x < RlcdFrameBuffer::kWidth; ++x) {
      const uint8_t px = fb.getPixel(x, y) ? 255 : 0;
      out.put(static_cast<char>(px));
    }
  }
}

void writeOledPgm(const std::string& path, const OledFrameBuffer& fb) {
  std::ofstream out(path, std::ios::binary);
  out << "P5\n128 64\n255\n";
  for (int y = 0; y < OledFrameBuffer::kHeight; ++y) {
    for (int x = 0; x < OledFrameBuffer::kWidth; ++x) {
      const uint8_t px = fb.getPixel(x, y) ? 255 : 0;
      out.put(static_cast<char>(px));
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "Usage: %s <raw_selfie_320x240.bin> <raw_rear_flipped_320x240.bin> <gallery_600x450.bin> <out_dir>\n", argv[0]);
    return 1;
  }

  const auto rawSensor = readRaw(argv[1], 320u * 240u);
  const auto rawRearFlipped = readRaw(argv[2], 320u * 240u);
  const auto galleryGray = readRaw(argv[3], 600u * 450u);
  const std::string outDir = argv[4];

  // Produce exact 304x228 packed 1-bit FMB1 mono plane via firmware ditherer (90CW Selfie baseline)
  std::vector<uint8_t> liveMono(kFmbRawPlaneBytes, 0);
  dither90CwCrop43ToMonoPlane(rawSensor.data(), 320, 240, liveMono.data(), kFmbViewW, kFmbViewH,
                              static_cast<uint8_t>(LookMode::DITHER),
                              static_cast<uint8_t>(ContrastLevel::C_HIGH), false);

  // Verify RLE / FMB1 round-trip on this synthetic scene
  FmbFrameHeader fmbHdr{};
  std::vector<uint8_t> fmbPayload;
  encodeFmbFrame(liveMono.data(), 1, 0, 0, fmbHdr, fmbPayload);
  std::vector<uint8_t> decodedMono(kFmbRawPlaneBytes, 0);
  assert(validateFmbHeader(fmbHdr));
  assert(decodeFmbPayload(fmbHdr, fmbPayload.data(), fmbPayload.size(), decodedMono.data(), decodedMono.size()));
  assert(decodedMono == liveMono);

  ButtonController bc;
  bc.init(false, false, 0, true, false);
  FlipCamUiState ui;
  ui.storageMounted = true;
  ui.linkState = LinkState::CONNECTED;
  ui.setStillCount(3);
  ui.settings.look = static_cast<uint8_t>(LookMode::DITHER);
  ui.settings.contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
  ui.settings.mirror = 0;
  ui.settings.orientation = static_cast<uint8_t>(OrientationMode::SELFIE);

  RlcdFrameBuffer fbSelfie, fbHold, fbRear, fbGallery;
  RlcdRenderContext rctx{};
  rctx.ui = &ui;
  rctx.buttons = &bc;
  rctx.liveMonoPlane = liveMono.data();
  rctx.freeMiB = 7.4f;
  rctx.batteryPresent = false;
  rctx.batteryPct = -1;
  rctx.nowMs = 2000;
  rctx.peerCamOk = true;

  // 1. Selfie Live
  renderRlcdFrame(fbSelfie, rctx);
  writeRlcdPgm(outDir + "/rlcd-selfie-live.pgm", fbSelfie);

  // 2. Held Snap (~65% hold progress toward mode switch)
  bc.update(true, false, 2000);
  bc.update(true, false, 2030);
  bc.update(true, false, 2680);
  rctx.nowMs = 2680;
  renderRlcdFrame(fbHold, rctx);
  writeRlcdPgm(outDir + "/rlcd-snap-hold.pgm", fbHold);
  bc.update(false, false, 2700);
  bc.update(false, false, 2730);

  // 3. Rear Live on SAME stationary input (180-deg firmware transform test)
  ui.settings.orientation = static_cast<uint8_t>(OrientationMode::REAR);
  rctx.nowMs = 3000;
  renderRlcdFrame(fbRear, rctx);
  writeRlcdPgm(outDir + "/rlcd-rear-live.pgm", fbRear);

  // 3b. Rear Live with physically flipped pod input (180-deg pod + 180-deg REAR compensation -> upright)
  std::vector<uint8_t> rearUprightMono(kFmbRawPlaneBytes, 0);
  dither90CwCrop43ToMonoPlane(rawRearFlipped.data(), 320, 240, rearUprightMono.data(), kFmbViewW, kFmbViewH,
                              static_cast<uint8_t>(LookMode::DITHER),
                              static_cast<uint8_t>(ContrastLevel::C_HIGH), false);
  RlcdFrameBuffer fbRearUpright;
  rctx.liveMonoPlane = rearUprightMono.data();
  renderRlcdFrame(fbRearUpright, rctx);
  writeRlcdPgm(outDir + "/rlcd-rear-upright.pgm", fbRearUpright);
  rctx.liveMonoPlane = liveMono.data();

  // 4. Gallery view (600x450 saved photo, never rerotated)
  ui.mode = AppMode::GALLERY;
  ui.stillIndex = 0;
  MediaEntry still{};
  still.id = 3;
  still.type = static_cast<uint8_t>(MediaType::STILL);
  still.look = static_cast<uint8_t>(LookMode::DITHER);
  still.contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
  still.mirror = 0;
  still.synthetic = 1;
  still.width = 600;
  still.height = 450;
  still.sizeBytes = 17100;  // ~17 KB matching tested scene
  std::snprintf(still.title, sizeof(still.title), "IMG_0003");
  rctx.currentStill = &still;
  rctx.galleryGray = galleryGray.data();
  rctx.galleryW = 600;
  rctx.galleryH = 450;
  renderRlcdFrame(fbGallery, rctx);
  writeRlcdPgm(outDir + "/rlcd-gallery.pgm", fbGallery);

  // 5. Mini OLED Selfie vs Rear (preview y=11..63 identical; status label changes LIVE -> REAR)
  OledFrameBuffer oledSelfie, oledRear;
  OledRenderContext octx{};
  octx.previewGray = rawSensor.data();
  octx.previewW = 320;
  octx.previewH = 240;
  octx.linkConnected = true;
  octx.cameraOk = true;
  octx.freeMiB = 3.9f;
  octx.look = static_cast<uint8_t>(LookMode::DITHER);
  octx.contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
  octx.mirror = false;
  octx.orientation = static_cast<uint8_t>(OrientationMode::SELFIE);
  renderOledFrame(oledSelfie, octx);
  writeOledPgm(outDir + "/oled-selfie-live.pgm", oledSelfie);

  octx.orientation = static_cast<uint8_t>(OrientationMode::REAR);
  renderOledFrame(oledRear, octx);
  writeOledPgm(outDir + "/oled-rear-live.pgm", oledRear);

  // Verify OLED preview rows (y=11..63) are bit-exact between SELFIE and REAR
  for (int y = 11; y < OledFrameBuffer::kHeight; ++y) {
    for (int x = 0; x < OledFrameBuffer::kWidth; ++x) {
      assert(oledSelfie.getPixel(x, y) == oledRear.getPixel(x, y));
    }
  }
  assert(oledSelfie.crc32() != oledRear.crc32());  // Header text changed LIVE -> REAR

  std::printf(
      "{"
      "\"selfie_crc32\":\"%08x\","
      "\"hold_crc32\":\"%08x\","
      "\"rear_crc32\":\"%08x\","
      "\"gallery_crc32\":\"%08x\","
      "\"oled_selfie_crc32\":\"%08x\","
      "\"oled_rear_crc32\":\"%08x\""
      "}\n",
      fbSelfie.crc32(), fbHold.crc32(), fbRear.crc32(),
      fbGallery.crc32(), oledSelfie.crc32(), oledRear.crc32());
  return 0;
}
