// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include "FlipCamCrc.h"
#include "FlipCamProtocol.h"
#include "FlipCamTypes.h"

namespace flipcam {

// Abstract filesystem backend so identical atomic-write/recovery/space-check
// logic runs in host C++ tests AND on ESP32 LittleFS.
class IStorageBackend {
 public:
  virtual ~IStorageBackend() = default;
  virtual bool mountNoFormat() = 0;
  virtual bool formatPartition(const char* exactLabel) = 0;
  virtual bool isMounted() const = 0;
  virtual uint64_t totalBytes() const = 0;
  virtual uint64_t usedBytes() const = 0;
  virtual bool exists(const char* path) const = 0;
  virtual bool fileSize(const char* path, size_t& outSize) const = 0;
  virtual bool removeFile(const char* path) = 0;
  virtual bool renameFile(const char* fromPath, const char* toPath) = 0;
  virtual bool writeAll(const char* path, const uint8_t* data, size_t len) = 0;
  virtual bool readAll(const char* path, std::vector<uint8_t>& out, size_t maxBytes) const = 0;
  virtual bool readSlice(const char* path, size_t offset, uint8_t* out, size_t len) const = 0;
  virtual bool listDir(const char* dirPath, std::vector<std::string>& outNames) const = 0;
};

struct StorageStatus {
  bool mounted = false;
  bool formattedOnFail = false;  // Always false!
  uint64_t totalBytes = 0;
  uint64_t usedBytes = 0;
  uint64_t freeBytes = 0;
  uint32_t recoveredTempDeleted = 0;
  uint32_t rejectedCorruptCount = 0;
  char fault[40] = {};
};

constexpr uint32_t kIndexMagic = 0x31584449u;  // 'IDX1'
constexpr size_t kStorageReserveBytes = 64u * 1024u;
constexpr size_t kMaxMediaItems = 256u;

#pragma pack(push, 1)
struct IndexHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t count;
  uint32_t nextId;
  uint32_t prodCaptureCount;
  uint32_t synthCaptureCount;
  uint32_t crc32;
};
#pragma pack(pop)

class MediaStore {
 public:
  static constexpr const char* kBaseDir = "/media";
  static constexpr const char* kIndexPath = "/media/index.bin";
  static constexpr const char* kIndexTmpPath = "/media/index.tmp";
  static constexpr const char* kSettingsPath = "/media/settings.bin";
  static constexpr const char* kSettingsTmpPath = "/media/settings.tmp";

  explicit MediaStore(IStorageBackend& backend) : backend_(backend) {}

  bool beginNoAutoFormat() {
    status_ = StorageStatus{};
    if (!backend_.mountNoFormat()) {
      status_.mounted = false;
      std::snprintf(status_.fault, sizeof(status_.fault), "MOUNT_FAILED_NO_AUTOFORMAT");
      entries_.clear();
      return false;
    }
    status_.mounted = true;
    recoverOnBoot();
    refreshSpace();
    return true;
  }

  bool guardedFormat(const char* providedLabel, bool busyWriting) {
    if (busyWriting) {
      std::snprintf(status_.fault, sizeof(status_.fault), "FORMAT_BUSY");
      return false;
    }
    if (!providedLabel || std::strcmp(providedLabel, kMediaPartitionLabel) != 0) {
      std::snprintf(status_.fault, sizeof(status_.fault), "FORMAT_BAD_LABEL");
      return false;
    }
    if (!backend_.formatPartition(providedLabel)) {
      std::snprintf(status_.fault, sizeof(status_.fault), "FORMAT_FAILED");
      return false;
    }
    entries_.clear();
    nextId_ = 1;
    prodCaptures_ = 0;
    synthCaptures_ = 0;
    status_.mounted = backend_.mountNoFormat();
    status_.fault[0] = '\0';
    if (status_.mounted) {
      saveIndex();
      refreshSpace();
    }
    return status_.mounted;
  }

  void refreshSpace() {
    if (!backend_.isMounted()) {
      status_.mounted = false;
      status_.totalBytes = 0;
      status_.usedBytes = 0;
      status_.freeBytes = 0;
      return;
    }
    status_.mounted = true;
    status_.totalBytes = backend_.totalBytes();
    status_.usedBytes = backend_.usedBytes();
    status_.freeBytes = (status_.totalBytes > status_.usedBytes)
                            ? (status_.totalBytes - status_.usedBytes)
                            : 0;
  }

  bool hasSpaceFor(size_t neededBytes) const {
    if (!status_.mounted) return false;
    if (neededBytes > kMaxClipFileBytes) return false;
    return status_.freeBytes >= (static_cast<uint64_t>(neededBytes) + kStorageReserveBytes);
  }

  bool isFullForMinCapture() const {
    return !hasSpaceFor(32u * 1024u);
  }

  // Atomic still save: validate JPEG -> check space -> write tmp -> verify -> rename -> save index
  bool saveStill(const uint8_t* jpegData, size_t len, uint16_t width, uint16_t height,
                 uint8_t look, uint8_t contrast, uint8_t mirror, bool synthetic,
                 MediaEntry& outEntry) {
    refreshSpace();
    if (!status_.mounted) {
      std::snprintf(status_.fault, sizeof(status_.fault), "NO_FS");
      return false;
    }
    if (!isValidJpegBuffer(jpegData, len, kMaxStillJpegBytes)) {
      std::snprintf(status_.fault, sizeof(status_.fault), "INVALID_JPEG");
      return false;
    }
    if (!hasSpaceFor(len + 4096u)) {
      std::snprintf(status_.fault, sizeof(status_.fault), "STORAGE_FULL");
      return false;
    }
    if (entries_.size() >= kMaxMediaItems) {
      std::snprintf(status_.fault, sizeof(status_.fault), "INDEX_FULL");
      return false;
    }

    const uint32_t id = nextId_;
    char finalName[32];
    std::snprintf(finalName, sizeof(finalName), "S_%06u.jpg", static_cast<unsigned>(id));
    char tmpName[32];
    std::snprintf(tmpName, sizeof(tmpName), "tmp_%06u.part", static_cast<unsigned>(id));

    char tmpPath[64];
    char finalPath[64];
    if (!buildSafeMediaPath(kBaseDir, tmpName, tmpPath, sizeof(tmpPath)) ||
        !buildSafeMediaPath(kBaseDir, finalName, finalPath, sizeof(finalPath))) {
      return false;
    }

    const uint32_t crc = crc32Compute(jpegData, len);
    if (!backend_.writeAll(tmpPath, jpegData, len)) {
      backend_.removeFile(tmpPath);
      std::snprintf(status_.fault, sizeof(status_.fault), "WRITE_FAIL");
      return false;
    }

    size_t writtenSz = 0;
    if (!backend_.fileSize(tmpPath, writtenSz) || writtenSz != len) {
      backend_.removeFile(tmpPath);
      std::snprintf(status_.fault, sizeof(status_.fault), "VERIFY_SIZE_FAIL");
      return false;
    }

    if (!backend_.renameFile(tmpPath, finalPath)) {
      backend_.removeFile(tmpPath);
      std::snprintf(status_.fault, sizeof(status_.fault), "RENAME_FAIL");
      return false;
    }

    MediaEntry e{};
    e.id = id;
    e.type = static_cast<uint8_t>(MediaType::STILL);
    e.look = look;
    e.contrast = contrast;
    e.mirror = mirror;
    e.synthetic = synthetic ? 1 : 0;
    e.hasAudio = 0;
    e.width = width;
    e.height = height;
    e.sizeBytes = static_cast<uint32_t>(len);
    e.durationMs = 0;
    e.crc32 = crc;
    e.createdSeq = id;
    std::snprintf(e.filename, sizeof(e.filename), "%s", finalName);
    std::snprintf(e.title, sizeof(e.title), "%s #%03u",
                  synthetic ? "SYNTH" : "PHOTO", static_cast<unsigned>(id));

    nextId_++;
    if (synthetic) synthCaptures_++;
    else prodCaptures_++;
    entries_.insert(entries_.begin(), e);  // Newest first!
    saveIndex();
    refreshSpace();
    outEntry = e;
    status_.fault[0] = '\0';
    return true;
  }

  // Finalize a pre-written FCV1 clip temp file after validating its header/chunks/CRC
  bool finalizeClipTempFile(const char* tmpFilename, uint16_t width, uint16_t height,
                            uint8_t look, uint8_t contrast, uint8_t mirror, bool synthetic,
                            MediaEntry& outEntry) {
    refreshSpace();
    if (!status_.mounted) return false;
    char tmpPath[64];
    if (!buildSafeMediaPath(kBaseDir, tmpFilename, tmpPath, sizeof(tmpPath))) return false;

    std::vector<uint8_t> buf;
    if (!backend_.readAll(tmpPath, buf, kMaxClipFileBytes)) {
      backend_.removeFile(tmpPath);
      std::snprintf(status_.fault, sizeof(status_.fault), "CLIP_OVERSIZE_OR_READ_FAIL");
      return false;
    }
    const FcvSummary sum = validateFcvBuffer(buf.data(), buf.size());
    if (!sum.valid) {
      backend_.removeFile(tmpPath);
      std::snprintf(status_.fault, sizeof(status_.fault), "CLIP_CORRUPT");
      return false;
    }

    const uint32_t id = nextId_;
    char finalName[32];
    std::snprintf(finalName, sizeof(finalName), "C_%06u.fcv", static_cast<unsigned>(id));
    char finalPath[64];
    if (!buildSafeMediaPath(kBaseDir, finalName, finalPath, sizeof(finalPath))) return false;

    if (!backend_.renameFile(tmpPath, finalPath)) {
      backend_.removeFile(tmpPath);
      return false;
    }

    MediaEntry e{};
    e.id = id;
    e.type = static_cast<uint8_t>(MediaType::CLIP);
    e.look = look;
    e.contrast = contrast;
    e.mirror = mirror;
    e.synthetic = synthetic ? 1 : 0;
    e.hasAudio = sum.hasAudio ? 1 : 0;
    e.width = width;
    e.height = height;
    e.sizeBytes = static_cast<uint32_t>(buf.size());
    e.durationMs = sum.durationMs;
    e.crc32 = crc32Compute(buf.data(), buf.size());
    e.createdSeq = id;
    std::snprintf(e.filename, sizeof(e.filename), "%s", finalName);
    std::snprintf(e.title, sizeof(e.title), "%s #%03u",
                  synthetic ? "SCLIP" : "CLIP", static_cast<unsigned>(id));

    nextId_++;
    if (synthetic) synthCaptures_++;
    else prodCaptures_++;
    entries_.insert(entries_.begin(), e);  // Newest first
    saveIndex();
    refreshSpace();
    outEntry = e;
    status_.fault[0] = '\0';
    return true;
  }

  bool deleteById(uint32_t id) {
    if (!status_.mounted || id == 0) return false;
    for (size_t i = 0; i < entries_.size(); ++i) {
      if (entries_[i].id == id) {
        char path[64];
        if (buildSafeMediaPath(kBaseDir, entries_[i].filename, path, sizeof(path))) {
          backend_.removeFile(path);
        }
        entries_.erase(entries_.begin() + static_cast<long>(i));
        saveIndex();
        refreshSpace();
        return true;
      }
    }
    return false;
  }

  const MediaEntry* findById(uint32_t id) const {
    for (const auto& e : entries_) {
      if (e.id == id) return &e;
    }
    return nullptr;
  }

  bool readMediaChunk(uint32_t id, size_t offset, size_t len,
                      std::vector<uint8_t>& outBytes, size_t& outTotalSize) const {
    outBytes.clear();
    outTotalSize = 0;
    if (!status_.mounted || len == 0 || len > 4096u) return false;
    const MediaEntry* e = findById(id);
    if (!e) return false;
    char path[64];
    if (!buildSafeMediaPath(kBaseDir, e->filename, path, sizeof(path))) return false;
    size_t sz = 0;
    if (!backend_.fileSize(path, sz) || offset >= sz) return false;
    outTotalSize = sz;
    const size_t actual = std::min(len, sz - offset);
    outBytes.resize(actual);
    if (!backend_.readSlice(path, offset, outBytes.data(), actual)) {
      outBytes.clear();
      return false;
    }
    return true;
  }

  // Still-only reverse chronological accessors (0 = newest still)
  int stillCount() const {
    int c = 0;
    for (const auto& e : entries_) {
      if (e.type == static_cast<uint8_t>(MediaType::STILL)) c++;
    }
    return c;
  }

  const MediaEntry* getStillNewestFirst(int index) const {
    if (index < 0) return nullptr;
    int seen = 0;
    for (const auto& e : entries_) {
      if (e.type == static_cast<uint8_t>(MediaType::STILL)) {
        if (seen == index) return &e;
        seen++;
      }
    }
    return nullptr;
  }

  const MediaEntry* latestEntry() const {
    return entries_.empty() ? nullptr : &entries_.front();
  }

  const std::vector<MediaEntry>& entries() const { return entries_; }
  const StorageStatus& status() const { return status_; }
  uint32_t prodCaptures() const { return prodCaptures_; }
  uint32_t synthCaptures() const { return synthCaptures_; }
  uint32_t nextId() const { return nextId_; }

  bool loadSettings(FlipCamSettings& out) const {
    if (!backend_.isMounted() || !backend_.exists(kSettingsPath)) return false;
    std::vector<uint8_t> buf;
    if (!backend_.readAll(kSettingsPath, buf, sizeof(FlipCamSettings))) return false;
    if (buf.size() != sizeof(FlipCamSettings)) return false;
    FlipCamSettings s{};
    std::memcpy(&s, buf.data(), sizeof(s));
    if (s.magic != 0x46435331u || s.version != 1) return false;
    const uint32_t crc = crc32Compute(reinterpret_cast<const uint8_t*>(&s),
                                      sizeof(FlipCamSettings) - sizeof(uint32_t));
    if (crc != s.crc32) return false;
    if (s.orientation > static_cast<uint8_t>(OrientationMode::REAR)) {
      s.orientation = static_cast<uint8_t>(OrientationMode::SELFIE);
    }
    out = s;
    return true;
  }

  bool saveSettingsIfChanged(const FlipCamSettings& desired) {
    if (!backend_.isMounted()) return false;
    FlipCamSettings current{};
    if (loadSettings(current) && current == desired) {
      return true;  // Already persisted; avoid unnecessary flash wear!
    }
    FlipCamSettings s = desired;
    s.magic = 0x46435331u;
    s.version = 1;
    s.crc32 = crc32Compute(reinterpret_cast<const uint8_t*>(&s),
                           sizeof(FlipCamSettings) - sizeof(uint32_t));
    if (!backend_.writeAll(kSettingsTmpPath, reinterpret_cast<const uint8_t*>(&s), sizeof(s))) {
      backend_.removeFile(kSettingsTmpPath);
      return false;
    }
    if (!backend_.renameFile(kSettingsTmpPath, kSettingsPath)) {
      backend_.removeFile(kSettingsTmpPath);
      return false;
    }
    return true;
  }

 private:
  IStorageBackend& backend_;
  StorageStatus status_{};
  std::vector<MediaEntry> entries_;
  uint32_t nextId_ = 1;
  uint32_t prodCaptures_ = 0;
  uint32_t synthCaptures_ = 0;

  static bool parseFinalFilename(const char* name, uint32_t& outId, uint8_t& outType) {
    if (!name) return false;
    size_t len = std::strlen(name);
    // Expected: S_000001.jpg or C_000001.fcv (at least 12 chars)
    if (len < 12 || name[1] != '_') return false;
    uint8_t type = 0;
    const char* ext = nullptr;
    if (name[0] == 'S') {
      type = static_cast<uint8_t>(MediaType::STILL);
      ext = ".jpg";
    } else if (name[0] == 'C') {
      type = static_cast<uint8_t>(MediaType::CLIP);
      ext = ".fcv";
    } else {
      return false;
    }
    if (len < 6 || std::strcmp(name + len - 4, ext) != 0) return false;
    char numBuf[16]{};
    const size_t numLen = len - 6;
    if (numLen == 0 || numLen >= sizeof(numBuf)) return false;
    std::memcpy(numBuf, name + 2, numLen);
    numBuf[numLen] = '\0';
    uint32_t id = 0;
    if (!parsePositiveId(numBuf, id)) return false;
    outId = id;
    outType = type;
    return true;
  }

  bool validateAndBuildDiskEntry(const char* filename, uint32_t id, uint8_t type,
                                 MediaEntry& outEntry) const {
    char path[64];
    size_t sz = 0;
    if (!buildSafeMediaPath(kBaseDir, filename, path, sizeof(path)) ||
        !backend_.fileSize(path, sz) || sz == 0) {
      return false;
    }
    outEntry = MediaEntry{};
    outEntry.id = id;
    outEntry.type = type;
    outEntry.sizeBytes = static_cast<uint32_t>(sz);
    outEntry.createdSeq = id;
    std::snprintf(outEntry.filename, sizeof(outEntry.filename), "%s", filename);

    if (type == static_cast<uint8_t>(MediaType::STILL)) {
      uint8_t head[2]{};
      uint8_t tail[4]{};
      const size_t tailOff = sz >= 4 ? sz - 4 : 0;
      if (sz < kMinJpegBytes || sz > kMaxStillJpegBytes ||
          !backend_.readSlice(path, 0, head, 2) ||
          !backend_.readSlice(path, tailOff, tail, sz - tailOff) ||
          head[0] != 0xFF || head[1] != 0xD8) {
        return false;
      }
      bool hasEoi = false;
      for (size_t k = 0; k + 1 < (sz - tailOff); ++k) {
        if (tail[k] == 0xFF && tail[k + 1] == 0xD9) {
          hasEoi = true;
          break;
        }
      }
      if (!hasEoi) return false;
      std::vector<uint8_t> buf;
      if (!backend_.readAll(path, buf, kMaxStillJpegBytes)) {
        return false;
      }
      outEntry.crc32 = crc32Compute(buf.data(), buf.size());
      std::snprintf(outEntry.title, sizeof(outEntry.title), "PHOTO #%03u", static_cast<unsigned>(id));
      return true;
    }

    if (type == static_cast<uint8_t>(MediaType::CLIP)) {
      FcvFileHeader fhdr{};
      if (sz < sizeof(FcvFileHeader) || sz > kMaxClipFileBytes ||
          !backend_.readSlice(path, 0, reinterpret_cast<uint8_t*>(&fhdr), sizeof(fhdr)) ||
          !validateFcvHeader(fhdr)) {
        return false;
      }
      std::vector<uint8_t> buf;
      if (!backend_.readAll(path, buf, kMaxClipFileBytes)) {
        return false;
      }
      const FcvSummary sum = validateFcvBuffer(buf.data(), buf.size());
      if (!sum.valid) {
        return false;
      }
      outEntry.durationMs = sum.durationMs;
      outEntry.hasAudio = sum.hasAudio ? 1 : 0;
      outEntry.synthetic = (fhdr.flags & kFcvFlagSynthetic) != 0 ? 1 : 0;
      outEntry.crc32 = crc32Compute(buf.data(), buf.size());
      std::snprintf(outEntry.title, sizeof(outEntry.title), "CLIP #%03u", static_cast<unsigned>(id));
      return true;
    }
    return false;
  }

  void recoverOnBoot() {
    entries_.clear();
    nextId_ = 1;
    prodCaptures_ = 0;
    synthCaptures_ = 0;

    // 1. Clean up interrupted temp files and collect all final media files on disk
    std::vector<std::string> names;
    struct DiskFile {
      std::string name;
      uint32_t id;
      uint8_t type;
    };
    std::vector<DiskFile> diskFiles;
    uint32_t maxSeenId = 0;

    if (backend_.listDir(kBaseDir, names)) {
      for (const auto& n : names) {
        const bool isPart = n.rfind("tmp_", 0) == 0 ||
                            (n.size() >= 4 && n.compare(n.size() - 4, 4, ".tmp") == 0);
        if (isPart) {
          char path[64];
          if (buildSafeMediaPath(kBaseDir, n.c_str(), path, sizeof(path))) {
            if (backend_.removeFile(path)) {
              status_.recoveredTempDeleted++;
            }
          }
          continue;
        }
        uint32_t fid = 0;
        uint8_t ftype = 0;
        if (parseFinalFilename(n.c_str(), fid, ftype)) {
          diskFiles.push_back({n, fid, ftype});
          if (fid > maxSeenId) maxSeenId = fid;
        }
      }
    }

    // 2. Load and validate index.bin if present and intact
    bool indexValid = false;
    bool needsResave = false;
    std::vector<uint32_t> rejectedIndexIds;
    std::vector<uint8_t> idxBuf;
    if (backend_.exists(kIndexPath) &&
        backend_.readAll(kIndexPath, idxBuf, sizeof(IndexHeader) + kMaxMediaItems * sizeof(MediaEntry)) &&
        idxBuf.size() >= sizeof(IndexHeader)) {
      IndexHeader hdr{};
      std::memcpy(&hdr, idxBuf.data(), sizeof(hdr));
      const size_t expectedSize =
          sizeof(IndexHeader) + static_cast<size_t>(hdr.count) * sizeof(MediaEntry);
      if (hdr.magic == kIndexMagic && hdr.version == 1 &&
          hdr.count <= kMaxMediaItems && idxBuf.size() == expectedSize) {
        IndexHeader hdrForCrc = hdr;
        hdrForCrc.crc32 = 0;
        uint32_t crc = crc32Update(0, reinterpret_cast<const uint8_t*>(&hdrForCrc), sizeof(hdrForCrc));
        if (hdr.count > 0) {
          crc = crc32Update(crc, idxBuf.data() + sizeof(IndexHeader), hdr.count * sizeof(MediaEntry));
        }
        if (crc == hdr.crc32) {
          indexValid = true;
          nextId_ = std::max<uint32_t>(1u, hdr.nextId);
          prodCaptures_ = hdr.prodCaptureCount;
          synthCaptures_ = hdr.synthCaptureCount;

          for (uint16_t i = 0; i < hdr.count; ++i) {
            MediaEntry e{};
            std::memcpy(&e, idxBuf.data() + sizeof(IndexHeader) + i * sizeof(MediaEntry), sizeof(e));
            if (e.id > maxSeenId) maxSeenId = e.id;
            MediaEntry verified{};
            if (!validateAndBuildDiskEntry(e.filename, e.id, e.type, verified) ||
                verified.sizeBytes != e.sizeBytes ||
                (e.crc32 != 0 && verified.crc32 != e.crc32)) {
              status_.rejectedCorruptCount++;
              rejectedIndexIds.push_back(e.id);
              needsResave = true;
              continue;
            }
            entries_.push_back(e);
          }
        }
      }
      if (!indexValid) {
        status_.rejectedCorruptCount++;
        needsResave = true;
      }
    } else if (!diskFiles.empty()) {
      // Missing index.bin with existing media on disk: rebuild without reusing IDs!
      needsResave = true;
    }

    // 3. Reconcile any valid orphan final media files on disk not yet in entries_
    // (handles missing/corrupt index.bin OR power cut between final rename and index commit)
    for (const auto& df : diskFiles) {
      if (findById(df.id) != nullptr) continue;
      if (std::find(rejectedIndexIds.begin(), rejectedIndexIds.end(), df.id) != rejectedIndexIds.end()) {
        continue;
      }
      MediaEntry recovered{};
      if (validateAndBuildDiskEntry(df.name.c_str(), df.id, df.type, recovered)) {
        if (entries_.size() < kMaxMediaItems) {
          entries_.push_back(recovered);
          needsResave = true;
        }
      } else {
        status_.rejectedCorruptCount++;
      }
    }

    // Sort newest-first by id descending so reverse-chronological order is preserved
    std::sort(entries_.begin(), entries_.end(),
              [](const MediaEntry& a, const MediaEntry& b) { return a.id > b.id; });

    // Never reuse IDs across missing/corrupt index or recovered orphan files
    if (maxSeenId >= nextId_) {
      nextId_ = maxSeenId + 1u;
    }
    if (!indexValid && !entries_.empty()) {
      prodCaptures_ = 0;
      synthCaptures_ = 0;
      for (const auto& e : entries_) {
        if (e.synthetic) synthCaptures_++;
        else prodCaptures_++;
      }
    }

    if (needsResave) {
      saveIndex();
    }
  }

  bool saveIndex() {
    if (!backend_.isMounted()) return false;
    const uint16_t count = static_cast<uint16_t>(std::min<size_t>(entries_.size(), kMaxMediaItems));
    std::vector<uint8_t> buf(sizeof(IndexHeader) + count * sizeof(MediaEntry));
    IndexHeader hdr{};
    hdr.magic = kIndexMagic;
    hdr.version = 1;
    hdr.count = count;
    hdr.nextId = nextId_;
    hdr.prodCaptureCount = prodCaptures_;
    hdr.synthCaptureCount = synthCaptures_;
    hdr.crc32 = 0;

    if (count > 0) {
      std::memcpy(buf.data() + sizeof(IndexHeader), entries_.data(), count * sizeof(MediaEntry));
    }
    uint32_t crc = crc32Update(0, reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
    if (count > 0) {
      crc = crc32Update(crc, buf.data() + sizeof(IndexHeader), count * sizeof(MediaEntry));
    }
    hdr.crc32 = crc;
    std::memcpy(buf.data(), &hdr, sizeof(hdr));

    if (!backend_.writeAll(kIndexTmpPath, buf.data(), buf.size())) {
      backend_.removeFile(kIndexTmpPath);
      return false;
    }
    if (!backend_.renameFile(kIndexTmpPath, kIndexPath)) {
      backend_.removeFile(kIndexTmpPath);
      return false;
    }
    return true;
  }
};

}  // namespace flipcam
