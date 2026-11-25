// SPDX-License-Identifier: MIT
#include "../core/BatteryStatus.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

int main() {
  using copy_face::formatBatteryBadge;
  char badge[16] = {};

  // Unqualified / missing / stale / out-of-range must render neutral BAT --.
  formatBatteryBadge(-1, 0, false, badge);
  assert(std::strcmp(badge, "BAT --") == 0);

  formatBatteryBadge(4100, 100, false, badge);
  assert(std::strcmp(badge, "BAT --") == 0);

  formatBatteryBadge(3850, 5001, true, badge);
  assert(std::strcmp(badge, "BAT --") == 0);

  formatBatteryBadge(1200, 100, true, badge);
  assert(std::strcmp(badge, "BAT --") == 0);

  // Qualified fresh voltage renders X.XXV without inventing percent or charging.
  formatBatteryBadge(3854, 250, true, badge);
  assert(std::strcmp(badge, "BAT 3.85V") == 0);

  // Ensure CrowPanel Copy-Face firmware App.cpp does NOT use misleading LV_SYMBOL_BATTERY_EMPTY.
  std::string this_file(__FILE__);
  const size_t pos = this_file.rfind("/tests/");
  assert(pos != std::string::npos);
  const std::string app_path = this_file.substr(0, pos) + "/firmware/main/App.cpp";
  std::ifstream ifs(app_path);
  assert(ifs.good());
  std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  assert(contents.find("LV_SYMBOL_BATTERY_EMPTY") == std::string::npos);
  assert(contents.find("formatBatteryBadge") != std::string::npos);

  std::puts("PASS Copy-Face truthful neutral battery badge & zero empty-battery icon regression");
  return 0;
}
