// SPDX-License-Identifier: MIT
#include "Interaction.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace touch169;

static void tap(Interaction& u, int x, int y, uint32_t t) {
  u.update({true, true, x, y}, t);
  u.update({true, false, x, y}, t + 40);
}

static void swipeTo(Interaction& u, int x0, int y0, int x1, int y1, uint32_t t, uint32_t dt = 120) {
  u.update({true, true, x0, y0}, t);
  u.update({true, true, x1, y1}, t + dt / 2);
  assert(u.poke == 0);
  u.update({true, false, x1, y1}, t + dt);
}

int main() {
  // Ordered app model: verify index/count and cycling in both directions from every state.
  static_assert(appCount == 2, "expected two apps");
  for (auto startApp : {App::Face, App::Water}) {
    const App other = (startApp == App::Face) ? App::Water : App::Face;

    // From Play: LEFT selects previous app, RIGHT selects next app.
    for (int dir : {-1, 1}) {
      Interaction u;
      u.app = startApp;
      u.page = Page::Play;
      u.persona = 3;
      u.brightness = 35;
      swipeTo(u, 120, 140, 120 + dir * 80, 145, 100);
      assert(u.app == other);
      assert(u.page == Page::Play);
      assert(u.poke == 0 && u.pokes == 0);
      assert(!u.waterTouch);
      assert(u.persona == 3 && u.brightness == 35);
      assert(u.swipes == 1);
    }

    // From Hardware & Software panels: LEFT/RIGHT cycles app and closes panel without activating buttons.
    for (auto panel : {Page::Hardware, Page::Software}) {
      for (int dir : {-1, 1}) {
        Interaction u;
        u.app = startApp;
        u.page = panel;
        u.persona = 2;
        u.brightness = 35;
        // Start directly over the first Software settings row (y = 105 -> target 0).
        swipeTo(u, 120, 105, 120 + dir * 90, 112, 100);
        assert(u.app == other);
        assert(u.page == Page::Play);
        assert(u.poke == 0 && u.pokes == 0);
        assert(u.persona == 2 && u.brightness == 35);
        assert(u.swipes == 1);
      }
    }
  }

  // Swipes starting over each Software settings row (persona, brightness, performance) must not activate row.
  for (int rowY : {105, 150, 195}) {
    Interaction u;
    u.app = App::Water;
    u.page = Page::Software;
    u.persona = 1;
    u.brightness = 100;
    u.header = true;
    // Horizontal swipe out of the row
    swipeTo(u, 120, rowY, 200, rowY + 5, 100);
    assert(u.app == App::Face && u.page == Page::Play);
    assert(u.persona == 1 && u.brightness == 100 && u.header);

    // Small drag (25 px < 48 px) starting and ending over the row must NOT activate it.
    Interaction drag;
    drag.app = App::Face;
    drag.page = Page::Software;
    drag.update({true, true, 100, rowY}, 100);
    drag.update({true, true, 125, rowY}, 140);
    drag.update({true, false, 125, rowY}, 180);
    assert(drag.page == Page::Software && drag.app == App::Face);
    assert(drag.persona == 1 && drag.brightness == 100 && drag.header);
  }

  // Swipes starting over bottom overlay buttons (y=255) must navigate via swipe without activating button.
  {
    Interaction u;
    u.app = App::Face;
    u.page = Page::Play;
    swipeTo(u, 60, 255, 60, 160, 100);  // Swipe UP from over HARDWARE button -> opens SOFTWARE
    assert(u.page == Page::Software && u.app == App::Face && u.poke == 0);
    swipeTo(u, 180, 255, 235, 255, 300);  // Swipe RIGHT from over SOFTWARE/BACK -> cycles to Water
    assert(u.page == Page::Play && u.app == App::Water && !u.waterTouch);
  }

  // Jitter, ambiguous diagonals and slow drags must not switch apps or dismiss a panel.
  for (auto end : std::vector<Touch>{{true, true, 155, 150}, {true, true, 190, 220}}) {
    Interaction u;
    u.page = Page::Software;
    u.app = App::Face;
    u.update({true, true, 120, 150}, 100);
    u.update(end, 180);
    end.down = false;
    u.update(end, 220);
    assert(u.page == Page::Software && u.app == App::Face && u.brightness == 100);
  }
  {
    Interaction slow;
    slow.page = Page::Play;
    slow.app = App::Face;
    slow.update({true, true, 40, 150}, 100);
    slow.update({true, true, 180, 150}, 1601);  // 1501 ms > 1500 ms
    slow.update({true, false, 180, 150}, 1601);
    assert(slow.page == Page::Play && slow.app == App::Face);

    Interaction boundary;
    boundary.page = Page::Play;
    boundary.app = App::Face;
    boundary.update({true, true, 40, 150}, 100);
    boundary.update({true, true, 180, 150}, 1599);
    boundary.update({true, false, 180, 150}, 1600);  // exactly 1500 ms
    assert(boundary.page == Page::Play && boundary.app == App::Water);
  }

  // Interrupted contact must not trigger swipe on stale release.
  {
    Interaction interrupted;
    interrupted.page = Page::Software;
    interrupted.app = App::Face;
    interrupted.update({true, true, 40, 150}, 100);
    interrupted.update({true, true, 180, 150}, 180);
    interrupted.update({false, false, 0, 0}, 200);
    interrupted.update({true, false, 180, 150}, 220);
    assert(interrupted.page == Page::Software && interrupted.app == App::Face);
  }

  // Performance mode preserves across app cycling; top-center hold restores Software on active app; Back returns to active app.
  {
    Interaction u;
    assert(u.header && u.app == App::Face && u.persona == 1);
    tap(u, 55, 130, 0);
    assert(u.poke == 1 && u.pokes == 1);
    tap(u, 185, 130, 100);
    assert(u.poke == 2);
    swipeTo(u, 80, 160, 80, 50, 200);  // Swipe UP -> Software
    assert(u.page == Page::Software && u.poke == 0 && u.pokes == 2);
    tap(u, 80, 110, 400);
    assert(u.persona == 2);
    tap(u, 80, 150, 500);
    assert(u.brightness == 35);
    tap(u, 80, 195, 600);  // PERFORMANCE
    assert(!u.header && u.page == Page::Play && u.app == App::Face);

    // Cycle to Water while staying in Performance mode.
    swipeTo(u, 190, 140, 60, 140, 650);
    assert(!u.header && u.page == Page::Play && u.app == App::Water);
    assert(u.persona == 2 && u.brightness == 35);

    // Top-center hold restores header and opens Software while preserving Water app.
    u.update({true, true, 120, 10}, 900);
    u.update({true, true, 120, 10}, 1700);
    assert(u.header && u.page == Page::Software && u.app == App::Water);
    u.update({true, false, 120, 10}, 1710);
    assert(u.poke == 0 && !u.waterTouch);

    // Back returns to Water (selected play app).
    tap(u, 100, 250, 1800);
    assert(u.page == Page::Play && u.app == App::Water);
    assert(u.persona == 2 && u.brightness == 35);

    // Swipe DOWN from Water -> Hardware, Back returns to Water.
    swipeTo(u, 100, 60, 100, 180, 1900);
    assert(u.page == Page::Hardware && u.app == App::Water && u.poke == 0);
    tap(u, 100, 250, 2100);
    assert(u.page == Page::Play && u.app == App::Water);
  }

  // Full-screen Water touch mapping: direct screen coordinates across [0,240)x[0,280);
  // taps/presses starting inside actual visible controls are consumed by controls, never water/poke.
  {
    Interaction u;
    u.app = App::Water;
    // Touch inside middle play area -> direct screen coordinates (no bar subtraction).
    u.update({true, true, 120, 140}, 100);
    assert(u.waterTouch && u.waterX == 120 && u.waterY == 140);
    assert(u.poke == 0);
    u.update({true, false, 120, 140}, 110);
    // Touch under overlaid header (y=12) and in gap between bottom buttons (120, 255) and below (120, 275) -> waterTouch!
    u.update({true, true, 120, 12}, 120);
    assert(u.waterTouch && u.waterX == 120 && u.waterY == 12);
    u.update({true, false, 120, 12}, 140);
    assert(!u.waterTouch && u.poke == 0 && u.pokes == 0);

    u.update({true, true, 120, 255}, 150);  // Gap between HARDWARE (10..115) and SOFTWARE (124..229)
    assert(u.waterTouch && u.waterX == 120 && u.waterY == 255);
    u.update({true, false, 120, 255}, 170);
    assert(u.page == Page::Play);  // Gap did NOT open a panel!

    u.update({true, true, 120, 275}, 180);  // Below buttons
    assert(u.waterTouch && u.waterX == 120 && u.waterY == 275);
    u.update({true, false, 120, 275}, 190);
    assert(u.page == Page::Play);

    // Press starting inside SOFTWARE button (180, 255): consumed by control, NEVER triggers waterTouch even if dragged!
    u.update({true, true, 180, 255}, 240);
    assert(!u.waterTouch);
    u.update({true, true, 180, 220}, 250);  // Dragged out of button (< 48px swipe threshold)
    assert(!u.waterTouch);
    u.update({true, false, 180, 220}, 260);  // Released outside button -> cancelled, neither button nor water
    assert(u.page == Page::Play && !u.waterTouch);

    tap(u, 180, 238, 270);  // Preserve the original usable button height, near its top edge
    assert(u.page == Page::Software);
    tap(u, 120, 255, 320);  // Back to Water Play
    assert(u.page == Page::Play && u.app == App::Water);

    // In Performance mode (!header), coordinates over former button rects (180, 255) also touch Water!
    u.header = false;
    u.update({true, true, 180, 255}, 340);
    assert(u.waterTouch && u.waterX == 180 && u.waterY == 255);
    u.update({true, false, 180, 255}, 350);
    u.header = true;

    // Invalid / out-of-range touch cancels waterTouch until release.
    u.update({true, true, 120, 140}, 360);
    assert(u.waterTouch);
    u.update({false, false, 0, 0}, 370);
    assert(!u.waterTouch);
    u.update({true, true, 120, 140}, 380);
    assert(!u.waterTouch);  // Still blocked until release!
    u.update({true, false, 120, 140}, 390);
    u.update({true, true, 120, 140}, 400);
    assert(u.waterTouch);
    u.update({true, false, 120, 140}, 410);
  }

  // Face full-screen touch: header area, gap between buttons poke; control buttons consume press without poking!
  {
    Interaction u;
    u.app = App::Face;
    u.header = true;
    tap(u, 55, 15, 100);  // Top area under overlaid header pokes left eye
    assert(u.poke == 1 && u.pokes == 1);
    tap(u, 120, 255, 200);  // Gap between buttons pokes both eyes
    assert(u.poke == 3 && u.pokes == 2);
    // Starting in HARDWARE button and dragging out must NOT poke Face.
    u.update({true, true, 50, 255}, 300);
    u.update({true, true, 50, 230}, 320);
    u.update({true, false, 50, 230}, 340);
    assert(u.poke == 0 && u.pokes == 2 && u.page == Page::Play);
    // Clean tap on HARDWARE opens Hardware panel.
    tap(u, 50, 255, 400);
    assert(u.page == Page::Hardware && u.poke == 0 && u.pokes == 2);
  }

  // Cancel drag-out and re-entry, including excursion that comes back to start.
  {
    Interaction u;
    u.update({true, true, 50, 255}, 2000);
    u.update({true, true, 50, 200}, 2010);
    u.update({true, true, 50, 255}, 2020);
    u.update({true, false, 50, 255}, 2030);
    assert(u.page == Page::Play && u.poke == 0);
    u.update({true, true, 20, 100}, 2300);
    u.update({true, true, 500, 100}, 2310);
    u.update({true, false, 20, 100}, 2320);
    assert(u.poke == 0);
  }

  // Unsigned duration arithmetic across millis rollover (both vertical and horizontal).
  {
    Interaction u;
    swipeTo(u, 100, 60, 100, 180, 0xffffffc0, 120);
    assert(u.page == Page::Hardware);
    swipeTo(u, 180, 140, 60, 140, 0xffffffc0, 120);
    assert(u.page == Page::Play && u.app == App::Water);
  }

  // Render every state into guarded storage: catches UI/face/water boundary writes.
  std::vector<uint16_t> out(width * height + 2, 0xabcd);
  Scene s;
  Diagnostics d;
  for (auto app : {App::Face, App::Water}) {
    for (auto p : {Page::Play, Page::Hardware, Page::Software}) {
      for (bool h : {true, false}) {
        for (bool imuOk : {false, true}) {
          Interaction u;
          u.app = app;
          u.page = p;
          u.header = h;
          u.persona = 1 + (static_cast<unsigned>(p) % 3);
          d.batteryMv = -1;
          d.rawBatteryMv = 4200;
          d.imuPresent = imuOk;
          d.imuFresh = imuOk;
          d.imuAddr = imuOk ? 0x6b : 0;
          d.imuWho = imuOk ? 0x05 : 0;
          d.ax = 0.99f;
          d.ay = -0.02f;
          d.az = 0.05f;
          s.render(out.data() + 1, u, d, 3000);
          assert(out.front() == 0xabcd && out.back() == 0xabcd);
        }
      }
    }
  }

  // Overlay visibility at edges, non-overlapping 2-row header, Face full-screen layout,
  // and Performance mode hiding overlays without physics or layout reset.
  {
    Scene scene;
    Diagnostics diag;
    diag.batteryMv = -1;
    diag.imuPresent = true;
    diag.imuFresh = false;
    Interaction ui;
    ui.app = App::Water;
    ui.page = Page::Play;
    ui.header = true;
    std::vector<uint16_t> withOverlay(width * height, 0);
    std::vector<uint16_t> perfOnly(width * height, 0);
    scene.render(withOverlay.data(), ui, diag, 1000);
    ui.header = false;
    scene.render(perfOnly.data(), ui, diag, 1000);

    // Top-left corner (0,0), top-center (120,2), bottom gap (120,255), bottom corner (239,279)
    // are pure water sky even when overlay is visible (no opaque full-width header/footer bar).
    for (auto pt : std::vector<std::pair<int, int>>{{0, 0}, {120, 2}, {120, 255}, {239, 279}}) {
      const size_t idx = size_t(pt.second) * width + pt.first;
      assert(withOverlay[idx] == perfOnly[idx]);
      const uint16_t c = withOverlay[idx];
      const int r = (c >> 11) & 31, b = c & 31;
      assert(b > r);
    }
    // Header text and warning must appear in top rows (y=8..33) when header=true, and disappear in Performance.
    int headerInk = 0, perfTopNonBlue = 0;
    for (int y = 7; y < 35; ++y) {
      for (int x = 8; x < width - 8; ++x) {
        if (withOverlay[y * width + x] == 0x07ff || withOverlay[y * width + x] == 0xfbe4) ++headerInk;
        if (perfOnly[y * width + x] == 0x07ff || perfOnly[y * width + x] == 0xfbe4) ++perfTopNonBlue;
      }
    }
    assert(headerInk > 80);
    assert(perfTopNonBlue == 0);
    // Row 1 left ("WATER V0.2.2-DEV") and row 1 right ("BAT 4.20V") must leave a clean clear gap.
    diag.batteryMv = 4200;
    ui.header = true;
    scene.render(withOverlay.data(), ui, diag, 1000);
    int gapOverlayPixels = 0;
    for (int y = 7; y < 19; ++y) {
      for (int x = 142; x < 156; ++x) {
        if (withOverlay[y * width + x] != perfOnly[y * width + x]) ++gapOverlayPixels;
      }
    }
    assert(gapOverlayPixels == 0);

    // Face receives identical full-screen projection regardless of header visibility.
    ui.app = App::Face;
    ui.header = true;
    std::vector<uint16_t> faceWith(width * height, 0);
    std::vector<uint16_t> facePerf(width * height, 0);
    scene.render(faceWith.data(), ui, diag, 3000);
    ui.header = false;
    scene.render(facePerf.data(), ui, diag, 3000);
    for (int y = 40; y < 240; ++y) {
      for (int x = 0; x < width; ++x) {
        if (!contains(hardwareButton, x, y) && !contains(softwareButton, x, y))
          assert(faceWith[y * width + x] == facePerf[y * width + x]);
      }
    }
  }

  std::cout << "interaction, cycling, water touch, rollover and renderer checks passed\n";
}
