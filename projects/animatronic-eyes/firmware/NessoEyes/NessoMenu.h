// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "Metadata.h"
#include "Presentation.h"
#if __has_include("Companion.h")
#include "Companion.h"
#else
#include "../AnimatronicEyes/Companion.h"
#endif
#if __has_include("TinyFont3x5.h")
#include "TinyFont3x5.h"
#else
#include "../../shared/TinyFont3x5.h"
#endif

namespace nesso_eyes {

struct ViewRect {
  int x = 0, y = 0, w = 0, h = 0;
  bool contains(float px, float py) const {
    return px >= float(x) && px < float(x + w) && py >= float(y) && py < float(y + h);
  }
};

struct NessoTouch {
  bool down = false;
  int x = -1;  // Screen [0..240)
  int y = -1;  // Screen [0..135)
};

enum class NessoControlId : int {
  None = -1,
  MenuBtn = 1,
  Row0 = 2,
  Row1 = 3,
  BackBtn = 4,
  NextBtn = 5,
  RestoreZone = 6,
};

struct NessoLayout {
  int vw = 234;
  int vh = 129;
  ViewRect menuBtn;
  ViewRect restoreZone;
  ViewRect row0;
  ViewRect row1;
  ViewRect backBtn;
  ViewRect nextBtn;

  static NessoLayout compute(const Transform& tr) {
    NessoLayout l;
    l.vw = std::max(88, int(std::lround(tr.viewWidth)));
    l.vh = std::max(88, int(std::lround(tr.viewHeight)));
    l.menuBtn = {l.vw - 32, 0, 32, 15};
    const int rw = std::max(44, (l.vw * 3) / 5);
    l.restoreZone = {(l.vw - rw) / 2, 0, rw, 24};

    const int top = 26;
    const int nav_h = std::max(16, std::min(22, (l.vh - top) / 4));
    const int avail = std::max(34, l.vh - top - nav_h - 6);
    const int row_h = std::max(16, std::min(24, (avail - 3) / 2));
    const int pad_x = 4;
    const int full_w = l.vw - 2 * pad_x;
    l.row0 = {pad_x, top, full_w, row_h};
    l.row1 = {pad_x, top + row_h + 3, full_w, row_h};
    const int nav_y = l.vh - nav_h - 2;
    const int half_w = (full_w - 4) / 2;
    l.backBtn = {pad_x, nav_y, half_w, nav_h};
    l.nextBtn = {pad_x + half_w + 4, nav_y, full_w - half_w - 4, nav_h};
    return l;
  }
};

class NessoMenu {
  int captured_ = int(NessoControlId::None);
  bool holdingRestore_ = false;
  bool waitRelease_ = false;
  uint32_t holdStart_ = 0;

  void transition() {
    ++epoch;
    pokeMask = 0;
    captured_ = int(NessoControlId::None);
    holdingRestore_ = false;
    waitRelease_ = true;
  }

  void activateRow(unsigned idx) {
    using opentoy_tiny_ui::MenuPage;
    if (page == MenuPage::Root) {
      const unsigned entry = rootPage * 2u + idx;
      if (entry >= 7u) return;
      switch (entry) {
        case 0: page = MenuPage::Play; break;
        case 1: page = MenuPage::Apps; break;
        case 2: page = MenuPage::Personality; subPage = 0; break;
        case 3: page = MenuPage::Sound; break;
        case 4: page = MenuPage::Display; break;
        case 5: page = MenuPage::Controls; break;
        case 6: page = MenuPage::About; break;
      }
      transition();
      return;
    }
    if (page == MenuPage::Personality) {
      const unsigned pid = subPage * 2u + idx;
      if (pid < 4u) {
        persona = pid;
        ++epoch;
      }
      return;
    }
    if (page == MenuPage::Display) {
      if (idx == 0) {
        // Enter session Performance mode: hides status bar and returns to clean Play.
        header = false;
        page = MenuPage::Play;
        transition();
      } else if (idx == 1) {
        brightness = brightness ? 0u : 1u;
        ++epoch;
      }
      return;
    }
    if (page == MenuPage::Apps && idx == 0) {
      page = MenuPage::Play;
      transition();
    }
  }

 public:
  opentoy_tiny_ui::MenuPage page = opentoy_tiny_ui::MenuPage::Play;
  unsigned rootPage = 0;    // 0..3 (2 items per page across 7 canonical entries)
  unsigned subPage = 0;     // 0..1 for Personality (Classic, Curious / Dreamy, Cheeky)
  unsigned persona = 0;     // 0=Classic, 1=Curious, 2=Dreamy, 3=Cheeky
  unsigned brightness = 1;  // 1=Normal (115), 0=Dim (45)
  bool header = true;       // Default ON after reboot; hidden in Performance mode
  unsigned pokeMask = 0;
  uint32_t epoch = 1;

  bool playing() const { return page == opentoy_tiny_ui::MenuPage::Play; }
  int pressedId() const { return captured_; }

  void openMenu() {
    header = true;
    page = opentoy_tiny_ui::MenuPage::Root;
    rootPage = 0;
    subPage = 0;
    transition();
  }

  void togglePerformance() {
    if (playing() && header) {
      header = false;
      transition();
    } else {
      openMenu();
    }
  }

  void back() {
    using opentoy_tiny_ui::MenuPage;
    if (page == MenuPage::Root) {
      page = MenuPage::Play;
      transition();
    } else if (page != MenuPage::Play) {
      page = MenuPage::Root;
      transition();
    }
  }

  void nextPage() {
    using opentoy_tiny_ui::MenuPage;
    if (page == MenuPage::Root) {
      rootPage = (rootPage + 1u) % 4u;
      ++epoch;
    } else if (page == MenuPage::Personality) {
      subPage = (subPage + 1u) % 2u;
      ++epoch;
    }
  }

  void setPersona(unsigned id) {
    if (id < 4u && persona != id) {
      persona = id;
      ++epoch;
    }
  }

  void loseInput() {
    pokeMask = 0;
    captured_ = int(NessoControlId::None);
    holdingRestore_ = false;
    waitRelease_ = false;
  }

  int hitControl(const Transform& tr, float sx, float sy) const {
    if (!tr.inViewport(sx, sy)) return int(NessoControlId::None);
    const Point v = tr.viewCoords(sx, sy);
    const NessoLayout lay = NessoLayout::compute(tr);

    if (playing()) {
      if (!header) {
        return lay.restoreZone.contains(v.x, v.y) ? int(NessoControlId::RestoreZone)
                                                  : int(NessoControlId::None);
      }
      return lay.menuBtn.contains(v.x, v.y) ? int(NessoControlId::MenuBtn)
                                            : int(NessoControlId::None);
    }

    if (lay.backBtn.contains(v.x, v.y)) return int(NessoControlId::BackBtn);
    if ((page == opentoy_tiny_ui::MenuPage::Root ||
         page == opentoy_tiny_ui::MenuPage::Personality) &&
        lay.nextBtn.contains(v.x, v.y)) {
      return int(NessoControlId::NextBtn);
    }
    if (lay.row0.contains(v.x, v.y)) return int(NessoControlId::Row0);
    if (lay.row1.contains(v.x, v.y)) {
      if (page == opentoy_tiny_ui::MenuPage::Root && (rootPage * 2u + 1u) >= 7u) {
        return int(NessoControlId::None);
      }
      return int(NessoControlId::Row1);
    }
    return int(NessoControlId::None);
  }

  void update(const NessoTouch& t, const Transform& tr, uint32_t now, bool valid = true) {
    if (!valid) {
      loseInput();
      return;
    }
    if (waitRelease_) {
      if (!t.down) waitRelease_ = false;
      pokeMask = 0;
      return;
    }

    if (playing()) {
      if (!header) {
        // Performance mode: top-center 0.8s hold restores status bar and opens Menu.
        const bool in_restore =
            t.down && (hitControl(tr, float(t.x), float(t.y)) == int(NessoControlId::RestoreZone));
        if (in_restore) {
          if (!holdingRestore_) {
            holdingRestore_ = true;
            holdStart_ = now;
            pokeMask = 0;
            return;
          }
          if (uint32_t(now - holdStart_) >= 800u) {
            openMenu();
            return;
          }
          pokeMask = 0;
          return;
        }
        holdingRestore_ = false;
      } else {
        // Visible Menu button in safe header: require press + release inside button.
        const int hit = t.down ? hitControl(tr, float(t.x), float(t.y)) : int(NessoControlId::None);
        if (captured_ == int(NessoControlId::MenuBtn)) {
          pokeMask = 0;
          if (!t.down) {
            // Activate if last position before release stayed inside MenuBtn (or if caller passed release coords).
            if (t.x >= 0 && t.y >= 0 &&
                hitControl(tr, float(t.x), float(t.y)) == int(NessoControlId::MenuBtn)) {
              openMenu();
            } else {
              captured_ = int(NessoControlId::None);
            }
          } else if (hit != int(NessoControlId::MenuBtn)) {
            captured_ = int(NessoControlId::None);
            waitRelease_ = true;
          }
          return;
        }
        if (t.down && hit == int(NessoControlId::MenuBtn)) {
          captured_ = int(NessoControlId::MenuBtn);
          pokeMask = 0;
          return;
        }
      }

      // Normal eye pokes outside header.
      if (t.down && tr.face(float(t.x), float(t.y))) {
        const unsigned e = tr.eye(float(t.x), float(t.y));
        pokeMask = e ? e : 3u;  // Face poke outside single eye angers both, matching Nesso 1.3.2.
      } else {
        pokeMask = 0;
      }
      return;
    }

    // Menu open: press + release inside same control activates; drag-out cancels.
    pokeMask = 0;
    if (t.down) {
      const int hit = hitControl(tr, float(t.x), float(t.y));
      if (captured_ == int(NessoControlId::None)) {
        captured_ = hit;
        if (captured_ == int(NessoControlId::None)) {
          waitRelease_ = true;
        }
      } else if (hit != captured_) {
        captured_ = int(NessoControlId::None);
        waitRelease_ = true;
      }
      return;
    }

    if (captured_ != int(NessoControlId::None)) {
      const int act = captured_;
      captured_ = int(NessoControlId::None);
      if (t.x >= 0 && t.y >= 0 && hitControl(tr, float(t.x), float(t.y)) != act) {
        return;
      }
      switch (NessoControlId(act)) {
        case NessoControlId::BackBtn: back(); break;
        case NessoControlId::NextBtn: nextPage(); break;
        case NessoControlId::Row0: activateRow(0); break;
        case NessoControlId::Row1: activateRow(1); break;
        default: break;
      }
    }
  }
};

inline uint16_t encodeRgb565(uint16_t c, bool swap_bytes) {
  return swap_bytes ? uint16_t((c >> 8) | (c << 8)) : c;
}

// Renders the 2-row 15-pixel upright HUD strip (width = hudWidth <= 240, buffer stride = 240).
// Fits all 4 required APP-STATUS-BAR.md fields + [MENU] tap target at every angle 0..360 deg.
inline void renderNessoHudStrip(uint16_t* hud240x15,
                                int hud_w,
                                int battery_pct,
                                bool menu_pressed,
                                bool swap_bytes = false) {
  if (!hud240x15) return;
  std::fill(hud240x15, hud240x15 + 240 * 15, uint16_t(0));
  hud_w = std::max(88, std::min(240, hud_w));

  const uint16_t cyan = encodeRgb565(0x07ff, swap_bytes);
  const uint16_t silver = encodeRgb565(0xbefb, swap_bytes);
  const uint16_t green = encodeRgb565(0x76f7, swap_bytes);
  const uint16_t red = encodeRgb565(0xec2c, swap_bytes);
  const uint16_t dark = encodeRgb565(0x1082, swap_bytes);
  const uint16_t black = 0x0000;

  char id_line[24] = {};
  std::snprintf(id_line, sizeof(id_line), "%s V%s", appName, version);
  opentoy_tiny_ui::drawText3x5(hud240x15, 240, 15, 1, 1, id_line, cyan, 1, hud_w);
  opentoy_tiny_ui::drawText3x5(hud240x15, 240, 15, 1, 8, created, silver, 1, hud_w);

  // Row 1 right-aligned truthful battery: neutral "BAT --" when unavailable, else XX% + mini gauge.
  char bat_txt[10] = {};
  opentoy_tiny_ui::formatGaugeBatteryText(battery_pct, bat_txt);
  if (battery_pct < 0 || battery_pct > 100) {
    const int bw = opentoy_tiny_ui::textWidth3x5(bat_txt, 1);
    opentoy_tiny_ui::drawText3x5(hud240x15, 240, 15, hud_w - bw - 1, 1, bat_txt, silver, 1, hud_w);
  } else {
    const int icon_w = 10;
    const int icon_x = hud_w - icon_w - 2;
    opentoy_tiny_ui::drawRectBuf(hud240x15, 240, 15, icon_x, 1, icon_w, 5, silver);
    opentoy_tiny_ui::fillRectBuf(hud240x15, 240, 15, icon_x + icon_w, 2, 1, 3, silver);
    const int fill_w = ((icon_w - 2) * battery_pct) / 100;
    if (fill_w > 0) {
      opentoy_tiny_ui::fillRectBuf(
          hud240x15, 240, 15, icon_x + 1, 2, fill_w, 3, battery_pct <= 15 ? red : green);
    }
    const int tw = opentoy_tiny_ui::textWidth3x5(bat_txt, 1);
    opentoy_tiny_ui::drawText3x5(
        hud240x15, 240, 15, icon_x - tw - 2, 1, bat_txt, silver, 1, hud_w);
  }

  // Row 2 right-aligned [MENU] button (24x7 inside the 32x15 hit target).
  const int btn_w = 22;
  const int btn_x = hud_w - btn_w - 1;
  opentoy_tiny_ui::fillRectBuf(
      hud240x15, 240, 15, btn_x, 7, btn_w, 7, menu_pressed ? cyan : dark);
  opentoy_tiny_ui::drawRectBuf(hud240x15, 240, 15, btn_x, 7, btn_w, 7, cyan);
  opentoy_tiny_ui::drawText3x5(
      hud240x15, 240, 15, btn_x + 3, 8, "MENU", menu_pressed ? black : cyan, 1, hud_w);
}

// Renders the full gravity-rotated Menu screen into a 240x135 RGB565 buffer.
inline void renderNessoMenuFrame(const NessoMenu& ui,
                                 int battery_pct,
                                 const Transform& tr,
                                 uint16_t* out240x135,
                                 bool swap_bytes = false) {
  if (!out240x135) return;
  const NessoLayout lay = NessoLayout::compute(tr);

  // Build upright viewport buffer (max 240x240 fits on stack/static cleanly).
  static uint16_t view_buf[240 * 240];
  std::fill(view_buf, view_buf + lay.vw * lay.vh, uint16_t(0));

  // Draw top 15-px header into view_buf via a 240x15 row scratch.
  uint16_t hud_scratch[240 * 15] = {};
  renderNessoHudStrip(hud_scratch, lay.vw, battery_pct, false, swap_bytes);
  for (int y = 0; y < 15 && y < lay.vh; ++y) {
    for (int x = 0; x < lay.vw; ++x) {
      view_buf[y * lay.vw + x] = hud_scratch[y * 240 + x];
    }
  }

  const uint16_t cyan = encodeRgb565(0x07ff, swap_bytes);
  const uint16_t white = encodeRgb565(0xffff, swap_bytes);
  const uint16_t silver = encodeRgb565(0xbefb, swap_bytes);
  const uint16_t dark = encodeRgb565(0x1082, swap_bytes);
  const uint16_t black = 0x0000;

  auto drawButton = [&](const ViewRect& r, const char* label, bool pressed, bool selected = false) {
    if (!label || !label[0]) return;
    opentoy_tiny_ui::fillRectBuf(view_buf, lay.vw, lay.vh, r.x, r.y, r.w, r.h, pressed ? cyan : dark);
    opentoy_tiny_ui::drawRectBuf(
        view_buf, lay.vw, lay.vh, r.x, r.y, r.w, r.h, (pressed || selected) ? white : silver);
    const int ty = r.y + (r.h - 5) / 2;
    int tx = r.x + 4;
    if (selected) {
      opentoy_tiny_ui::fillRectBuf(
          view_buf, lay.vw, lay.vh, r.x + 2, ty + 1, 2, 3, pressed ? black : cyan);
      tx += 3;
    }
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, tx, ty, label, pressed ? black : white, 1, r.x + r.w - 2);
  };

  using opentoy_tiny_ui::MenuPage;
  char title[32] = {};
  if (ui.page == MenuPage::Root) {
    std::snprintf(title, sizeof(title), "MENU %u/4", ui.rootPage + 1u);
    const unsigned i0 = ui.rootPage * 2u;
    const unsigned i1 = i0 + 1u;
    drawButton(lay.row0,
               opentoy_tiny_ui::rootEntryLabel(i0),
               ui.pressedId() == int(NessoControlId::Row0),
               i0 == 0);
    if (i1 < 7u) {
      drawButton(lay.row1,
                 opentoy_tiny_ui::rootEntryLabel(i1),
                 ui.pressedId() == int(NessoControlId::Row1),
                 false);
    }
  } else if (ui.page == MenuPage::Apps) {
    std::snprintf(title, sizeof(title), "APPS");
    drawButton(lay.row0, "EYES1 ACTIVE", ui.pressedId() == int(NessoControlId::Row0), true);
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row1.x, lay.row1.y + 4, "MORE APPS NEED INSTALL", silver, 1, lay.vw - 2);
  } else if (ui.page == MenuPage::Personality) {
    std::snprintf(title, sizeof(title), "PERSONA %u/2", ui.subPage + 1u);
    const unsigned p0 = ui.subPage * 2u;
    const unsigned p1 = p0 + 1u;
    drawButton(lay.row0,
               feather_face::personality(p0).name,
               ui.pressedId() == int(NessoControlId::Row0),
               ui.persona == p0);
    drawButton(lay.row1,
               feather_face::personality(p1).name,
               ui.pressedId() == int(NessoControlId::Row1),
               ui.persona == p1);
  } else if (ui.page == MenuPage::Sound) {
    std::snprintf(title, sizeof(title), "SOUND");
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row0.x, lay.row0.y + 2, "MIC/SPK UNQUALIFIED", silver, 1, lay.vw - 2);
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row1.x, lay.row1.y + 2, "EYES1 SILENT ON NESSO", silver, 1, lay.vw - 2);
  } else if (ui.page == MenuPage::Display) {
    std::snprintf(title, sizeof(title), "DISPLAY");
    drawButton(lay.row0, "PERFORMANCE", ui.pressedId() == int(NessoControlId::Row0), false);
    drawButton(lay.row1,
               ui.brightness ? "LIGHT: NORMAL" : "LIGHT: DIM",
               ui.pressedId() == int(NessoControlId::Row1),
               true);
  } else if (ui.page == MenuPage::Controls) {
    std::snprintf(title, sizeof(title), "CONTROLS");
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row0.x, lay.row0.y + 2, "POKE EYES / TAP MENU", silver, 1, lay.vw - 2);
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row1.x, lay.row1.y + 2, "HOLD TOP 0.8S RESTORE", silver, 1, lay.vw - 2);
  } else if (ui.page == MenuPage::About) {
    std::snprintf(title, sizeof(title), "ABOUT");
    char l0[32] = {};
    std::snprintf(l0, sizeof(l0), "%s V%s %s", appName, version, created);
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row0.x, lay.row0.y + 2, l0, white, 1, lay.vw - 2);
    opentoy_tiny_ui::drawText3x5(
        view_buf, lay.vw, lay.vh, lay.row1.x, lay.row1.y + 2, "NESSO N1 ROTATING TOUCH", silver, 1, lay.vw - 2);
  }

  opentoy_tiny_ui::drawText3x5(view_buf, lay.vw, lay.vh, 4, 18, title, cyan, 1, lay.vw - 2);
  drawButton(lay.backBtn, "BACK", ui.pressedId() == int(NessoControlId::BackBtn), false);
  if (ui.page == MenuPage::Root || ui.page == MenuPage::Personality) {
    drawButton(lay.nextBtn, "NEXT", ui.pressedId() == int(NessoControlId::NextBtn), false);
  }

  // Inverse-rasterize view_buf onto 240x135 landscape screen buffer using 16.16 fixed-point.
  const int32_t hxStep = int32_t(tr.c * 65536.0f);
  const int32_t hyStep = int32_t(-tr.s * 65536.0f);
  for (int y = 0; y < 135; ++y) {
    const Point p = tr.local(0.5f, float(y) + 0.5f);
    int32_t vx = int32_t((p.x + tr.viewWidth * 0.5f) * 65536.0f);
    int32_t vy = int32_t((p.y + tr.viewHeight * 0.5f) * 65536.0f);
    for (int x = 0; x < 240; ++x, vx += hxStep, vy += hyStep) {
      const int ix = vx >> 16;
      const int iy = vy >> 16;
      uint16_t px = 0;
      if (ix >= 0 && ix < lay.vw && iy >= 0 && iy < lay.vh) {
        px = view_buf[iy * lay.vw + ix];
      }
      out240x135[y * 240 + x] = px;
    }
  }
}

}  // namespace nesso_eyes
