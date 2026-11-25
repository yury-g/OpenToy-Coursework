// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "../core/UnoQEyesApp.h"
#include "../core/UnoQTftAdapter.h"

using namespace opentoy::uno_q;

namespace {

int countLitPixels(const std::vector<uint16_t>& fb, int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      if (fb[size_t(y) * 320 + x] == kRgbInk) {
        ++n;
      }
    }
  }
  return n;
}

}  // namespace

int main(int argc, char** argv) {
  // 1. Default unbranded shield is strictly unqualified and refuses GPIO/SPI drive.
  ShieldDescriptor unbranded;
  assert(!isHardwareQualified(unbranded));

  NullUnqualifiedPanel nullPanel(unbranded);
  assert(!nullPanel.isQualified());
  assert(!nullPanel.initPanel());
  assert(!nullPanel.supportsTouch());
  std::vector<uint16_t> fb(320 * 240, 0);
  assert(!nullPanel.presentRgb565(fb.data(), 320, 240));

  // 2. 5V tolerance unknown must fail qualification even if controller & pin map set.
  ShieldDescriptor unsafe5v;
  unsafe5v.controller_ic = "ILI9341";
  unsafe5v.bus = ShieldBus::Spi4Wire;
  unsafe5v.logic_level = LogicLevel::ToleranceUnverified5V;
  unsafe5v.pin_map_verified = true;
  assert(!isHardwareQualified(unsafe5v));

  // 3. Qualified 3.3V shield passes hardware qualification metadata gate.
  ShieldDescriptor qualified = unsafe5v;
  qualified.logic_level = LogicLevel::Lv3v3;
  assert(isHardwareQualified(qualified));

  // 4. Truthful battery helper: USB-only / unqualified / stale always BAT --.
  char bat[12] = {};
  formatUnoQBattery(4100, 100, false, bat);
  assert(std::strcmp(bat, "BAT --") == 0);
  formatUnoQBattery(4100, 5001, true, bat);
  assert(std::strcmp(bat, "BAT --") == 0);
  formatUnoQBattery(1800, 100, true, bat);
  assert(std::strcmp(bat, "BAT --") == 0);
  formatUnoQBattery(3850, 100, true, bat);
  assert(std::strcmp(bat, "BAT 3.85V") == 0);

  // 5. Portable eyes rendering & behavior tests (large pupils, header, performance mode, idle motion).
  UnoQEyesApp app(unbranded, 1);
  auto pose0 = app.step(0);
  app.render(pose0, 0, fb);
  assert(app.headerVisible());
  assert(fb[0] == kRgbHeaderBg);  // default header visible

  // Eyes must draw into left & right eye halves cleanly with dark pupil center holes.
  const int left_lit = countLitPixels(fb, 20, 40, 150, 220);
  const int right_lit = countLitPixels(fb, 170, 40, 300, 220);
  assert(left_lit > 1200 && right_lit > 1200);
  assert(fb[135 * 320 + 80] == kRgbBg);   // left pupil hollow center
  assert(fb[135 * 320 + 240] == kRgbBg);  // right pupil hollow center

  // Unqualified touch must NOT trigger pokes.
  assert(!app.applyQualifiedTouch(80, 120, 100));

  // Performance mode hides header cleanly.
  app.setPerformanceMode(true);
  assert(!app.headerVisible());
  app.render(pose0, 0, fb);
  assert(fb[0] == kRgbBg);

  // Distinct personalities + autonomous idle evolution across 0..4000 ms.
  app.setPerformanceMode(false);
  std::vector<uint16_t> fb_later;
  bool changed = false;
  for (uint32_t t = 200; t <= 4000; t += 200) {
    auto p = app.step(t);
    app.render(p, t, fb_later);
    if (fb_later != fb) {
      changed = true;
      break;
    }
  }
  assert(changed);

  UnoQEyesApp sleepy(unbranded, 2);
  UnoQEyesApp cheeky(unbranded, 3);
  assert(sleepy.personality() == 2 && cheeky.personality() == 3);

  // Optional synthetic PPM dump mode for labelled documentation previews.
  if (argc == 3 && std::strcmp(argv[1], "--dump-ppm") == 0) {
    std::vector<uint16_t> out_fb;
    auto p = app.step(600);
    app.render(p, 600, out_fb);
    std::ofstream ofs(argv[2], std::ios::binary);
    ofs << "P6\n320 240\n255\n";
    for (uint16_t px : out_fb) {
      uint8_t r = uint8_t(((px >> 11) & 31) * 255 / 31);
      uint8_t g = uint8_t(((px >> 5) & 63) * 255 / 63);
      uint8_t b = uint8_t((px & 31) * 255 / 31);
      ofs.put(char(r));
      ofs.put(char(g));
      ofs.put(char(b));
    }
  }

  std::cout << "PASS UNO Q TFT unqualified boundary, truthful battery, large-pupil eyes, and personas\n";
  return 0;
}
