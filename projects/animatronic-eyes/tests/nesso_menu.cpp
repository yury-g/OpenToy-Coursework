// SPDX-License-Identifier: MIT
#include "../firmware/NessoEyes/NessoMenu.h"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace nesso_eyes;
using opentoy_tiny_ui::MenuPage;

namespace {

constexpr float kPi = 3.14159265358979323846f;

void tapViewPoint(NessoMenu& ui, const Transform& tr, float vx, float vy, uint32_t& now) {
  const Point s = tr.headerPoint(vx, vy);
  const int sx = int(std::lround(s.x));
  const int sy = int(std::lround(s.y));
  ui.update({false, -1, -1}, tr, now);
  now += 15;
  ui.update({true, sx, sy}, tr, now);
  now += 25;
  ui.update({false, sx, sy}, tr, now);
  now += 15;
}

}  // namespace

int main() {
  // 1. Verify HUD + Menu layout fit cleanly across all 360 rotation angles, including 45 deg diagonal.
  uint16_t hud[240 * 15] = {};
  uint16_t frame[240 * 135] = {};
  for (int deg = 0; deg < 360; deg += 15) {
    const Transform tr(float(deg) * kPi / 180.0f, true);
    const NessoLayout lay = NessoLayout::compute(tr);
    assert(lay.vw >= 88 && lay.vh >= 88);
    assert(lay.row0.y >= 15);
    assert(lay.row1.y + lay.row1.h <= lay.backBtn.y);
    assert(lay.backBtn.y + lay.backBtn.h <= lay.vh);

    // Unavailable battery (-1) renders neutral "BAT --" text, never an empty gauge box.
    renderNessoHudStrip(hud, lay.vw, -1, false, false);
    unsigned lit_row1 = 0, lit_row2 = 0;
    for (int x = 0; x < lay.vw; ++x) {
      for (int y = 1; y <= 5; ++y) if (hud[y * 240 + x]) ++lit_row1;
      for (int y = 7; y <= 13; ++y) if (hud[y * 240 + x]) ++lit_row2;
      // Nothing drawn outside [0, lay.vw)
    }
    assert(lit_row1 > 40 && lit_row2 > 40);
    for (int y = 0; y < 15; ++y) {
      for (int x = lay.vw; x < 240; ++x) {
        assert(hud[y * 240 + x] == 0);
      }
    }

    // Valid battery (82%) renders gauge + text cleanly.
    renderNessoHudStrip(hud, lay.vw, 82, false, false);
    assert(hud[2 * 240 + (lay.vw - 8)] != 0 || hud[1 * 240 + (lay.vw - 3)] != 0);

    // Full rotated Menu frame renders non-empty pixels inside safe screen bounds.
    NessoMenu menu;
    menu.openMenu();
    renderNessoMenuFrame(menu, 82, tr, frame, false);
    unsigned menu_lit = 0;
    for (int i = 0; i < 240 * 135; ++i) {
      if (frame[i]) ++menu_lit;
    }
    assert(menu_lit > 250);
  }

  // 2. Rotated touch Menu navigation (tested at 90-degree portrait orientation!), drag-out cancel,
  //    Performance mode reclaim, and 0.8s top-center safe-edge restore.
  {
    uint32_t now = 100;
    NessoMenu ui;
    const Transform tr90(kPi * 0.5f, true);
    const NessoLayout lay = NessoLayout::compute(tr90);

    // Dragging out of Menu button cancels without opening Menu.
    const Point mpt = tr90.headerPoint(float(lay.menuBtn.x + lay.menuBtn.w / 2), 7.0f);
    const Point out_pt = tr90.screen(0.0f, 0.0f);
    ui.update({true, int(std::lround(mpt.x)), int(std::lround(mpt.y))}, tr90, now);
    now += 20;
    ui.update({true, int(std::lround(out_pt.x)), int(std::lround(out_pt.y))}, tr90, now);
    now += 20;
    ui.update({false, int(std::lround(out_pt.x)), int(std::lround(out_pt.y))}, tr90, now);
    assert(ui.playing() && ui.header);

    // Tap Menu button cleanly -> opens Root page 1/4.
    tapViewPoint(ui, tr90, float(lay.menuBtn.x + lay.menuBtn.w / 2), 7.0f, now);
    assert(ui.page == MenuPage::Root && ui.rootPage == 0);

    // Navigate to Personality (page 2/4, row 0) and select Cheeky (subPage 1, row 1).
    tapViewPoint(ui, tr90, float(lay.nextBtn.x + lay.nextBtn.w / 2), float(lay.nextBtn.y + 8), now);
    assert(ui.rootPage == 1);
    tapViewPoint(ui, tr90, float(lay.row0.x + 20), float(lay.row0.y + 8), now);
    assert(ui.page == MenuPage::Personality && ui.subPage == 0);
    tapViewPoint(ui, tr90, float(lay.nextBtn.x + 12), float(lay.nextBtn.y + 8), now);
    assert(ui.subPage == 1);
    tapViewPoint(ui, tr90, float(lay.row1.x + 20), float(lay.row1.y + 8), now);
    assert(ui.persona == 3);
    tapViewPoint(ui, tr90, float(lay.backBtn.x + 12), float(lay.backBtn.y + 8), now);
    assert(ui.page == MenuPage::Root);

    // Navigate to Display (page 3/4, row 0) and activate Performance mode.
    ui.rootPage = 2;
    tapViewPoint(ui, tr90, float(lay.row0.x + 20), float(lay.row0.y + 8), now);
    assert(ui.page == MenuPage::Display);
    tapViewPoint(ui, tr90, float(lay.row0.x + 20), float(lay.row0.y + 8), now);
    assert(ui.playing() && !ui.header);

    // Performance mode disables header exclusion and keeps >= normal eye scale.
    const Transform tr_perf(kPi * 0.5f, false);
    assert(tr_perf.scale >= tr90.scale && tr_perf.headerRows == 0);
    assert(!tr_perf.header(mpt.x, mpt.y));

    // Short 400 ms touch in top-center restore zone does NOT exit Performance mode.
    const NessoLayout perf_lay = NessoLayout::compute(tr_perf);
    const Point rpt = tr_perf.headerPoint(
        float(perf_lay.restoreZone.x + perf_lay.restoreZone.w / 2), 10.0f);
    ui.update({true, int(std::lround(rpt.x)), int(std::lround(rpt.y))}, tr_perf, now);
    now += 400;
    ui.update({true, int(std::lround(rpt.x)), int(std::lround(rpt.y))}, tr_perf, now);
    assert(!ui.header && ui.playing());
    ui.update({false, int(std::lround(rpt.x)), int(std::lround(rpt.y))}, tr_perf, now);
    now += 30;

    // Deliberate >= 800 ms hold in top-center safe restore zone restores header and opens Menu!
    ui.update({true, int(std::lround(rpt.x)), int(std::lround(rpt.y))}, tr_perf, now);
    now += 820;
    ui.update({true, int(std::lround(rpt.x)), int(std::lround(rpt.y))}, tr_perf, now);
    assert(ui.header && ui.page == MenuPage::Root);
  }

  std::puts("PASS Nesso 1.4.0 360-deg status bar, rotated touch Menu, drag cancel, and 0.8s Performance restore");
  return 0;
}
