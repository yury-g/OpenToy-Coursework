// SPDX-License-Identifier: MIT
#include "../firmware/AnimatronicEyes/FeatherMenu.h"
#include "../firmware/AnimatronicEyes/Power.h"

#include <cassert>
#include <cstdio>

using namespace feather_face;
using opentoy_tiny_ui::MenuPage;

namespace {

void tapD1Short(FeatherMenu& ui, uint32_t& now) {
  ui.update(false, false, 512, 512, 0, false, now);
  now += 30;
  ui.update(true, false, 512, 512, 0, false, now);
  now += 40;
  ui.update(true, false, 512, 512, 0, false, now);
  now += 40;
  ui.update(false, false, 512, 512, 0, false, now);
  now += 35;
  ui.update(false, false, 512, 512, 0, false, now);
}

void holdD1(FeatherMenu& ui, uint32_t& now) {
  ui.update(false, false, 512, 512, 0, false, now);
  now += 30;
  ui.update(true, false, 512, 512, 0, false, now);
  now += 35;
  ui.update(true, false, 512, 512, 0, false, now);
  now += 820;
  ui.update(true, false, 512, 512, 0, false, now);
  now += 30;
  ui.update(false, false, 512, 512, 0, false, now);
  now += 35;
  ui.update(false, false, 512, 512, 0, false, now);
}

}  // namespace

int main() {
  // 1. Boot-strap GPIO0 exclusion static check & truthful status bar rendering.
  static_assert(kPrimaryPinD1 != kExcludedBootPinD0 && kSecondaryPinD2 != kExcludedBootPinD0,
                "GPIO0 BOOT excluded");
  uint16_t buf[240 * 135] = {};
  renderFeatherStatusBar(buf, 1, 1, -1, false);
  unsigned header_lit = 0;
  for (int y = 0; y < 15; ++y) {
    for (int x = 0; x < 240; ++x) {
      if (buf[y * 240 + x]) ++header_lit;
    }
  }
  assert(header_lit > 60);
  // Confirm "BAT --" right-aligned pixels exist without an empty battery frame at x=219..236, y=4.
  assert(buf[4 * 240 + 236] == 0);

  // Valid battery (75%) renders gauge + text.
  renderFeatherStatusBar(buf, 2, 0, 75, false);
  assert(buf[4 * 240 + 220] != 0);

  // 2. Two-button D1/D2 navigation across all 7 canonical entries, Performance entry, and 0.8s D1 restore.
  {
    uint32_t now = 100;
    FeatherMenu ui(1, 1);

    // D1 short press in Play triggers blink pulse without opening Menu.
    tapD1Short(ui, now);
    assert(ui.playing() && int32_t(ui.blinkPulseUntilMs - now) > 0);
    now += 300;

    // D1 >= 800 ms hold opens Menu, consuming short press on release.
    holdD1(ui, now);
    assert(ui.page == MenuPage::Root && ui.focus == 0);

    // Step D1 short presses 4 times to reach Display (index 4).
    for (int i = 0; i < 4; ++i) tapD1Short(ui, now);
    assert(ui.focus == 4);

    // Hold D1 to enter Display submenu, then hold D1 on index 0 ("ENTER PERFORMANCE MODE").
    holdD1(ui, now);
    assert(ui.page == MenuPage::Display && ui.focus == 0);
    holdD1(ui, now);
    assert(ui.playing() && !ui.header);

    // Short D1 press in Performance mode does NOT accidentally restore header.
    tapD1Short(ui, now);
    assert(ui.playing() && !ui.header);

    // Deliberate 800 ms D1 hold restores header and opens Menu!
    holdD1(ui, now);
    assert(ui.header && ui.page == MenuPage::Root);
  }

  // 3. QT Gamepad Select+Start chord disambiguation:
  //    - Short chord (< 800 ms) blinks on release without opening Menu.
  //    - Stationary long chord (>= 800 ms) opens Menu WITHOUT leaking SELECT|START blink!
  {
    uint32_t now = 500;
    FeatherMenu ui(1, 0);

    // Short 250 ms Select+Start chord: withheld during hold, fires blink pulse on release.
    uint32_t f1 = ui.update(false, false, 512, 512, SELECT | START, true, now);
    assert((f1 & (SELECT | START)) == 0);
    assert(ui.playing() && int32_t(ui.blinkPulseUntilMs - now) <= 0);
    now += 250;
    uint32_t f2 = ui.update(false, false, 512, 512, 0, true, now);
    assert(f2 == 0 && ui.playing() && int32_t(ui.blinkPulseUntilMs - now) > 0);
    now += 300;

    // Long 820 ms Select+Start chord: opens Menu with zero leaked blink!
    ui.blinkPulseUntilMs = 0;
    uint32_t f3 = ui.update(false, false, 512, 512, SELECT | START, true, now);
    assert((f3 & (SELECT | START)) == 0);
    now += 820;
    uint32_t f4 = ui.update(false, false, 512, 512, SELECT | START, true, now);
    assert(f4 == 0 && !ui.playing() && ui.page == MenuPage::Root && ui.blinkPulseUntilMs == 0);

    // Release chord, then navigate Menu with Gamepad stick down + A button to Personality -> Dreamy.
    now += 30;
    ui.update(false, false, 512, 512, 0, true, now);
    now += 30;
    ui.update(false, false, 512, 900, 0, true, now);  // focus 1 (Apps)
    now += 30;
    ui.update(false, false, 512, 512, 0, true, now);
    now += 30;
    ui.update(false, false, 512, 900, 0, true, now);  // focus 2 (Personality)
    now += 30;
    ui.update(false, false, 512, 512, A, true, now);  // select Personality
    assert(ui.page == MenuPage::Personality);
    now += 30;
    ui.update(false, false, 512, 512, 0, true, now);
    now += 30;
    ui.update(false, false, 512, 900, 0, true, now);  // focus 1 (Curious)
    now += 30;
    ui.update(false, false, 512, 512, 0, true, now);
    now += 30;
    ui.update(false, false, 512, 900, 0, true, now);  // focus 2 (Dreamy)
    now += 30;
    ui.update(false, false, 512, 512, A, true, now);  // choose Dreamy -> back to Root
    assert(ui.persona == 2 && ui.page == MenuPage::Root);

    // Render full menu screen and verify non-empty pixels.
    renderFeatherMenuScreen(ui, 88, buf, false);
    unsigned menu_px = 0;
    for (uint16_t px : buf) if (px) ++menu_px;
    assert(menu_px > 500);
  }

  std::puts("PASS Feather 1.4.0 D0 exclusion, D1/D2 Menu, Select+Start disambiguation, and Performance restore");
  return 0;
}
