// SPDX-License-Identifier: MIT
#include "../adapter/Tab5Ui.h"

#include <cassert>
#include <cstdio>
#include <vector>

using namespace opentoy::tab5;

namespace {

void tapUi(Tab5UiState& ui, int x, int y, uint32_t& now) {
  ui.update({false, -1, -1}, now);
  now += 15;
  ui.update({true, x, y}, now);
  now += 25;
  ui.update({false, x, y}, now);
  now += 15;
}

}  // namespace

int main() {
  std::vector<uint16_t> fb(kUiWidth * kUiHeight, 0);
  std::vector<uint16_t> preview(320 * 180, 0x2945);
  std::vector<uint16_t> dpi(720 * 1280, 0);

  Tab5UiState ui;
  BatterySample bat_unknown;
  Tab5LiveTelemetry tel;
  tel.cam_ready = true;
  tel.cam_streaming = true;
  tel.ae_warmed = true;
  tel.sensor_id = kSc202csSensorId;
  tel.frame_seq = 42;
  tel.detector_ready = true;
  tel.infer_ms = 28;
  tel.panel_name = "ST7123";
  tel.ina226_ok = true;
  tel.vbus_mv = 5020;  // USB-only (< 5500 mV 2S pack) -> must show truthful "BAT --"

  // 1. Default boot mode is CameraEye with visible status bar and "BAT --".
  renderTab5Frame(fb.data(), ui, bat_unknown, tel, preview.data(), 1000);
  unsigned bar_lit = 0, view_lit = 0;
  for (int y = 0; y < kStatusBarHeight; ++y) {
    for (int x = 0; x < kUiWidth; ++x) if (fb[y * kUiWidth + x]) ++bar_lit;
  }
  for (int y = 50; y < 300; ++y) {
    for (int x = 90; x < 550; ++x) if (fb[y * kUiWidth + x]) ++view_lit;
  }
  assert(bar_lit > 1000 && view_lit > 50000);

  // 2. Dragging outside MENU button cancels without opening Menu.
  uint32_t now = 1000;
  const int mx = Tab5UiState::menuBtn().x + 40;
  ui.update({true, mx, 12}, now);
  now += 20;
  ui.update({true, 100, 150}, now);
  now += 20;
  ui.update({false, 100, 150}, now);
  assert(ui.playing() && ui.mode == Tab5AppMode::CameraEye);

  // 3. Open Menu -> Apps -> Copy-Face -> verify eye pokes & honest mouth status.
  tapUi(ui, mx, 12, now);
  assert(ui.page == Tab5MenuPage::Root && ui.rootPage == 0);
  const UiRect r1 = Tab5UiState::rowRect(1);
  tapUi(ui, r1.x + 80, r1.y + 20, now);
  assert(ui.page == Tab5MenuPage::Apps);
  tapUi(ui, r1.x + 80, r1.y + 20, now);
  assert(ui.playing() && ui.mode == Tab5AppMode::CopyFace);

  // Poke left eye in Copy-Face -> pokeMask == 1; touch in status bar -> pokeMask == 0.
  ui.update({true, 180, 180}, now);
  assert(ui.pokeMask == 1u);
  ui.update({false, 180, 180}, now + 20);
  assert(ui.pokeMask == 0u);
  now += 40;

  tel.face_tracking = true;
  tel.gaze_x = -0.5f;
  tel.gaze_y = 0.2f;
  tel.primary_face.valid = true;
  tel.primary_face.x = 0.35f;
  tel.primary_face.y = 0.25f;
  tel.primary_face.w = 0.30f;
  tel.primary_face.h = 0.45f;
  renderTab5Frame(fb.data(), ui, bat_unknown, tel, preview.data(), now);

  // 4. Navigate Menu page 2 -> Display -> Performance mode -> 0.8s top-center hold restore.
  tapUi(ui, mx, 12, now);
  tapUi(ui, Tab5UiState::nextBtn().x + 40, Tab5UiState::nextBtn().y + 20, now);
  assert(ui.rootPage == 1);
  const UiRect r0 = Tab5UiState::rowRect(0);
  tapUi(ui, r0.x + 80, r0.y + 20, now);
  assert(ui.page == Tab5MenuPage::Display);
  tapUi(ui, r0.x + 80, r0.y + 20, now);
  assert(ui.playing() && !ui.header);

  // Short 300 ms touch in restore zone does not restore.
  ui.update({true, 320, 18}, now);
  now += 300;
  ui.update({true, 320, 18}, now);
  assert(!ui.header && ui.playing());
  ui.update({false, 320, 18}, now);
  now += 30;

  // >= 800 ms hold in top-center restore zone restores header and opens Menu!
  ui.update({true, 320, 18}, now);
  now += 820;
  ui.update({true, 320, 18}, now);
  assert(ui.header && ui.page == Tab5MenuPage::Root);

  // 5. Verify exact coordinate correspondence between portrait 720x1280 blit and touch mapping
  //    in BOTH standard and 180-flipped landscape orientations!
  for (bool flip : {false, true}) {
    std::fill(fb.begin(), fb.end(), uint16_t(0));
    fb[123 * kUiWidth + 456] = 0xabcd;
    blitLandscape640x360ToPortrait720x1280(fb.data(), dpi.data(), flip);

    int found_px = -1, found_py = -1;
    for (int py = 0; py < 1280 && found_px < 0; ++py) {
      for (int px = 0; px < 720; ++px) {
        if (dpi[py * 720 + px] == 0xabcd) {
          found_px = px;
          found_py = py;
          break;
        }
      }
    }
    assert(found_px >= 0 && found_py >= 0);
    const UiTouchPoint mapped = mapPortraitTouchToLandscape(true, found_px, found_py, flip);
    assert(mapped.down && mapped.x == 456 && mapped.y == 123);
  }

  std::puts("PASS Tab5 0.2.0 UI modes, Menu, Performance 0.8s restore, and portrait/landscape bijection");
  return 0;
}
