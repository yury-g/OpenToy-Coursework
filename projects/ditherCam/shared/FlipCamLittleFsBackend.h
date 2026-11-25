// SPDX-License-Identifier: MIT
#pragma once
#ifdef ARDUINO

#include <FS.h>
#include <LittleFS.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "FlipCamStorage.h"

namespace flipcam {

// Converts "/media/foo" -> "/foo" because Arduino LittleFS mounts partition at basePath="/media"
// and File paths passed to LittleFS.open() are relative to that mount point.
inline const char* stripBasePrefix(const char* path) {
  if (!path) return "/";
  constexpr const char* kPrefix = "/media";
  constexpr size_t kPrefixLen = 6;
  if (std::strncmp(path, kPrefix, kPrefixLen) == 0) {
    const char* rest = path + kPrefixLen;
    return rest[0] == '\0' ? "/" : rest;
  }
  return path;
}

class LittleFsBackend : public IStorageBackend {
 public:
  bool mountNoFormat() override {
    // CRITICAL: formatOnFail MUST be false (never auto-format on mount failure).
    mounted_ = LittleFS.begin(false, "/media", 10, kMediaPartitionLabel);
    return mounted_;
  }

  bool formatPartition(const char* exactLabel) override {
    if (!exactLabel || std::strcmp(exactLabel, kMediaPartitionLabel) != 0) return false;
    LittleFS.end();
    mounted_ = false;
    // Register with formatOnFail=false, or mount & format explicitly
    if (!LittleFS.begin(false, "/media", 10, kMediaPartitionLabel)) {
      // Unformatted partition: use esp_littlefs_format directly via LittleFS.format()
      // Note: LittleFS.begin sets partitionLabel_ before failing, so LittleFS.format() targets "media".
      if (!LittleFS.format()) return false;
      mounted_ = LittleFS.begin(false, "/media", 10, kMediaPartitionLabel);
      return mounted_;
    }
    const bool ok = LittleFS.format();
    if (!ok) return false;
    LittleFS.end();
    mounted_ = LittleFS.begin(false, "/media", 10, kMediaPartitionLabel);
    return mounted_;
  }

  bool isMounted() const override { return mounted_; }

  uint64_t totalBytes() const override {
    return mounted_ ? static_cast<uint64_t>(LittleFS.totalBytes()) : 0;
  }

  uint64_t usedBytes() const override {
    return mounted_ ? static_cast<uint64_t>(LittleFS.usedBytes()) : 0;
  }

  bool exists(const char* path) const override {
    if (!mounted_) return false;
    return LittleFS.exists(stripBasePrefix(path));
  }

  bool fileSize(const char* path, size_t& outSize) const override {
    if (!mounted_) return false;
    File f = LittleFS.open(stripBasePrefix(path), FILE_READ);
    if (!f || f.isDirectory()) {
      if (f) f.close();
      return false;
    }
    outSize = f.size();
    f.close();
    return true;
  }

  bool removeFile(const char* path) override {
    if (!mounted_) return false;
    return LittleFS.remove(stripBasePrefix(path));
  }

  bool renameFile(const char* fromPath, const char* toPath) override {
    if (!mounted_) return false;
    // LittleFS rename atomically replaces destination if it exists; never delete dst first!
    return LittleFS.rename(stripBasePrefix(fromPath), stripBasePrefix(toPath));
  }

  bool writeAll(const char* path, const uint8_t* data, size_t len) override {
    if (!mounted_ || (!data && len > 0)) return false;
    File f = LittleFS.open(stripBasePrefix(path), FILE_WRITE);
    if (!f) return false;
    size_t written = 0;
    while (written < len) {
      const size_t chunk = (len - written > 4096) ? 4096 : (len - written);
      const size_t w = f.write(data + written, chunk);
      if (w == 0) {
        f.close();
        return false;
      }
      written += w;
    }
    f.flush();
    f.close();
    return written == len;
  }

  bool readAll(const char* path, std::vector<uint8_t>& out, size_t maxBytes) const override {
    out.clear();
    if (!mounted_) return false;
    File f = LittleFS.open(stripBasePrefix(path), FILE_READ);
    if (!f || f.isDirectory()) {
      if (f) f.close();
      return false;
    }
    const size_t sz = f.size();
    if (sz > maxBytes) {
      f.close();
      return false;
    }
    out.resize(sz);
    size_t got = 0;
    while (got < sz) {
      const size_t r = f.read(out.data() + got, sz - got);
      if (r == 0) break;
      got += r;
    }
    f.close();
    return got == sz;
  }

  bool readSlice(const char* path, size_t offset, uint8_t* out, size_t len) const override {
    if (!mounted_ || !out) return false;
    File f = LittleFS.open(stripBasePrefix(path), FILE_READ);
    if (!f || f.isDirectory()) {
      if (f) f.close();
      return false;
    }
    if (offset + len > f.size()) {
      f.close();
      return false;
    }
    if (!f.seek(offset)) {
      f.close();
      return false;
    }
    const size_t got = f.read(out, len);
    f.close();
    return got == len;
  }

  bool listDir(const char* dirPath, std::vector<std::string>& outNames) const override {
    outNames.clear();
    if (!mounted_) return false;
    File dir = LittleFS.open(stripBasePrefix(dirPath));
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      return false;
    }
    File entry = dir.openNextFile();
    while (entry) {
      const char* name = entry.name();
      if (name) {
        const char* slash = std::strrchr(name, '/');
        outNames.emplace_back(slash ? slash + 1 : name);
      }
      entry.close();
      entry = dir.openNextFile();
    }
    dir.close();
    return true;
  }

 private:
  bool mounted_ = false;
};

}  // namespace flipcam

#endif  // ARDUINO
