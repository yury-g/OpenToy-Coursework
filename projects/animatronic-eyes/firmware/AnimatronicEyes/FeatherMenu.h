// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "Controls.h"
#include "Metadata.h"
#include "Personality.h"
#if __has_include("TinyFont3x5.h")
#include "TinyFont3x5.h"
#else
#include "../../shared/TinyFont3x5.h"
#endif

namespace feather_face {

// Adafruit ESP32-S3 Reverse TFT Feather (#5691) button qualification (CONTROLS.md):
// - D0 (GPIO0) is the ESP32-S3 BOOT strapping pin -> STRICTLY EXCLUDED from normal controls.
// - D1 (GPIO1, active-high) -> Primary user button (short: reaction/Next, 0.8s hold: Menu/Select/Restore).
// - D2 (GPIO2, active-high) -> Secondary user button (short: cycle/Previous, 0.8s hold: Back).
constexpr int kExcludedBootPinD0 = 0;
constexpr int kPrimaryPinD1 = 1;
constexpr int kSecondaryPinD2 = 2;

struct ButtonEvents {
  bool short_press = false;
  bool hold_fired = false;
};

class DebouncedButton {
  bool raw_last_ = false;
  bool stable_ = false;
  bool hold_sent_ = false;
  uint32_t edge_ms_ = 0;
  uint32_t press_ms_ = 0;
  bool inited_ = false;

 public:
  static constexpr uint32_t kDebounceMs = 25;
  static constexpr uint32_t kHoldMs = 800;

  void reset() {
    raw_last_ = false;
    stable_ = false;
    hold_sent_ = false;
    inited_ = false;
  }

  ButtonEvents update(bool raw_pressed, uint32_t now_ms) {
    ButtonEvents ev;
    if (!inited_) {
      inited_ = true;
      raw_last_ = raw_pressed;
      edge_ms_ = now_ms;
      if (raw_pressed) {
        // Ignore a button already held at boot/reset until released.
        stable_ = true;
        hold_sent_ = true;
      }
      return ev;
    }

    if (raw_pressed != raw_last_) {
      raw_last_ = raw_pressed;
      edge_ms_ = now_ms;
    }

    if (raw_pressed != stable_ && uint32_t(now_ms - edge_ms_) >= kDebounceMs) {
      stable_ = raw_pressed;
      if (stable_) {
        press_ms_ = now_ms;
        hold_sent_ = false;
      } else {
        if (!hold_sent_) {
          ev.short_press = true;
        }
        hold_sent_ = false;
      }
    }

    if (stable_ && !hold_sent_ && uint32_t(now_ms - press_ms_) >= kHoldMs) {
      hold_sent_ = true;
      ev.hold_fired = true;
    }
    return ev;
  }
};

class FeatherMenu {
  DebouncedButton d1_, d2_;
  bool chord_active_ = false;
  bool chord_opened_menu_ = false;
  uint32_t chord_start_ms_ = 0;
  bool wait_pad_release_ = false;
  uint32_t prev_pad_buttons_ = 0;
  int prev_stick_dir_ = 0;

  unsigned itemCount() const {
    using opentoy_tiny_ui::MenuPage;
    switch (page) {
      case MenuPage::Root: return 7;
      case MenuPage::Personality: return faceVersion == 1 ? 4 : 1;
      case MenuPage::Display: return 3;  // Performance, Brightness, Back
      default: return 1;                 // Back / Return
    }
  }

  void transition() {
    ++epoch;
    focus = 0;
    wait_pad_release_ = true;
    prev_stick_dir_ = 0;
  }

  void selectFocused() {
    using opentoy_tiny_ui::MenuPage;
    if (page == MenuPage::Root) {
      switch (focus) {
        case 0: page = MenuPage::Play; transition(); break;
        case 1: page = MenuPage::Apps; transition(); break;
        case 2: page = MenuPage::Personality; transition(); break;
        case 3: page = MenuPage::Sound; transition(); break;
        case 4: page = MenuPage::Display; transition(); break;
        case 5: page = MenuPage::Controls; transition(); break;
        case 6: page = MenuPage::About; transition(); break;
      }
      return;
    }
    if (page == MenuPage::Personality) {
      if (faceVersion == 1 && focus < 4) {
        persona = focus;
        page = MenuPage::Root;
        transition();
      } else {
        back();
      }
      return;
    }
    if (page == MenuPage::Display) {
      if (focus == 0) {
        header = false;
        page = MenuPage::Play;
        transition();
      } else if (focus == 1) {
        dimBacklight = !dimBacklight;
        ++epoch;
      } else {
        back();
      }
      return;
    }
    back();
  }

 public:
  int faceVersion = 1;
  opentoy_tiny_ui::MenuPage page = opentoy_tiny_ui::MenuPage::Play;
  unsigned focus = 0;
  unsigned persona = 0;
  bool header = true;
  bool dimBacklight = false;
  uint32_t blinkPulseUntilMs = 0;
  uint32_t epoch = 1;

  explicit FeatherMenu(int fv = 1, unsigned initial_persona = 0)
      : faceVersion(fv), persona(initial_persona) {}

  bool playing() const { return page == opentoy_tiny_ui::MenuPage::Play; }

  void openMenu() {
    header = true;
    page = opentoy_tiny_ui::MenuPage::Root;
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

  void nextItem() {
    const unsigned n = itemCount();
    focus = (focus + 1u) % n;
    ++epoch;
  }

  void prevItem() {
    const unsigned n = itemCount();
    focus = (focus + n - 1u) % n;
    ++epoch;
  }

  void loseInput() {
    d1_.reset();
    d2_.reset();
    chord_active_ = false;
    chord_opened_menu_ = false;
    wait_pad_release_ = false;
    prev_pad_buttons_ = 0;
    prev_stick_dir_ = 0;
  }

  // Updates menu/buttons and returns filtered gamepad buttons safe to pass to Play Controls/Companion.
  // Stationary Select+Start is disambiguated here so >=800ms opens Menu without leaking a blink,
  // while <800ms release triggers a clean 180ms blink pulse!
  uint32_t update(bool d1_pressed,
                  bool d2_pressed,
                  int pad_x,
                  int pad_y,
                  uint32_t pad_buttons,
                  bool pad_valid,
                  uint32_t now_ms) {
    const ButtonEvents e1 = d1_.update(d1_pressed, now_ms);
    const ButtonEvents e2 = d2_.update(d2_pressed, now_ms);

    if (!pad_valid) {
      chord_active_ = false;
      chord_opened_menu_ = false;
      wait_pad_release_ = false;
      prev_pad_buttons_ = 0;
      prev_stick_dir_ = 0;
      pad_buttons = 0;
    }

    if (playing()) {
      if (!header) {
        if (e1.hold_fired) {
          openMenu();
          return 0;
        }
      } else {
        if (e1.hold_fired) {
          openMenu();
          return 0;
        }
        if (e1.short_press) {
          blinkPulseUntilMs = now_ms + 220u;
          ++epoch;
        }
        if (e2.short_press && faceVersion == 1) {
          persona = (persona + 1u) % 4u;
          ++epoch;
        }
      }

      if (!pad_valid) return 0;
      if (wait_pad_release_) {
        if (pad_buttons == 0 && Controls::axis(pad_x) == 0 && Controls::axis(pad_y) == 0) {
          wait_pad_release_ = false;
        } else {
          prev_pad_buttons_ = pad_buttons;
          return 0;
        }
      }

      const bool sel_start = ((pad_buttons & (SELECT | START)) == (SELECT | START));
      const bool stationary =
          (Controls::axis(pad_x) == 0 && Controls::axis(pad_y) == 0 &&
           (pad_buttons & (A | B | X | Y)) == 0);

      if (sel_start && stationary) {
        if (!chord_active_) {
          chord_active_ = true;
          chord_opened_menu_ = false;
          chord_start_ms_ = now_ms;
        } else if (!chord_opened_menu_ && uint32_t(now_ms - chord_start_ms_) >= 800u) {
          chord_opened_menu_ = true;
          openMenu();
          return 0;
        }
        prev_pad_buttons_ = pad_buttons;
        // Withhold SELECT|START while disambiguating so no blink leaks into Menu.
        return pad_buttons & ~(SELECT | START);
      }

      if (chord_active_) {
        const bool was_short = !chord_opened_menu_;
        chord_active_ = false;
        chord_opened_menu_ = false;
        if (was_short && !sel_start) {
          blinkPulseUntilMs = now_ms + 180u;
          ++epoch;
        }
      }

      prev_pad_buttons_ = pad_buttons;
      return pad_buttons;
    }

    // Menu is open: handle D1/D2 and optional QT gamepad navigation.
    if (e1.hold_fired) {
      selectFocused();
      return 0;
    }
    if (e1.short_press) {
      nextItem();
    }
    if (e2.hold_fired) {
      back();
      return 0;
    }
    if (e2.short_press) {
      prevItem();
    }

    if (pad_valid) {
      if (wait_pad_release_) {
        if (pad_buttons == 0 && Controls::axis(pad_x) == 0 && Controls::axis(pad_y) == 0) {
          wait_pad_release_ = false;
        }
        prev_pad_buttons_ = pad_buttons;
        return 0;
      }
      const int ay = Controls::axis(pad_y);
      const int dir = (ay < -45) ? -1 : (ay > 45) ? 1 : 0;
      if (dir != 0 && dir != prev_stick_dir_) {
        if (dir < 0) prevItem();
        else nextItem();
      }
      prev_stick_dir_ = dir;

      const uint32_t pressed = pad_buttons & ~prev_pad_buttons_;
      prev_pad_buttons_ = pad_buttons;
      if (pressed & A) {
        selectFocused();
      } else if (pressed & B) {
        back();
      }
    }
    return 0;
  }
};

inline uint16_t featherSwapColor(uint16_t rgb565, bool swap_bytes) {
  return swap_bytes ? uint16_t((rgb565 >> 8) | (rgb565 << 8)) : rgb565;
}

// Renders the single-row 240x15 APP-STATUS-BAR.md v1.0 header into a 240x135 canvas.
inline void renderFeatherStatusBar(uint16_t* buf240x135,
                                   int face_version,
                                   unsigned persona_id,
                                   int battery_pct,
                                   bool swap_bytes = true) {
  if (!buf240x135) return;
  opentoy_tiny_ui::fillRectBuf(buf240x135, 240, 135, 0, 0, 240, 15, 0x0000);

  const uint16_t accent = featherSwapColor(
      face_version == 2 ? 0xfd20 : (persona_id ? personality(persona_id).ink : 0x07ff),
      swap_bytes);
  const uint16_t silver = featherSwapColor(0xbefb, swap_bytes);
  const uint16_t green = featherSwapColor(0x76f7, swap_bytes);
  const uint16_t red = featherSwapColor(0xec2c, swap_bytes);

  char left_txt[40] = {};
  std::snprintf(left_txt,
                sizeof(left_txt),
                "%s V%s %s",
                appShortName(face_version),
                version,
                created);
  opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 3, 5, left_txt, accent, 1, 185);

  if (face_version == 1 && persona_id > 0) {
    const int lx = 3 + opentoy_tiny_ui::textWidth3x5(left_txt, 1) + 6;
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, lx, 5, personality(persona_id).name, silver, 1, 182);
  }

  // Right-aligned truthful battery: "BAT --" (no misleading empty battery box) or "XX%" + gauge.
  char bat_txt[10] = {};
  opentoy_tiny_ui::formatGaugeBatteryText(battery_pct, bat_txt);
  if (battery_pct < 0 || battery_pct > 100) {
    const int bw = opentoy_tiny_ui::textWidth3x5(bat_txt, 1);
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 237 - bw, 5, bat_txt, silver, 1, 240);
  } else {
    opentoy_tiny_ui::drawRectBuf(buf240x135, 240, 135, 219, 4, 16, 7, silver);
    opentoy_tiny_ui::fillRectBuf(buf240x135, 240, 135, 235, 6, 2, 3, silver);
    const int fw = (14 * battery_pct) / 100;
    if (fw > 0) {
      opentoy_tiny_ui::fillRectBuf(
          buf240x135, 240, 135, 220, 5, fw, 5, battery_pct <= 15 ? red : green);
    }
    const int tw = opentoy_tiny_ui::textWidth3x5(bat_txt, 1);
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 216 - tw, 5, bat_txt, silver, 1, 218);
  }
}

// Renders the full 240x135 Common Menu screen on Feather Reverse TFT.
inline void renderFeatherMenuScreen(const FeatherMenu& ui,
                                    int battery_pct,
                                    uint16_t* buf240x135,
                                    bool swap_bytes = true) {
  if (!buf240x135) return;
  std::fill(buf240x135, buf240x135 + 240 * 135, uint16_t(0));
  renderFeatherStatusBar(buf240x135, ui.faceVersion, ui.persona, battery_pct, swap_bytes);

  const uint16_t cyan = featherSwapColor(0x07ff, swap_bytes);
  const uint16_t white = featherSwapColor(0xffff, swap_bytes);
  const uint16_t silver = featherSwapColor(0xbefb, swap_bytes);
  const uint16_t dark = featherSwapColor(0x1082, swap_bytes);
  const uint16_t black = 0x0000;

  auto drawRow = [&](int y, const char* label, bool focused, bool active_mark = false) {
    opentoy_tiny_ui::fillRectBuf(buf240x135, 240, 135, 8, y, 224, 14, focused ? cyan : dark);
    opentoy_tiny_ui::drawRectBuf(buf240x135, 240, 135, 8, y, 224, 14, focused ? white : silver);
    int tx = 14;
    if (active_mark) {
      opentoy_tiny_ui::fillRectBuf(buf240x135, 240, 135, 12, y + 5, 3, 4, focused ? black : cyan);
      tx = 19;
    }
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, tx, y + 5, label, focused ? black : white, 1, 228);
  };

  using opentoy_tiny_ui::MenuPage;
  if (ui.page == MenuPage::Root) {
    // Window of 5 visible items out of 7 so every item is 14 px tall and clearly readable.
    const unsigned start = (ui.focus >= 4u) ? 2u : 0u;
    for (unsigned i = 0; i < 5u && (start + i) < 7u; ++i) {
      const unsigned idx = start + i;
      drawRow(18 + int(i) * 19, opentoy_tiny_ui::rootEntryLabel(idx), ui.focus == idx, false);
    }
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 120, "D1 NEXT/HOLD OK   D2 PREV/HOLD BACK", silver, 1, 236);
    return;
  }

  if (ui.page == MenuPage::Personality) {
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 19, "PERSONALITY", cyan, 1, 232);
    if (ui.faceVersion == 1) {
      for (unsigned i = 0; i < 4u; ++i) {
        drawRow(30 + int(i) * 20, personality(i).name, ui.focus == i, ui.persona == i);
      }
    } else {
      opentoy_tiny_ui::drawText3x5(
          buf240x135, 240, 135, 8, 44, "EYES2 USES DOODLE EXPRESSIONS", silver, 1, 232);
      drawRow(90, "BACK", true, false);
    }
    return;
  }

  if (ui.page == MenuPage::Display) {
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 19, "DISPLAY", cyan, 1, 232);
    drawRow(32, "ENTER PERFORMANCE MODE", ui.focus == 0, false);
    drawRow(54, ui.dimBacklight ? "BACKLIGHT: ECO DIM" : "BACKLIGHT: NORMAL", ui.focus == 1, true);
    drawRow(76, "BACK", ui.focus == 2, false);
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 108, "HOLD D1 OR SEL+START 0.8S TO RESTORE", silver, 1, 236);
    return;
  }

  if (ui.page == MenuPage::Apps) {
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 19, "APPS", cyan, 1, 232);
    char cur[32] = {};
    std::snprintf(cur, sizeof(cur), "%s INSTALLED", appShortName(ui.faceVersion));
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 40, cur, white, 1, 232);
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 54, "MORE APPS REQUIRE INSTALLATION", silver, 1, 232);
    drawRow(92, "BACK", true, false);
    return;
  }

  if (ui.page == MenuPage::Sound) {
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 19, "SOUND", cyan, 1, 232);
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 40, "NO SPEAKER OR MIC ON THIS ASSEMBLY", silver, 1, 232);
    drawRow(92, "BACK", true, false);
    return;
  }

  if (ui.page == MenuPage::Controls) {
    opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 19, "CONTROLS (D0 BOOT EXCLUDED)", cyan, 1, 232);
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 38, "D1 SHORT: BLINK   HOLD 0.8S: MENU", white, 1, 232);
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 52, "D2 SHORT: PERSONA HOLD 0.8S: BACK", silver, 1, 232);
    opentoy_tiny_ui::drawText3x5(
        buf240x135, 240, 135, 8, 66, "PAD SEL+START HOLD 0.8S: MENU", silver, 1, 232);
    drawRow(92, "BACK", true, false);
    return;
  }

  // About
  opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 19, "ABOUT", cyan, 1, 232);
  char info[40] = {};
  std::snprintf(info, sizeof(info), "%s V%s %s", appShortName(ui.faceVersion), version, created);
  opentoy_tiny_ui::drawText3x5(buf240x135, 240, 135, 8, 40, info, white, 1, 232);
  opentoy_tiny_ui::drawText3x5(
      buf240x135, 240, 135, 8, 54, "REVERSE TFT FEATHER ESP32-S3", silver, 1, 232);
  drawRow(92, "BACK", true, false);
}

}  // namespace feather_face
