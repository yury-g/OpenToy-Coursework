// SPDX-License-Identifier: MIT
#pragma once
#include "src/opentoy/DoodleProtocol.h"

inline uint16_t touchZone(int x, int width) {
  if (width <= 0 || x < 0 || x >= width) return 0;
  return 1u << ((x * 3 / width) * 4);
}

struct LiveFace {
  FaceState pose = restingDoodle();
  bool remote = false;
  bool touchEnabled = true;

  Result apply(const char *command) {
    if (!strcmp(command, "status")) return STATUS_QUERY;
    if (!strcmp(command, "local")) {
      remote = false;
      return ACCEPTED;
    }
    if (!strcmp(command, "reset")) {
      *this = LiveFace{};
      return ACCEPTED;
    }
    if (!strcmp(command, "touch on") || !strcmp(command, "touch off")) {
      touchEnabled = command[7] == 'n';
      return ACCEPTED;
    }
    if (strncmp(command, "doodle ", 7) != 0 || !parseDoodle(command + 7, pose)) {
      return INVALID;
    }
    remote = true;
    return ACCEPTED;
  }
};
