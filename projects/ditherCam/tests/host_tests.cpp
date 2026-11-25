// SPDX-License-Identifier: MIT
// Adversarial + functional host C++ test suite for shared FlipCam logic.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "../shared/FlipCamButtons.h"
#include "../shared/FlipCamCrc.h"
#include "../shared/FlipCamDisplay.h"
#include "../shared/FlipCamDither.h"
#include "../shared/FlipCamFont5x7.h"
#include "../shared/FlipCamJpegDecode.h"
#include "../shared/FlipCamPairConfig.h"
#include "../shared/FlipCamProtocol.h"
#include "../shared/FlipCamStorage.h"
#include "../shared/FlipCamStream.h"
#include "../shared/FlipCamTypes.h"
#include "../shared/FlipCamUiState.h"
#include "../shared/FlipCamUsbProtocol.h"

using namespace flipcam;

namespace {

// In-memory mock storage backend with failure injection hooks
class MockStorageBackend : public IStorageBackend {
 public:
  bool failMount = false;
  bool mounted = false;
  uint64_t capacity = 4u * 1024u * 1024u;
  int formatCallCount = 0;
  std::map<std::string, std::vector<uint8_t>> files;

  bool mountNoFormat() override {
    if (failMount) {
      mounted = false;
      return false;
    }
    mounted = true;
    return true;
  }

  bool formatPartition(const char* exactLabel) override {
    formatCallCount++;
    if (!exactLabel || std::strcmp(exactLabel, "media") != 0) return false;
    files.clear();
    mounted = true;
    return true;
  }

  bool isMounted() const override { return mounted; }
  uint64_t totalBytes() const override { return capacity; }
  uint64_t usedBytes() const override {
    uint64_t u = 0;
    for (const auto& kv : files) u += kv.second.size();
    return u;
  }
  bool exists(const char* path) const override {
    return mounted && files.count(path) > 0;
  }
  bool fileSize(const char* path, size_t& outSize) const override {
    auto it = files.find(path);
    if (!mounted || it == files.end()) return false;
    outSize = it->second.size();
    return true;
  }
  bool removeFile(const char* path) override {
    if (!mounted) return false;
    return files.erase(path) > 0;
  }
  bool renameFile(const char* fromPath, const char* toPath) override {
    auto it = files.find(fromPath);
    if (!mounted || it == files.end()) return false;
    files[toPath] = std::move(it->second);
    files.erase(it);
    return true;
  }
  bool writeAll(const char* path, const uint8_t* data, size_t len) override {
    if (!mounted) return false;
    if (usedBytes() + len > capacity) return false;
    files[path] = std::vector<uint8_t>(data, data + len);
    return true;
  }
  bool readAll(const char* path, std::vector<uint8_t>& out, size_t maxBytes) const override {
    auto it = files.find(path);
    if (!mounted || it == files.end() || it->second.size() > maxBytes) return false;
    out = it->second;
    return true;
  }
  bool readSlice(const char* path, size_t offset, uint8_t* out, size_t len) const override {
    auto it = files.find(path);
    if (!mounted || it == files.end() || offset + len > it->second.size()) return false;
    std::memcpy(out, it->second.data() + offset, len);
    return true;
  }
  bool listDir(const char* dirPath, std::vector<std::string>& outNames) const override {
    outNames.clear();
    if (!mounted) return false;
    const std::string prefix = std::string(dirPath) + "/";
    for (const auto& kv : files) {
      if (kv.first.rfind(prefix, 0) == 0) {
        outNames.push_back(kv.first.substr(prefix.size()));
      }
    }
    return true;
  }
};

std::vector<uint8_t> makeValidJpeg(size_t size = 1024, uint8_t fill = 0x42) {
  std::vector<uint8_t> jpg(size, fill);
  jpg[0] = 0xFF;
  jpg[1] = 0xD8;
  jpg[size - 2] = 0xFF;
  jpg[size - 1] = 0xD9;
  return jpg;
}

std::vector<uint8_t> makeValidFcv(uint32_t frames, uint32_t audioBytesPerFrame, uint32_t durMs) {
  std::vector<uint8_t> out(sizeof(FcvFileHeader), 0);
  uint32_t totalAudio = 0;
  for (uint32_t i = 0; i < frames; ++i) {
    auto jpg = makeValidJpeg(256, static_cast<uint8_t>(0x10 + i));
    FcvChunkHeader ch{};
    ch.chunkMagic = kChunkFrame;
    ch.timestampMs = (i * durMs) / std::max<uint32_t>(1u, frames);
    ch.payloadLen = static_cast<uint32_t>(jpg.size());
    ch.payloadCrc32 = crc32Compute(jpg.data(), jpg.size());
    const size_t off = out.size();
    out.resize(off + sizeof(ch) + jpg.size());
    std::memcpy(out.data() + off, &ch, sizeof(ch));
    std::memcpy(out.data() + off + sizeof(ch), jpg.data(), jpg.size());

    if (audioBytesPerFrame > 0) {
      std::vector<uint8_t> pcm(audioBytesPerFrame, static_cast<uint8_t>(0x20 + i));
      FcvChunkHeader ach{};
      ach.chunkMagic = kChunkAudio;
      ach.timestampMs = ch.timestampMs;
      ach.payloadLen = static_cast<uint32_t>(pcm.size());
      ach.payloadCrc32 = crc32Compute(pcm.data(), pcm.size());
      const size_t aoff = out.size();
      out.resize(aoff + sizeof(ach) + pcm.size());
      std::memcpy(out.data() + aoff, &ach, sizeof(ach));
      std::memcpy(out.data() + aoff + sizeof(ach), pcm.data(), pcm.size());
      totalAudio += ach.payloadLen;
    }
  }
  FcvFileHeader hdr{};
  hdr.sampleRate = 16000;
  hdr.channels = 1;
  hdr.bitsPerSample = 16;
  hdr.durationMs = durMs;
  hdr.frameCount = frames;
  hdr.audioBytes = totalAudio;
  hdr.flags = totalAudio > 0 ? kFcvFlagHasAudio : 0u;
  finalizeFcvHeader(hdr);
  std::memcpy(out.data(), &hdr, sizeof(hdr));
  return out;
}

void testButtonsAndSteering() {
  ButtonController bc;
  // Boot-held SNAP must be suppressed until released
  bc.init(true, false, 0);
  auto ev = bc.update(true, false, 900);
  assert(!ev.snapHold && !ev.snapTap);
  assert(bc.snap().bootSuppressed);

  // Release boot-held SNAP cleanly
  ev = bc.update(false, false, 910);
  ev = bc.update(false, false, 950);
  assert(!ev.snapTap && !bc.snap().bootSuppressed);

  // Bounce suppression (<25ms)
  ev = bc.update(true, false, 1000);
  ev = bc.update(false, false, 1010);
  ev = bc.update(false, false, 1050);
  assert(!ev.snapTap && bc.snap().bounceSuppressedCount >= 1);

  // Long press SNAP has NO hold event, holdProgress == 0, and releasing fires ONE tap (never loop/autorepeat)
  ev = bc.update(true, false, 1100);
  ev = bc.update(true, false, 1130);  // debounced down
  assert(bc.snap().active && bc.snap().holdProgress == 0.0f);
  ev = bc.update(true, false, 1930);  // held >= 800ms -> NO hold event, no autorepeat
  assert(!ev.snapHold && !ev.snapTap && bc.snap().holdProgress == 0.0f);
  ev = bc.update(true, false, 2200);
  assert(!ev.snapHold && !ev.snapTap);
  ev = bc.update(false, false, 2210);
  ev = bc.update(false, false, 2240);  // release yields one deliberate tap
  assert(ev.snapTap && !ev.snapHold);
  assert(bc.snap().physPressCount == 1 && bc.snap().physHoldCount == 0 && bc.snap().physTapCount == 1);

  // Stuck input (>15s) sets stuck flag and suppresses tap
  ev = bc.update(true, false, 3000);
  ev = bc.update(true, false, 3030);
  ev = bc.update(true, false, 19000);
  assert(bc.snap().stuck);
  ev = bc.update(false, false, 19050);
  ev = bc.update(false, false, 19090);
  assert(!ev.snapTap);

  // Browse button: NO hold event, holdProgress always 0, tap fires once on release
  ev = bc.update(false, true, 20000);
  ev = bc.update(false, true, 20030);
  assert(bc.browse().active && bc.browse().holdProgress == 0.0f);
  ev = bc.update(false, true, 21000);  // held 1s
  assert(!ev.browseHold && bc.browse().holdProgress == 0.0f);
  ev = bc.update(false, false, 21010);
  ev = bc.update(false, false, 21040);
  assert(ev.browseTap);

  // Simultaneous chord: both active, releasing browse doesn't trigger snap
  ev = bc.update(true, true, 22000);
  ev = bc.update(true, true, 22030);
  assert(bc.snap().active && bc.browse().active);
  ev = bc.update(true, false, 22050);
  ev = bc.update(true, false, 22080);
  assert(ev.browseTap && !ev.snapTap && bc.snap().active);
  ev = bc.update(false, false, 22100);
  ev = bc.update(false, false, 22130);
  assert(ev.snapTap);

  // Injection gated by TESTMODE and tracked separately from physical counters
  assert(!bc.injectDown(ButtonId::SNAP, 23000));
  bc.setTestMode(true, 23000);
  assert(bc.injectDown(ButtonId::SNAP, 23010));
  assert(bc.injectUp(ButtonId::SNAP, 23050, ev));
  assert(ev.snapTap);
  assert(bc.snap().injTapCount == 1);
  assert(bc.snap().physTapCount == 2);  // physical taps unchanged by inject!

  // RLCD board config: SNAP hold (~1s) enabled, BROWSE hold disabled
  ButtonController bcRlcd;
  bcRlcd.init(false, false, 0, true, false);
  ev = bcRlcd.update(true, false, 1000);
  ev = bcRlcd.update(true, false, 1030);  // debounced press start at 1030
  assert(bcRlcd.snap().active && bcRlcd.snap().holdProgress == 0.0f);

  // Mid-hold (500ms): progress ~0.5, neither hold nor tap fired yet
  ev = bcRlcd.update(true, false, 1530);
  assert(!ev.snapHold && !ev.snapTap);
  assert(bcRlcd.snap().holdProgress > 0.45f && bcRlcd.snap().holdProgress < 0.55f);

  // Cross hold threshold (1000ms -> 2030ms): snapHold fires ONCE
  ev = bcRlcd.update(true, false, 2030);
  assert(ev.snapHold && !ev.snapTap && bcRlcd.snap().holdProgress == 1.0f);

  // Continuing to hold must NOT re-fire snapHold
  ev = bcRlcd.update(true, false, 2500);
  assert(!ev.snapHold && !ev.snapTap);

  // Releasing after hold NEVER triggers capture tap!
  ev = bcRlcd.update(false, false, 2510);
  ev = bcRlcd.update(false, false, 2540);
  assert(!ev.snapTap && !ev.snapHold);
  assert(bcRlcd.snap().physHoldCount == 1 && bcRlcd.snap().physTapCount == 0);

  // Short tap (<1s) fires tap and NOT hold
  ev = bcRlcd.update(true, false, 3000);
  ev = bcRlcd.update(true, false, 3030);
  ev = bcRlcd.update(true, false, 3200);
  assert(!ev.snapHold && bcRlcd.snap().holdProgress > 0.0f && bcRlcd.snap().holdProgress < 0.3f);
  ev = bcRlcd.update(false, false, 3210);
  ev = bcRlcd.update(false, false, 3240);
  assert(ev.snapTap && !ev.snapHold);
  assert(bcRlcd.snap().physTapCount == 1 && bcRlcd.snap().physHoldCount == 1);

  // Browse hold remains strictly disabled even on RLCD config
  ev = bcRlcd.update(false, true, 4000);
  ev = bcRlcd.update(false, true, 4030);
  ev = bcRlcd.update(false, true, 5200);
  assert(!ev.browseHold && bcRlcd.browse().holdProgress == 0.0f);
  ev = bcRlcd.update(false, false, 5210);
  ev = bcRlcd.update(false, false, 5240);
  assert(ev.browseTap);

  // Injected hold semantics match physical hold: fires snapHold once, release never captures
  bcRlcd.setTestMode(true, 6000);
  assert(bcRlcd.injectDown(ButtonId::SNAP, 6000));
  ev = bcRlcd.update(false, false, 6500);
  assert(!ev.snapHold && bcRlcd.snap().holdProgress > 0.45f);
  ev = bcRlcd.update(false, false, 7050);
  assert(ev.snapHold && !ev.snapTap);
  ButtonEvents upEv{};
  assert(bcRlcd.injectUp(ButtonId::SNAP, 7060, upEv));
  assert(!upEv.snapTap && !upEv.snapHold);
  assert(bcRlcd.snap().injHoldCount == 1 && bcRlcd.snap().injTapCount == 0);

  // Direct injectDown + injectUp past kHoldMs (without intermediate update) also fires hold & suppresses tap
  upEv = ButtonEvents{};
  assert(bcRlcd.injectDown(ButtonId::SNAP, 8000));
  assert(bcRlcd.injectUp(ButtonId::SNAP, 9100, upEv));
  assert(upEv.snapHold && !upEv.snapTap);
  assert(bcRlcd.snap().injHoldCount == 2 && bcRlcd.snap().injTapCount == 0);

  // Cancel & TESTMODE OFF suppress active injected hold safely
  assert(bcRlcd.injectDown(ButtonId::SNAP, 10000));
  assert(bcRlcd.injectCancel(ButtonId::SNAP));
  assert(!bcRlcd.snap().active && bcRlcd.snap().holdProgress == 0.0f);
  assert(bcRlcd.injectDown(ButtonId::SNAP, 10100));
  bcRlcd.setTestMode(false, 10200);
  assert(!bcRlcd.snap().active && bcRlcd.snap().holdProgress == 0.0f);
}

void testReverseChronologicalBrowseUi() {
  FlipCamUiState ui;
  ui.storageMounted = true;
  ui.linkState = LinkState::CONNECTED;
  ui.setStillCount(0);

  // Verify LIVE context hints (SNAP tap=STILL, hold=FLIP, BROWSE=PREV)
  auto hints = ui.contextHints();
  assert(std::strcmp(hints.snapTap, "STILL") == 0);
  assert(std::strcmp(hints.snapHold, "FLIP") == 0);
  assert(std::strcmp(hints.browseTap, "PREV") == 0);

  // Hold SNAP in LIVE toggles orientation once (selfie -> rear -> selfie), never captures!
  assert(ui.settings.orientation == static_cast<uint8_t>(OrientationMode::SELFIE));
  ButtonEvents ev{};
  ev.snapHold = true;
  auto cmd = ui.applyButtonEvents(ev, 50);
  assert(cmd == UiCommand::NONE);
  assert(ui.settings.orientation == static_cast<uint8_t>(OrientationMode::REAR));
  assert(ui.settingsDirty);
  assert(std::strcmp(ui.toast, "MODE: REAR 180") == 0);

  // Busy hold in LIVE is ignored safely with toast and does NOT flip orientation
  ui.settingsDirty = false;
  ui.captureBusy = true;
  cmd = ui.applyButtonEvents(ev, 75);
  assert(cmd == UiCommand::NONE);
  assert(ui.settings.orientation == static_cast<uint8_t>(OrientationMode::REAR));
  assert(!ui.settingsDirty);
  assert(std::strcmp(ui.toast, "BUSY: WAIT") == 0);
  ui.captureBusy = false;

  // Hold again toggles back to SELFIE
  cmd = ui.applyButtonEvents(ev, 90);
  assert(cmd == UiCommand::NONE);
  assert(ui.settings.orientation == static_cast<uint8_t>(OrientationMode::SELFIE));
  assert(std::strcmp(ui.toast, "MODE: SELFIE") == 0);

  // Browse when 0 stills stays in LIVE
  ev = ButtonEvents{};
  ev.browseTap = true;
  cmd = ui.applyButtonEvents(ev, 100);
  assert(ui.mode == AppMode::LIVE && cmd == UiCommand::NONE);

  // With 3 stills (0=newest, 2=oldest), Browse steps 0 -> 1 -> 2 -> stays at 2 (NO wraparound!)
  ui.setStillCount(3);
  ev = ButtonEvents{};
  ev.browseTap = true;
  ui.applyButtonEvents(ev, 200);
  assert(ui.mode == AppMode::GALLERY && ui.stillIndex == 0);
  assert(std::strcmp(ui.contextHints().snapTap, "LIVE") == 0);
  assert(std::strcmp(ui.contextHints().snapHold, "") == 0);

  ui.applyButtonEvents(ev, 300);
  assert(ui.mode == AppMode::GALLERY && ui.stillIndex == 1);

  ui.applyButtonEvents(ev, 400);
  assert(ui.mode == AppMode::GALLERY && ui.stillIndex == 2);

  ui.applyButtonEvents(ev, 500);
  assert(ui.mode == AppMode::GALLERY && ui.stillIndex == 2);
  assert(std::strcmp(ui.toast, "OLDEST") == 0);

  // Browse hold in GALLERY does NOTHING (no settings/menu)
  ev = ButtonEvents{};
  ev.browseHold = true;
  ui.applyButtonEvents(ev, 600);
  assert(ui.mode == AppMode::GALLERY && ui.stillIndex == 2);

  // Snap hold in GALLERY is a deliberate no-op (does NOT flip orientation or rotate old photos)
  const uint8_t orientBeforeGalHold = ui.settings.orientation;
  ev = ButtonEvents{};
  ev.snapHold = true;
  cmd = ui.applyButtonEvents(ev, 700);
  assert(cmd == UiCommand::NONE && !ui.recording && ui.mode == AppMode::GALLERY);
  assert(ui.settings.orientation == orientBeforeGalHold);

  // Snap tap in GALLERY returns to LIVE on first tap without capturing
  ev = ButtonEvents{};
  ev.snapTap = true;
  cmd = ui.applyButtonEvents(ev, 800);
  assert(ui.mode == AppMode::LIVE && cmd == UiCommand::NONE);

  // Second Snap tap in LIVE captures
  cmd = ui.applyButtonEvents(ev, 900);
  assert(cmd == UiCommand::CAPTURE_STILL);

  // Link lost blocks capture cleanly
  ui.linkState = LinkState::DISCONNECTED;
  cmd = ui.applyButtonEvents(ev, 1000);
  assert(cmd == UiCommand::NONE);
  // Browse still works while link is lost!
  ev = ButtonEvents{};
  ev.browseTap = true;
  ui.applyButtonEvents(ev, 1100);
  assert(ui.mode == AppMode::GALLERY);
}

void testProtocolAndFailureCases() {
  // Valid FCV1
  auto fcv = makeValidFcv(3, 320, 1500);
  auto sum = validateFcvBuffer(fcv.data(), fcv.size());
  assert(sum.valid && sum.frames == 3 && sum.audioChunks == 3 && sum.audioBytes == 960 && sum.hasAudio);

  // Corrupt payload byte -> CRC mismatch
  auto badCrc = fcv;
  badCrc[sizeof(FcvFileHeader) + sizeof(FcvChunkHeader) + 10] ^= 0xFF;
  assert(!validateFcvBuffer(badCrc.data(), badCrc.size()).valid);

  // Corrupt header CRC
  auto badHdr = fcv;
  badHdr[8] ^= 0x01;
  assert(!validateFcvBuffer(badHdr.data(), badHdr.size()).valid);

  // Truncated clip
  auto trunc = fcv;
  trunc.resize(trunc.size() - 20);
  assert(!validateFcvBuffer(trunc.data(), trunc.size()).valid);

  // Oversize clip
  assert(!validateFcvBuffer(fcv.data(), kMaxClipFileBytes + 1).valid);

  // Oversize chunk header
  FcvChunkHeader badCh{};
  badCh.chunkMagic = kChunkFrame;
  badCh.payloadLen = kMaxFrameChunkBytes + 1;
  assert(!validateFcvChunkHeader(badCh, kMaxClipFileBytes));

  // WAV header check
  uint8_t wav[44];
  buildWavHeader(wav, 16000, 1, 16, 960);
  assert(std::memcmp(wav, "RIFF", 4) == 0 && std::memcmp(wav + 8, "WAVE", 4) == 0);

  // Safe ID & path traversal checks
  char path[64];
  assert(buildSafeMediaPath("/media", "S_000001.jpg", path, sizeof(path)));
  assert(!buildSafeMediaPath("/media", "../etc/passwd", path, sizeof(path)));
  assert(!buildSafeMediaPath("/media", "a/b.jpg", path, sizeof(path)));
  assert(!buildSafeMediaPath("/media", ".hidden", path, sizeof(path)));
  uint32_t id = 0;
  assert(parsePositiveId("42", id) && id == 42);
  assert(!parsePositiveId("0", id));
  assert(!parsePositiveId("-1", id));
  assert(!parsePositiveId("12/../3", id));
}

void testStorageAndRecovery() {
  MockStorageBackend fs;
  MediaStore store(fs);

  // 1. Mount failure must NEVER auto-format!
  fs.failMount = true;
  assert(!store.beginNoAutoFormat());
  assert(!store.status().mounted);
  assert(fs.formatCallCount == 0);
  assert(std::strcmp(store.status().fault, "MOUNT_FAILED_NO_AUTOFORMAT") == 0);

  // 2. Guarded format rejects wrong label and busy state
  fs.failMount = false;
  assert(store.beginNoAutoFormat());
  assert(!store.guardedFormat("MEDIA", false));
  assert(!store.guardedFormat("spiffs", false));
  assert(!store.guardedFormat("media", true));
  assert(store.guardedFormat("media", false));

  // 3. Save stills + clip; ensure still-only browse skips clips!
  MediaEntry s1{}, s2{}, c1{};
  auto jpg1 = makeValidJpeg(4096, 0x11);
  auto jpg2 = makeValidJpeg(4096, 0x22);
  assert(store.saveStill(jpg1.data(), jpg1.size(), 800, 600, 0, 1, 0, false, s1));
  auto fcv = makeValidFcv(2, 160, 1000);
  assert(fs.writeAll("/media/tmp_clip.part", fcv.data(), fcv.size()));
  assert(store.finalizeClipTempFile("tmp_clip.part", 640, 480, 0, 1, 0, false, c1));
  assert(store.saveStill(jpg2.data(), jpg2.size(), 800, 600, 0, 1, 0, true, s2));

  assert(store.entries().size() == 3);
  assert(store.stillCount() == 2);
  assert(store.getStillNewestFirst(0)->id == s2.id);  // newest still first
  assert(store.getStillNewestFirst(1)->id == s1.id);  // older still second
  assert(store.getStillNewestFirst(2) == nullptr);    // clip is NOT in still browse!
  assert(store.prodCaptures() == 2 && store.synthCaptures() == 1);

  // 4. Interrupted write recovery + corrupt file rejection on reboot
  fs.files["/media/tmp_999.part"] = {1, 2, 3};
  fs.files["/media/orphan.tmp"] = {4, 5, 6};
  // Corrupt s1's JPEG EOI marker on disk
  fs.files[std::string("/media/") + s1.filename].back() = 0x00;

  MediaStore rebootStore(fs);
  assert(rebootStore.beginNoAutoFormat());
  assert(rebootStore.status().recoveredTempDeleted == 2);
  assert(rebootStore.status().rejectedCorruptCount == 1);
  assert(rebootStore.entries().size() == 2);  // c1 and s2 preserved!

  // 5. Storage full must reject capture & NEVER auto-delete existing items!
  fs.capacity = fs.usedBytes() + 20000;  // less than reserve (64KB)
  MediaEntry s3{};
  assert(!rebootStore.saveStill(jpg1.data(), jpg1.size(), 800, 600, 0, 1, 0, false, s3));
  assert(std::strcmp(rebootStore.status().fault, "STORAGE_FULL") == 0);
  assert(rebootStore.entries().size() == 2);  // Nothing auto-deleted!

  // 6. Settings persist only when changed + old-blob backward compatibility + orientation round-trip
  FlipCamSettings st{};
  assert(rebootStore.saveSettingsIfChanged(st));
  auto settingsCopy = fs.files["/media/settings.bin"];
  assert(rebootStore.saveSettingsIfChanged(st));
  assert(fs.files["/media/settings.bin"] == settingsCopy);

  // Old 16-byte settings blob (magic='FCS1', version=1, reserved={0,0}) must load with orientation=SELFIE(0)
  uint8_t oldBlob[16]{};
  const uint32_t magic = 0x46435331u;
  const uint16_t ver = 1;
  std::memcpy(oldBlob + 0, &magic, 4);
  std::memcpy(oldBlob + 4, &ver, 2);
  oldBlob[6] = 1;  // look = THRESHOLD
  oldBlob[7] = 0;  // contrast = C_LOW
  oldBlob[8] = 1;  // mirror = 1
  oldBlob[9] = 0;  // sound = 0
  oldBlob[10] = 0; // old reserved[0] = 0
  oldBlob[11] = 0; // old reserved[1] = 0
  const uint32_t oldCrc = crc32Compute(oldBlob, 12);
  std::memcpy(oldBlob + 12, &oldCrc, 4);
  fs.files["/media/settings.bin"].assign(oldBlob, oldBlob + 16);
  FlipCamSettings loadedOld{};
  assert(rebootStore.loadSettings(loadedOld));
  assert(loadedOld.look == 1 && loadedOld.contrast == 0 && loadedOld.mirror == 1 && loadedOld.sound == 0);
  assert(loadedOld.orientation == static_cast<uint8_t>(OrientationMode::SELFIE));

  // New orientation=REAR(1) persists & reloads cleanly
  loadedOld.orientation = static_cast<uint8_t>(OrientationMode::REAR);
  assert(rebootStore.saveSettingsIfChanged(loadedOld));
  FlipCamSettings loadedRear{};
  assert(rebootStore.loadSettings(loadedRear));
  assert(loadedRear.orientation == static_cast<uint8_t>(OrientationMode::REAR));
  assert(rebootStore.saveSettingsIfChanged(st));  // Restore default for subsequent size/capacity checks

  // 7. Power-cut between final rename and index commit (orphan final files + index.tmp)
  auto orphanStill = makeValidJpeg(2048, 0x77);
  auto orphanClip = makeValidFcv(2, 160, 800);
  fs.files["/media/S_000010.jpg"] = orphanStill;
  fs.files["/media/C_000011.fcv"] = orphanClip;
  fs.files["/media/index.tmp"] = {0xDE, 0xAD, 0xBE, 0xEF};
  MediaStore powerCutStore(fs);
  assert(powerCutStore.beginNoAutoFormat());
  assert(powerCutStore.status().recoveredTempDeleted == 1);
  assert(powerCutStore.findById(10) != nullptr);
  assert(powerCutStore.findById(11) != nullptr);
  assert(powerCutStore.nextId() == 12);
  assert(fs.files["/media/S_000010.jpg"] == orphanStill);

  // 8. Missing index.bin with existing final media -> zero ID reuse or overwrite
  fs.capacity = 4u * 1024u * 1024u;
  fs.files.erase("/media/index.bin");
  MediaStore missingIdxStore(fs);
  assert(missingIdxStore.beginNoAutoFormat());
  assert(missingIdxStore.findById(10) != nullptr);
  assert(missingIdxStore.findById(11) != nullptr);
  assert(missingIdxStore.nextId() >= 12);
  MediaEntry freshAfterRebuild{};
  assert(missingIdxStore.saveStill(jpg1.data(), jpg1.size(), 800, 600, 0, 1, 0, false, freshAfterRebuild));
  assert(freshAfterRebuild.id >= 12);
  assert(fs.files["/media/S_000010.jpg"] == orphanStill);  // Old bytes untouched!

  // 9. Corrupt index.bin with existing final media -> recovers all valid files & preserves nextId
  fs.files["/media/index.bin"][0] ^= 0xFF;
  MediaStore corruptIdxStore(fs);
  assert(corruptIdxStore.beginNoAutoFormat());
  assert(corruptIdxStore.findById(10) != nullptr);
  assert(corruptIdxStore.findById(freshAfterRebuild.id) != nullptr);
  assert(corruptIdxStore.nextId() == freshAfterRebuild.id + 1);

  // 10. Bounded USB READ chunk export
  std::vector<uint8_t> chunk;
  size_t totalSz = 0;
  assert(corruptIdxStore.readMediaChunk(10, 0, 512, chunk, totalSz));
  assert(totalSz == orphanStill.size() && chunk.size() == 512);
  assert(std::memcmp(chunk.data(), orphanStill.data(), 512) == 0);
  assert(!corruptIdxStore.readMediaChunk(10, 0, 5000, chunk, totalSz));              // >4096 rejected
  assert(!corruptIdxStore.readMediaChunk(10, orphanStill.size(), 64, chunk, totalSz)); // OOB offset rejected
  assert(!corruptIdxStore.readMediaChunk(999, 0, 64, chunk, totalSz));             // missing ID rejected

  // 11. Corrupt FCV chunk payload CRC with valid 32B header rejected on boot (with or without index)
  MockStorageBackend clipFs;
  MediaStore clipStore(clipFs);
  assert(clipStore.beginNoAutoFormat());
  auto badChunkClip = makeValidFcv(2, 160, 800);
  badChunkClip[sizeof(FcvFileHeader) + sizeof(FcvChunkHeader) + 5] ^= 0xFF;  // corrupt frame CRC
  clipFs.files["/media/C_000020.fcv"] = badChunkClip;
  MediaStore clipReboot(clipFs);
  assert(clipReboot.beginNoAutoFormat());
  assert(clipReboot.status().rejectedCorruptCount >= 1);
  assert(clipReboot.findById(20) == nullptr);

  // 12. Synthetic capture counter must NOT double across reboots when prodCaptures == 0
  MockStorageBackend synthFs;
  MediaStore synthStore(synthFs);
  assert(synthStore.beginNoAutoFormat());
  MediaEntry synthItem{};
  assert(synthStore.saveStill(jpg1.data(), jpg1.size(), 800, 600, 0, 1, 0, true, synthItem));
  assert(synthStore.prodCaptures() == 0 && synthStore.synthCaptures() == 1);
  MediaStore synthReboot1(synthFs);
  assert(synthReboot1.beginNoAutoFormat());
  assert(synthReboot1.prodCaptures() == 0 && synthReboot1.synthCaptures() == 1);
  MediaStore synthReboot2(synthFs);
  assert(synthReboot2.beginNoAutoFormat());
  assert(synthReboot2.prodCaptures() == 0 && synthReboot2.synthCaptures() == 1);
}

void testRealButtonToCapturePipeline() {
  // Simulates loop() single-lock non-reentrant dispatch from physical GPIO transitions
  ButtonController bc;
  FlipCamUiState ui;
  bc.init(false, false, 0);
  ui.storageMounted = true;
  ui.linkState = LinkState::CONNECTED;
  ui.setStillCount(0);

  uint32_t queuedReqId = 0;
  bool mutexHeld = false;
  auto dispatchLocked = [&](UiCommand cmd) {
    assert(mutexHeld);  // Must NOT re-acquire mutex!
    if (cmd == UiCommand::CAPTURE_STILL && ui.canStartCapture() && queuedReqId == 0) {
      ui.captureBusy = true;
      queuedReqId = 1;
    }
  };

  mutexHeld = true;
  // Physical press down + debounce + release -> tap -> capture queued!
  auto ev = bc.update(true, false, 100);
  dispatchLocked(ui.applyButtonEvents(ev, 100));
  ev = bc.update(true, false, 130);
  dispatchLocked(ui.applyButtonEvents(ev, 130));
  assert(bc.snap().active && queuedReqId == 0);
  ev = bc.update(false, false, 180);
  dispatchLocked(ui.applyButtonEvents(ev, 180));
  ev = bc.update(false, false, 210);
  dispatchLocked(ui.applyButtonEvents(ev, 210));
  mutexHeld = false;

  assert(ev.snapTap);
  assert(queuedReqId == 1 && ui.captureBusy);
  assert(bc.snap().physTapCount == 1 && bc.snap().injTapCount == 0);
}

void testDisplayAndBatteryIcons() {
  ButtonController bc;
  bc.init(false, false, 0);
  FlipCamUiState ui;
  ui.storageMounted = true;
  ui.linkState = LinkState::CONNECTED;

  RlcdFrameBuffer rlcd;
  std::vector<uint8_t> gray(160 * 120, 128);
  generateSyntheticGray(gray.data(), 160, 120, 1);

  RlcdRenderContext rctx{};
  rctx.ui = &ui;
  rctx.buttons = &bc;
  rctx.previewGray = gray.data();
  rctx.previewW = 160;
  rctx.previewH = 120;
  rctx.freeMiB = 6.5f;
  rctx.batteryPresent = false;
  rctx.batteryPct = -1;
  renderRlcdFrame(rlcd, rctx);
  const uint32_t crcUnknownBat = rlcd.crc32();

  rctx.batteryPresent = true;
  rctx.batteryPct = 85;
  renderRlcdFrame(rlcd, rctx);
  assert(rlcd.crc32() != crcUnknownBat);  // Battery icon reflects measured state!

  OledFrameBuffer oled;
  OledRenderContext octx{};
  octx.previewGray = gray.data();
  octx.previewW = 160;
  octx.previewH = 120;
  octx.linkConnected = true;
  octx.cameraOk = true;
  octx.freeMiB = 3.2f;
  renderOledFrame(oled, octx);
  // Check top-right battery outline pixel at (110, 1) is lit
  assert(oled.getPixel(110, 1));
}

void testUsbParser() {
  assert(parseUsbCommand("STATUS").kind == UsbCmdKind::STATUS);
  assert(parseUsbCommand("CAPTURE SYNTH").kind == UsbCmdKind::CAPTURE_SYNTH);
  assert(parseUsbCommand("FORMAT MEDIA media").kind == UsbCmdKind::FORMAT_MEDIA);
  assert(std::strcmp(parseUsbCommand("FORMAT MEDIA media").arg1, "media") == 0);
  assert(parseUsbCommand("TONE 880 250").kind == UsbCmdKind::TONE);
  assert(parseUsbCommand("TONE 20 250").kind == UsbCmdKind::INVALID);
  assert(parseUsbCommand("DELETE 0").kind == UsbCmdKind::INVALID);
  assert(parseUsbCommand("DELETE ../x").kind == UsbCmdKind::INVALID);
  assert(parseUsbCommand("READ 2 0 1024").kind == UsbCmdKind::READ_MEDIA);
  assert(parseUsbCommand("READ 2 0 5000").kind == UsbCmdKind::INVALID);
  assert(parseUsbCommand("READ 0 0 128").kind == UsbCmdKind::INVALID);
  auto setOrient = parseUsbCommand("SET orientation rear");
  assert(setOrient.kind == UsbCmdKind::SET_SETTING &&
         std::strcmp(setOrient.arg1, "orientation") == 0 &&
         std::strcmp(setOrient.arg2, "rear") == 0);
  assert(parseUsbCommand("GET SETTINGS").kind == UsbCmdKind::GET_SETTINGS);
  assert(parseUsbCommand("BOGUS").kind == UsbCmdKind::INVALID);

  FlipCamUiState uiSet;
  assert(uiSet.updateSettingsField("orientation", "rear"));
  assert(uiSet.settings.orientation == static_cast<uint8_t>(OrientationMode::REAR));
  assert(uiSet.updateSettingsField("orientation", "selfie"));
  assert(uiSet.settings.orientation == static_cast<uint8_t>(OrientationMode::SELFIE));
  assert(!uiSet.updateSettingsField("orientation", "sideways"));
}

void testFastMonoProtocolAndOrientation() {
  // 1. Default settings: DITHER + HIGH contrast (gain ~2.0)
  FlipCamSettings def{};
  assert(def.look == static_cast<uint8_t>(LookMode::DITHER));
  assert(def.contrast == static_cast<uint8_t>(ContrastLevel::C_HIGH));
  const uint8_t* lutHi = getContrastLut(static_cast<uint8_t>(ContrastLevel::C_HIGH));
  const uint8_t* lutNorm = getContrastLut(static_cast<uint8_t>(ContrastLevel::C_NORMAL));
  assert(lutNorm[192] == 192);
  assert(lutHi[192] == 255);
  assert(lutHi[63] == 0);
  assert(lutHi[160] > lutNorm[160]);
  assert(lutHi[96] < lutNorm[96]);

  // 2. Asymmetric 400x300 source: top-left quadrant bright (255), rest dark (0), plus unique corner pixel
  // After 90 deg CW: top-left (sx < 200, sy < 150) rotates to TOP-RIGHT!
  // After 90 deg CW + Rear 180: TOP-RIGHT rotates to BOTTOM-LEFT (exact 180 deg, NOT mirror!).
  std::vector<uint8_t> src400(400u * 300u, 0);
  for (int y = 0; y < 150; ++y) {
    for (int x = 0; x < 200; ++x) {
      src400[y * 400 + x] = 255;
    }
  }
  std::vector<uint8_t> rot600(600u * 450u, 0);
  rotate90CwCrop43Gray(src400.data(), 400, 300, rot600.data(), 600, 450, false, false);
  assert(rot600[10 * 600 + 500] == 255);  // Top-right bright
  assert(rot600[10 * 600 + 50] == 0);     // Top-left dark

  std::vector<uint8_t> rot600Rear(600u * 450u, 0);
  rotate90CwCrop43Gray(src400.data(), 400, 300, rot600Rear.data(), 600, 450, false, true);
  assert(rot600Rear[439 * 600 + 99] == 255);  // Bottom-left bright (180 deg from top-right!)
  assert(rot600Rear[10 * 600 + 500] == 0);    // Top-right dark
  assert(rot600Rear[10 * 600 + 50] == 0);     // Top-left dark (NOT horizontal mirror!)
  assert(rot600Rear[439 * 600 + 500] == 0);   // Bottom-right dark
  // Verify exact pixel-for-pixel 180 deg symmetry across entire 600x450 frame (including mirror=true variant)
  for (int y = 0; y < 450; ++y) {
    for (int x = 0; x < 600; ++x) {
      assert(rot600Rear[y * 600 + x] == rot600[(449 - y) * 600 + (599 - x)]);
    }
  }
  std::vector<uint8_t> rot600Mir(600u * 450u, 0), rot600MirRear(600u * 450u, 0);
  rotate90CwCrop43Gray(src400.data(), 400, 300, rot600Mir.data(), 600, 450, true, false);
  rotate90CwCrop43Gray(src400.data(), 400, 300, rot600MirRear.data(), 600, 450, true, true);
  for (int y = 0; y < 450; ++y) {
    for (int x = 0; x < 600; ++x) {
      assert(rot600MirRear[y * 600 + x] == rot600Mir[(449 - y) * 600 + (599 - x)]);
    }
  }

  // Exact 4:3 packed mono stream geometry & offsets inside RLCD 384x232 viewport
  assert(kFmbViewW == 304 && kFmbViewH == 228);
  assert(kFmbViewW * 3 == kFmbViewH * 4);
  assert(kFmbRawPlaneBytes == 8664);
  assert(kFmbDrawOffsetX == 40 && kFmbDrawOffsetY == 2);

  // Same 90CW geometry in fused 1-bit dither plane (304x228 = 8,664 bytes)
  std::vector<uint8_t> mono(kFmbRawPlaneBytes, 0);
  dither90CwCrop43ToMonoPlane(src400.data(), 400, 300, mono.data(), kFmbViewW, kFmbViewH,
                              static_cast<uint8_t>(LookMode::THRESHOLD),
                              static_cast<uint8_t>(ContrastLevel::C_HIGH), false);
  auto monoPx = [&](int x, int y) {
    return (mono[y * (kFmbViewW / 8) + (x >> 3)] & static_cast<uint8_t>(0x80u >> (x & 7))) != 0;
  };
  assert(monoPx(260, 20));   // Top-right white (1)
  assert(!monoPx(30, 20));   // Top-left ink (0)

  // Mirror flips horizontally -> top-left becomes white, top-right becomes ink
  std::vector<uint8_t> monoMir(kFmbRawPlaneBytes, 0);
  dither90CwCrop43ToMonoPlane(src400.data(), 400, 300, monoMir.data(), kFmbViewW, kFmbViewH,
                              static_cast<uint8_t>(LookMode::THRESHOLD),
                              static_cast<uint8_t>(ContrastLevel::C_HIGH), true);
  auto monoMirPx = [&](int x, int y) {
    return (monoMir[y * (kFmbViewW / 8) + (x >> 3)] & static_cast<uint8_t>(0x80u >> (x & 7))) != 0;
  };
  assert(monoMirPx(30, 20));
  assert(!monoMirPx(260, 20));

  // 2b. Fast QVGA (320x240) live path matches SVGA (800x600) 90CW + 4:3 framing identically
  std::vector<uint8_t> srcQvga(320u * 240u, 0);
  for (int y = 0; y < 120; ++y) {
    for (int x = 0; x < 160; ++x) srcQvga[y * 320 + x] = 255;
  }
  std::vector<uint8_t> monoQvga(kFmbRawPlaneBytes, 0);
  dither90CwCrop43ToMonoPlane(srcQvga.data(), 320, 240, monoQvga.data(), kFmbViewW, kFmbViewH,
                              static_cast<uint8_t>(LookMode::THRESHOLD),
                              static_cast<uint8_t>(ContrastLevel::C_HIGH), false);
  auto monoQvgaPx = [&](int x, int y) {
    return (monoQvga[y * (kFmbViewW / 8) + (x >> 3)] & static_cast<uint8_t>(0x80u >> (x & 7))) != 0;
  };
  assert(monoQvgaPx(260, 20));    // Top-right white matches 400x300/800x600 90CW
  assert(!monoQvgaPx(30, 20));    // Top-left ink
  assert(!monoQvgaPx(30, 200));   // Bottom-left ink
  assert(!monoQvgaPx(260, 200));  // Bottom-right ink

  // 3. FMB1 PackBits RLE round-trip on structured mono plane (must compress smaller than raw)
  FmbFrameHeader hdr{};
  std::vector<uint8_t> payload;
  encodeFmbFrame(mono.data(), 42, 7, 0x01, hdr, payload);
  assert(validateFmbHeader(hdr));
  assert(hdr.codec == static_cast<uint8_t>(FmbCodec::PACKBITS_RLE_1BIT));
  assert(hdr.payloadLen < kFmbRawPlaneBytes);
  std::vector<uint8_t> decoded(kFmbRawPlaneBytes, 0xAA);
  assert(decodeFmbPayload(hdr, payload.data(), payload.size(), decoded.data(), decoded.size()));
  assert(decoded == mono);

  // 4. Incompressible alternating noise -> automatic RAW_1BIT fallback with exact CRC
  std::vector<uint8_t> noisy(kFmbRawPlaneBytes, 0);
  uint32_t lfsr = 0xACE1u;
  for (size_t i = 0; i < noisy.size(); ++i) {
    lfsr ^= lfsr << 13;
    lfsr ^= lfsr >> 17;
    lfsr ^= lfsr << 5;
    noisy[i] = static_cast<uint8_t>(lfsr & 0xFFu);
  }
  FmbFrameHeader rawHdr{};
  std::vector<uint8_t> rawPayload;
  encodeFmbFrame(noisy.data(), 43, 3, 0, rawHdr, rawPayload);
  assert(validateFmbHeader(rawHdr));
  assert(rawHdr.codec == static_cast<uint8_t>(FmbCodec::RAW_1BIT));
  assert(rawHdr.payloadLen == kFmbRawPlaneBytes);
  std::vector<uint8_t> decodedRaw(kFmbRawPlaneBytes, 0);
  assert(decodeFmbPayload(rawHdr, rawPayload.data(), rawPayload.size(), decodedRaw.data(), decodedRaw.size()));
  assert(decodedRaw == noisy);

  // 5. Adversarial stream header/payload/RLE rejections
  FmbFrameHeader badMagic = hdr;
  badMagic.magic = 0x12345678u;
  assert(!validateFmbHeader(badMagic));

  FmbFrameHeader badDims = hdr;
  badDims.width = 383;
  finalizeFmbHeader(badDims);
  assert(!validateFmbHeader(badDims));

  FmbFrameHeader badCodec = hdr;
  badCodec.codec = 9;
  finalizeFmbHeader(badCodec);
  assert(!validateFmbHeader(badCodec));

  FmbFrameHeader badSize = hdr;
  badSize.payloadLen = kFmbMaxPayloadBytes + 1u;
  finalizeFmbHeader(badSize);
  assert(!validateFmbHeader(badSize));

  // Payload corruption + truncation rejected
  auto corruptPayload = payload;
  corruptPayload[corruptPayload.size() / 2] ^= 0xFFu;
  assert(!decodeFmbPayload(hdr, corruptPayload.data(), corruptPayload.size(), decoded.data(), decoded.size()));
  assert(!decodeFmbPayload(hdr, payload.data(), payload.size() - 1, decoded.data(), decoded.size()));

  // Malformed PackBits 0x80 control byte rejected directly
  const uint8_t badRle[2] = {0x80u, 0x00u};
  assert(!packBitsDecompress(badRle, sizeof(badRle), decoded.data(), kFmbRawPlaneBytes));

  // 6. Full-screen 400x300 RLCD liveMonoPlane resample (Selfie vs Rear 180), edge pixels & UI overlay order
  // First verify fast packer blitLiveMono400x300 is bit-exact with setPixel in both orientations
  for (int rot = 0; rot <= 1; ++rot) {
    const bool r180 = (rot == 1);
    RlcdFrameBuffer fbFast, fbRef;
    fbRef.clear(false);
    blitLiveMono400x300(fbFast, mono.data(), r180);
    for (int dy = 0; dy < RlcdFrameBuffer::kHeight; ++dy) {
      const int oy = r180 ? (RlcdFrameBuffer::kHeight - 1 - dy) : dy;
      const int sy = (oy * static_cast<int>(kFmbViewH)) / RlcdFrameBuffer::kHeight;
      const uint8_t* row = mono.data() + sy * (kFmbViewW / 8);
      for (int dx = 0; dx < RlcdFrameBuffer::kWidth; ++dx) {
        const int ox = r180 ? (RlcdFrameBuffer::kWidth - 1 - dx) : dx;
        const int sx = (ox * static_cast<int>(kFmbViewW)) / RlcdFrameBuffer::kWidth;
        const bool white = (row[sx >> 3] & static_cast<uint8_t>(0x80u >> (sx & 7))) != 0;
        fbRef.setPixel(dx, dy, white);
      }
    }
    assert(fbFast.crc32() == fbRef.crc32());
  }

  ButtonController bc;
  bc.init(false, false, 0, true, false);
  FlipCamUiState ui;
  ui.storageMounted = true;
  ui.linkState = LinkState::CONNECTED;
  RlcdFrameBuffer rlcd;
  RlcdRenderContext rctx{};
  rctx.ui = &ui;
  rctx.buttons = &bc;
  rctx.liveMonoPlane = mono.data();
  rctx.freeMiB = 7.0f;
  renderRlcdFrame(rlcd, rctx);

  // Full 400x300 edges carry image pixels (no side pillar bars, no outer frame, no top/bottom full bars)
  assert(rlcd.getPixel(399, 100));   // Top-right edge pixel of full 400x300 Selfie image is white!
  assert(rlcd.getPixel(250, 0));     // Top edge pixel in top-right quadrant is white!
  assert(!rlcd.getPixel(0, 150));    // Left edge pixel is dark (bottom/left of asymmetric image)
  assert(rlcd.getPixel(350, 80));    // Top-right bright photo pixels stay light (not inverted)
  assert(!rlcd.getPixel(50, 80));    // Top-left dark photo pixels stay dark

  // All-white liveMonoPlane proves edge pixels + UI overlay order (UI drawn AFTER image)
  std::vector<uint8_t> whiteMono(kFmbRawPlaneBytes, 0xFFu);
  rctx.liveMonoPlane = whiteMono.data();
  bc.update(true, false, 1000);
  bc.update(true, false, 1030);
  bc.update(true, false, 1530);  // Snap active with ~0.5 holdProgress
  renderRlcdFrame(rlcd, rctx);
  assert(rlcd.getPixel(0, 150));     // Left edge has white image (no outer frame)
  assert(rlcd.getPixel(399, 150));   // Right edge has white image (no outer frame)
  assert(rlcd.getPixel(200, 299));   // Bottom edge has white image (no full-width footer bar)
  assert(rlcd.getPixel(120, 0));     // Top edge outside badges has white image (no full-width top bar)
  assert(!rlcd.getPixel(4, 4));      // Pressed SNAP badge 1px dark outer border overlays white image!
  assert(rlcd.getPixel(6, 6));       // Pressed SNAP badge light fill overlays image!
  assert(!rlcd.getPixel(376, 284));  // Battery dark backing overlays white image at bottom-right!
  // Release SNAP cleanly before Rear 180 check
  bc.update(false, false, 1540);
  bc.update(false, false, 1570);

  // Rear 180 mode rotates 400x300 image 180 deg (top-right -> bottom-left), UI text stays upright!
  rctx.liveMonoPlane = mono.data();
  ui.settings.orientation = static_cast<uint8_t>(OrientationMode::REAR);
  renderRlcdFrame(rlcd, rctx);
  assert(rlcd.getPixel(0, 200));     // Left-bottom edge now white (rotated 180 from top-right!)
  assert(rlcd.getPixel(100, 299));   // Bottom-left edge pixel now white!
  assert(!rlcd.getPixel(399, 100));  // Top-right now dark!

  // 6b. RLCD fallback gray LIVE mode fills full 400x300 in Selfie and Rear 180 without tone inversion
  RlcdRenderContext rctxFallback{};
  rctxFallback.ui = &ui;
  rctxFallback.buttons = &bc;
  rctxFallback.previewGray = src400.data();
  rctxFallback.previewW = 400;
  rctxFallback.previewH = 300;
  rctxFallback.freeMiB = 7.0f;
  ui.settings.look = static_cast<uint8_t>(LookMode::THRESHOLD);
  ui.settings.orientation = static_cast<uint8_t>(OrientationMode::SELFIE);
  renderRlcdFrame(rlcd, rctxFallback);
  assert(rlcd.getPixel(399, 100));   // Selfie top-right edge white
  assert(!rlcd.getPixel(0, 200));    // Selfie bottom-left edge dark
  ui.settings.orientation = static_cast<uint8_t>(OrientationMode::REAR);
  renderRlcdFrame(rlcd, rctxFallback);
  assert(rlcd.getPixel(0, 200));     // Rear bottom-left edge white
  assert(!rlcd.getPixel(399, 100));  // Rear top-right edge dark

  // Offline/waiting black fallback remains legible (no image, crisp light center text on black)
  RlcdRenderContext rctxOffline{};
  rctxOffline.ui = &ui;
  rctxOffline.buttons = &bc;
  ui.linkState = LinkState::DISCONNECTED;
  renderRlcdFrame(rlcd, rctxOffline);
  assert(!rlcd.getPixel(0, 150));    // Black background
  bool anyLitCenterText = false;
  for (int y = 136; y < 166 && !anyLitCenterText; ++y) {
    for (int x = 120; x < 280; ++x) {
      if (rlcd.getPixel(x, y)) { anyLitCenterText = true; break; }
    }
  }
  assert(anyLitCenterText);
  ui.linkState = LinkState::CONNECTED;

  // 6c. GALLERY fills full 400x300 for 4:3 300x225 (decoded 600x450) and NEVER rerotates stored photos in Rear mode!
  std::vector<uint8_t> gal300(300u * 225u, 0);  // All dark except top-right quarter bright (x>=150, y<112)
  for (int y = 0; y < 112; ++y) {
    for (int x = 150; x < 300; ++x) gal300[y * 300 + x] = 255;
  }
  MediaEntry stillEntry{};
  stillEntry.id = 1;
  stillEntry.type = static_cast<uint8_t>(MediaType::STILL);
  stillEntry.width = 600;
  stillEntry.height = 450;
  stillEntry.sizeBytes = 42000;
  stillEntry.look = static_cast<uint8_t>(LookMode::THRESHOLD);
  stillEntry.contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
  std::snprintf(stillEntry.title, sizeof(stillEntry.title), "IMG_0001");
  ui.mode = AppMode::GALLERY;
  ui.setStillCount(1);
  ui.settings.orientation = static_cast<uint8_t>(OrientationMode::REAR);  // Must NOT rerotate stored photo!
  RlcdRenderContext rctxGal{};
  rctxGal.ui = &ui;
  rctxGal.buttons = &bc;
  rctxGal.galleryGray = gal300.data();
  rctxGal.galleryW = 300;
  rctxGal.galleryH = 225;
  rctxGal.currentStill = &stillEntry;
  renderRlcdFrame(rlcd, rctxGal);
  assert(rlcd.getPixel(399, 100));   // Top-right 4:3 photo edge fills full 400x300 (normal tones, NOT rerotated)
  assert(!rlcd.getPixel(0, 100));    // Top-left dark photo area stays black
  assert(!rlcd.getPixel(100, 200));  // Bottom-left dark photo area stays black

  // Non-4:3 older photo (e.g., 160x80 2:1 wide) preserves aspect ratio with black letterbox margins!
  std::vector<uint8_t> galWide(160u * 80u, 255);
  MediaEntry wideEntry = stillEntry;
  wideEntry.width = 160;
  wideEntry.height = 80;
  rctxGal.galleryGray = galWide.data();
  rctxGal.galleryW = 160;
  rctxGal.galleryH = 80;
  rctxGal.currentStill = &wideEntry;
  renderRlcdFrame(rlcd, rctxGal);
  assert(!rlcd.getPixel(200, 30));   // Top letterbox margin stays black (400x200 centered at y=50..249)
  assert(!rlcd.getPixel(200, 270));  // Bottom letterbox margin stays black
  assert(rlcd.getPixel(200, 150));   // Center wide photo is white

  // 7. OLED 4:3 exact 68x51 letterbox inside (0, 11, 128, 53) stays unchanged even when orientation=REAR!
  OledFrameBuffer oled;
  OledRenderContext octx{};
  octx.previewGray = src400.data();
  octx.previewW = 400;
  octx.previewH = 300;
  octx.linkConnected = true;
  octx.cameraOk = true;
  octx.look = static_cast<uint8_t>(LookMode::THRESHOLD);
  octx.contrast = static_cast<uint8_t>(ContrastLevel::C_HIGH);
  octx.orientation = static_cast<uint8_t>(OrientationMode::SELFIE);
  renderOledFrame(oled, octx);
  assert(!oled.getPixel(15, 30));   // Left letterbox margin
  assert(!oled.getPixel(115, 30));  // Right letterbox margin
  assert(oled.getPixel(80, 15));    // Top-right quadrant inside 4:3 box is lit

  OledFrameBuffer oledRear;
  octx.orientation = static_cast<uint8_t>(OrientationMode::REAR);
  renderOledFrame(oledRear, octx);
  // Preview area (y=11..59) on OLED must remain byte-for-byte identical in Rear mode!
  for (int y = 11; y < 60; ++y) {
    for (int x = 0; x < 128; ++x) {
      assert(oledRear.getPixel(x, y) == oled.getPixel(x, y));
    }
  }

  // 8. JPEG SOF dimension parser rejects stale QVGA (320x240) and accepts true SVGA (800x600)
  auto makeJpegWithSof = [](uint16_t w, uint16_t h) {
    std::vector<uint8_t> j(96, 0);
    j[0] = 0xFF; j[1] = 0xD8;  // SOI
    j[2] = 0xFF; j[3] = 0xE0; j[4] = 0x00; j[5] = 0x04;  // APP0 len=4
    j[8] = 0xFF; j[9] = 0xC0; j[10] = 0x00; j[11] = 0x0B;  // SOF0 len=11
    j[12] = 8;
    j[13] = static_cast<uint8_t>(h >> 8);
    j[14] = static_cast<uint8_t>(h & 0xFF);
    j[15] = static_cast<uint8_t>(w >> 8);
    j[16] = static_cast<uint8_t>(w & 0xFF);
    j[17] = 1;
    j[94] = 0xFF; j[95] = 0xD9;  // EOI
    return j;
  };
  const auto jpgSvga = makeJpegWithSof(800, 600);
  const auto jpgQvga = makeJpegWithSof(320, 240);
  uint16_t pw = 0, ph = 0;
  assert(parseJpegDimensions(jpgSvga.data(), jpgSvga.size(), pw, ph) && pw == 800 && ph == 600);
  assert(parseJpegDimensions(jpgQvga.data(), jpgQvga.size(), pw, ph) && pw == 320 && ph == 240);
  std::vector<uint8_t> badJpg(80, 0);
  badJpg[0] = 0xFF; badJpg[1] = 0xD8; badJpg[78] = 0xFF; badJpg[79] = 0xD9;
  assert(!parseJpegDimensions(badJpg.data(), badJpg.size(), pw, ph));
}

void testSoftJpgDecoder() {
  // Public synthetic 64x48 single-component (monochrome, Nf=1) asymmetric JPEG fixture:
  // top-left quadrant (x<32, y<24) bright (~240), other three quadrants dark (~20).
  static const uint8_t kGrayJpg64x48[] = {
      0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x48,
      0x00, 0x48, 0x00, 0x00, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x30, 0x00, 0x40, 0x01, 0x01, 0x11,
      0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
      0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05,
      0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
      0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23,
      0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
      0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
      0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
      0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
      0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
      0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5,
      0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1,
      0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x02, 0x02,
      0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x03, 0x04, 0x03, 0x03, 0x03, 0x04, 0x05, 0x04, 0x04,
      0x04, 0x04, 0x05, 0x07, 0x05, 0x05, 0x05, 0x05, 0x05, 0x07, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07,
      0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B,
      0x0B, 0x0B, 0x0B, 0x0B, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0xFF, 0xDD,
      0x00, 0x04, 0x00, 0x08, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00, 0xFD, 0xC0,
      0xA2, 0x8A, 0x2B, 0xF8, 0x8F, 0xA2, 0x8A, 0x2B, 0xFF, 0xD0, 0xFD, 0xC0, 0xA2, 0x8A, 0x2B, 0xF8,
      0x8F, 0xA2, 0x8A, 0x2B, 0xFF, 0xD1, 0xFD, 0xC0, 0xA2, 0x8A, 0x2B, 0xF8, 0x8F, 0xA2, 0x8A, 0x2B,
      0xFF, 0xD2, 0xFC, 0x4F, 0xA2, 0x8A, 0x28, 0xA2, 0x8A, 0x2B, 0xFF, 0xD3, 0xFC, 0x4F, 0xA2, 0x8A,
      0x28, 0xA2, 0x8A, 0x2B, 0xFF, 0xD4, 0xFC, 0x4F, 0xA2, 0x8A, 0x28, 0xA2, 0x8A, 0x2B, 0xFF, 0xD9,
  };

  std::vector<uint8_t> out(64u * 48u, 0);
  int decW = 0, decH = 0;

  // 1. Single-component monochrome JPEG decode (full scale 64x48)
  assert(decodeJpegToGraySoft(kGrayJpg64x48, sizeof(kGrayJpg64x48), out.data(), out.size(),
                              304, 228, decW, decH));
  assert(decW == 64 && decH == 48);
  assert(out[8 * 64 + 10] > 200);   // Top-left bright quadrant
  assert(out[8 * 64 + 48] < 50);    // Top-right dark quadrant
  assert(out[36 * 64 + 10] < 50);   // Bottom-left dark quadrant
  assert(out[36 * 64 + 48] < 50);   // Bottom-right dark quadrant

  // 2. Monochrome JPEG downscale (scale 1/2 -> 32x24)
  std::fill(out.begin(), out.end(), 0);
  assert(decodeJpegToGraySoft(kGrayJpg64x48, sizeof(kGrayJpg64x48), out.data(), out.size(),
                              32, 24, decW, decH));
  assert(decW == 32 && decH == 24);
  assert(out[4 * 32 + 4] > 200);
  assert(out[18 * 32 + 24] < 50);

  // 3. 3-component RGB/YCbCr JPEG decode via software fallback (tests/fixtures/synthetic-color-64x48.jpg)
  std::ifstream ifs("tests/fixtures/synthetic-color-64x48.jpg", std::ios::binary);
  assert(ifs.good());
  const std::vector<uint8_t> colorFixture((std::istreambuf_iterator<char>(ifs)), {});
  assert(!colorFixture.empty());
  std::fill(out.begin(), out.end(), 0);
  assert(decodeJpegToGraySoft(colorFixture.data(), colorFixture.size(), out.data(), out.size(),
                              304, 228, decW, decH));
  assert(decW == 64 && decH == 48);
  assert(out[8 * 64 + 10] > 200);
  assert(out[36 * 64 + 48] < 50);

  // 4. Bounds & failure guards: truncated, undersized buffer, oversize input, invalid payload
  assert(!decodeJpegToGraySoft(kGrayJpg64x48, sizeof(kGrayJpg64x48) / 2, out.data(), out.size(),
                               304, 228, decW, decH));
  std::vector<uint8_t> corrupt(kGrayJpg64x48, kGrayJpg64x48 + sizeof(kGrayJpg64x48));
  corrupt[corrupt.size() - 8] ^= 0xFFu;
  corrupt[corrupt.size() - 6] ^= 0xFFu;
  assert(!decodeJpegToGraySoft(corrupt.data(), 32, out.data(), out.size(), 304, 228, decW, decH));
  std::vector<uint8_t> tinyDst(10, 0);  // Smaller than even 1/8 scale (8x6 = 48 bytes)
  assert(!decodeJpegToGraySoft(kGrayJpg64x48, sizeof(kGrayJpg64x48), tinyDst.data(), tinyDst.size(),
                               304, 228, decW, decH));
  assert(!decodeJpegToGraySoft(kGrayJpg64x48, kMaxStillJpegBytes + 1u, out.data(), out.size(),
                               304, 228, decW, decH));
  assert(!decodeJpegToGraySoft(nullptr, sizeof(kGrayJpg64x48), out.data(), out.size(),
                               304, 228, decW, decH));
}

}  // namespace

int main() {
  testButtonsAndSteering();
  testReverseChronologicalBrowseUi();
  testProtocolAndFailureCases();
  testStorageAndRecovery();
  testRealButtonToCapturePipeline();
  testDisplayAndBatteryIcons();
  testUsbParser();
  testFastMonoProtocolAndOrientation();
  testSoftJpgDecoder();
  std::printf("ALL C++ HOST TESTS PASSED.\n");
  return 0;
}
