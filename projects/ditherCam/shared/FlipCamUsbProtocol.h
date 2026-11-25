// SPDX-License-Identifier: MIT
#pragma once
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace flipcam {

enum class UsbCmdKind : uint8_t {
  INVALID = 0,
  STATUS,
  DIAG,
  CAPTURE,
  CAPTURE_SYNTH,
  RECORD_START,
  RECORD_STOP,
  LIST,
  PLAY,
  STOP,
  DELETE_ID,
  TONE,
  SET_SETTING,
  GET_SETTINGS,
  SCREENSHOT,
  TESTMODE_ON,
  TESTMODE_OFF,
  INJECT,
  READ_MEDIA,
  FORMAT_MEDIA,
  REBOOT,
};

struct ParsedUsbCommand {
  UsbCmdKind kind = UsbCmdKind::INVALID;
  char arg1[32] = {};
  char arg2[32] = {};
  uint32_t num1 = 0;
  uint32_t num2 = 0;
};

inline void trimAscii(const char* in, char* out, size_t cap) {
  if (!in || !out || cap == 0) return;
  size_t len = std::strlen(in);
  size_t start = 0;
  while (start < len && (in[start] == ' ' || in[start] == '\t' || in[start] == '\r' || in[start] == '\n')) {
    ++start;
  }
  while (len > start && (in[len - 1] == ' ' || in[len - 1] == '\t' || in[len - 1] == '\r' || in[len - 1] == '\n')) {
    --len;
  }
  const size_t n = (len - start) < (cap - 1) ? (len - start) : (cap - 1);
  std::memcpy(out, in + start, n);
  out[n] = '\0';
}

inline int splitTokens(const char* line, char tok[4][32]) {
  int count = 0;
  size_t i = 0;
  while (line[i] != '\0' && count < 4) {
    while (line[i] == ' ' || line[i] == '\t') ++i;
    if (line[i] == '\0') break;
    size_t k = 0;
    while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t') {
      if (k < 31) tok[count][k++] = line[i];
      ++i;
    }
    tok[count][k] = '\0';
    ++count;
  }
  return count;
}

inline bool eqUpper(const char* a, const char* b) {
  for (size_t i = 0;; ++i) {
    const unsigned char ca = static_cast<unsigned char>(a[i]);
    const unsigned char cb = static_cast<unsigned char>(b[i]);
    if (std::toupper(ca) != std::toupper(cb)) return false;
    if (ca == '\0') return true;
  }
}

inline bool parseUIntStrict(const char* s, uint32_t& out) {
  if (!s || !s[0]) return false;
  uint64_t v = 0;
  for (size_t i = 0; s[i] != '\0'; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    v = v * 10u + static_cast<uint64_t>(s[i] - '0');
    if (v > 0xFFFFFFFFull) return false;
  }
  out = static_cast<uint32_t>(v);
  return true;
}

inline ParsedUsbCommand parseUsbCommand(const char* rawLine) {
  ParsedUsbCommand cmd{};
  char trimmed[128]{};
  trimAscii(rawLine, trimmed, sizeof(trimmed));
  if (!trimmed[0]) return cmd;

  char tok[4][32]{};
  const int n = splitTokens(trimmed, tok);
  if (n <= 0) return cmd;

  if (eqUpper(tok[0], "STATUS") && n == 1) {
    cmd.kind = UsbCmdKind::STATUS;
  } else if (eqUpper(tok[0], "DIAG") && n == 1) {
    cmd.kind = UsbCmdKind::DIAG;
  } else if (eqUpper(tok[0], "CAPTURE")) {
    if (n == 1) cmd.kind = UsbCmdKind::CAPTURE;
    else if (n == 2 && eqUpper(tok[1], "SYNTH")) cmd.kind = UsbCmdKind::CAPTURE_SYNTH;
  } else if (eqUpper(tok[0], "RECORD") && n == 2) {
    if (eqUpper(tok[1], "START")) cmd.kind = UsbCmdKind::RECORD_START;
    else if (eqUpper(tok[1], "STOP")) cmd.kind = UsbCmdKind::RECORD_STOP;
  } else if (eqUpper(tok[0], "LIST") && n == 1) {
    cmd.kind = UsbCmdKind::LIST;
  } else if (eqUpper(tok[0], "PLAY") && n == 2) {
    if (parseUIntStrict(tok[1], cmd.num1) && cmd.num1 > 0) cmd.kind = UsbCmdKind::PLAY;
  } else if (eqUpper(tok[0], "STOP") && n == 1) {
    cmd.kind = UsbCmdKind::STOP;
  } else if (eqUpper(tok[0], "DELETE") && n == 2) {
    if (parseUIntStrict(tok[1], cmd.num1) && cmd.num1 > 0) cmd.kind = UsbCmdKind::DELETE_ID;
  } else if (eqUpper(tok[0], "TONE")) {
    cmd.kind = UsbCmdKind::TONE;
    cmd.num1 = 880;
    cmd.num2 = 250;
    if (n >= 2 && !parseUIntStrict(tok[1], cmd.num1)) cmd.kind = UsbCmdKind::INVALID;
    if (n >= 3 && !parseUIntStrict(tok[2], cmd.num2)) cmd.kind = UsbCmdKind::INVALID;
    if (cmd.num1 < 100 || cmd.num1 > 8000 || cmd.num2 < 20 || cmd.num2 > 3000) {
      cmd.kind = UsbCmdKind::INVALID;
    }
  } else if (eqUpper(tok[0], "SET") && n == 3) {
    cmd.kind = UsbCmdKind::SET_SETTING;
    for (size_t i = 0; tok[1][i] && i < 31; ++i) cmd.arg1[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(tok[1][i])));
    for (size_t i = 0; tok[2][i] && i < 31; ++i) cmd.arg2[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(tok[2][i])));
  } else if (eqUpper(tok[0], "GET") && n == 2 && eqUpper(tok[1], "SETTINGS")) {
    cmd.kind = UsbCmdKind::GET_SETTINGS;
  } else if (eqUpper(tok[0], "SCREENSHOT")) {
    cmd.kind = UsbCmdKind::SCREENSHOT;
    if (n == 2) std::snprintf(cmd.arg1, sizeof(cmd.arg1), "%s", tok[1]);
  } else if (eqUpper(tok[0], "TESTMODE") && n == 2) {
    if (eqUpper(tok[1], "ON")) cmd.kind = UsbCmdKind::TESTMODE_ON;
    else if (eqUpper(tok[1], "OFF")) cmd.kind = UsbCmdKind::TESTMODE_OFF;
  } else if (eqUpper(tok[0], "INJECT") && n == 3) {
    cmd.kind = UsbCmdKind::INJECT;
    std::snprintf(cmd.arg1, sizeof(cmd.arg1), "%s", tok[1]);
    std::snprintf(cmd.arg2, sizeof(cmd.arg2), "%s", tok[2]);
  } else if (eqUpper(tok[0], "READ") && n == 4) {
    // READ <id> <offset> <length> (bounded 1..4096 bytes per chunk)
    uint32_t id = 0, off = 0, len = 0;
    if (parseUIntStrict(tok[1], id) && id > 0 &&
        parseUIntStrict(tok[2], off) &&
        parseUIntStrict(tok[3], len) && len >= 1 && len <= 4096) {
      cmd.kind = UsbCmdKind::READ_MEDIA;
      cmd.num1 = id;
      cmd.num2 = off;
      std::snprintf(cmd.arg1, sizeof(cmd.arg1), "%u", static_cast<unsigned>(len));
    }
  } else if (eqUpper(tok[0], "FORMAT") && n == 3 && eqUpper(tok[1], "MEDIA")) {
    cmd.kind = UsbCmdKind::FORMAT_MEDIA;
    std::snprintf(cmd.arg1, sizeof(cmd.arg1), "%s", tok[2]);  // exact case-sensitive label check!
  } else if (eqUpper(tok[0], "REBOOT") && n == 1) {
    cmd.kind = UsbCmdKind::REBOOT;
  }

  return cmd;
}

inline std::string encodeBase64(const uint8_t* data, size_t len) {
  static constexpr char kTab[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((len + 2) / 3) * 4);
  for (size_t i = 0; i < len; i += 3) {
    const uint32_t b0 = data[i];
    const uint32_t b1 = (i + 1 < len) ? data[i + 1] : 0;
    const uint32_t b2 = (i + 2 < len) ? data[i + 2] : 0;
    const uint32_t triple = (b0 << 16) | (b1 << 8) | b2;
    out.push_back(kTab[(triple >> 18) & 0x3F]);
    out.push_back(kTab[(triple >> 12) & 0x3F]);
    out.push_back((i + 1 < len) ? kTab[(triple >> 6) & 0x3F] : '=');
    out.push_back((i + 2 < len) ? kTab[triple & 0x3F] : '=');
  }
  return out;
}

}  // namespace flipcam
