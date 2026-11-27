// SPDX-License-Identifier: MIT
#pragma once
#include "Metadata.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace touch169 {

enum class Page { Play, Hardware, Software };

inline const char* pageName(Page p) {
  switch (p) {
    case Page::Play: return "play";
    case Page::Hardware: return "hardware";
    case Page::Software: return "software";
  }
  return "play";
}

struct Touch {
  bool valid = true;
  bool down = false;
  int x = 0;
  int y = 0;
};

class Interaction {
  bool active = false;
  bool blocked = false;
  bool moved = false;
  bool startedInControl = false;
  int startX = 0;
  int startY = 0;
  int lastX = 0;
  int lastY = 0;
  int target = -1;
  uint32_t started = 0;

  int hit(int x, int y) const {
    if (page == Page::Play) {
      if (!header) return -1;
      if (contains(hardwareButton, x, y)) return 10;
      if (contains(softwareButton, x, y)) return 11;
      return -1;
    }
    if (contains(panelBackButton, x, y)) return 20;
    if (page == Page::Software && x >= 10 && x < 230 && y >= 88 && y < 220) {
      const int localY = y - 88;
      if (localY % 44 < 40) return localY / 44;
    }
    return -1;
  }

  void activate(int id) {
    if (id == 10) {
      page = Page::Hardware;
      header = true;
    } else if (id == 11) {
      page = Page::Software;
      header = true;
    } else if (id == 20) {
      page = Page::Play;
    } else if (id == 0) {
      persona = persona % 3 + 1;
    } else if (id == 1) {
      brightness = brightness == 100 ? 35 : 100;
    } else if (id == 2) {
      header = false;
      page = Page::Play;
    }
  }

  void selectOffset(int delta) {
    const int count = static_cast<int>(appCount);
    const int idx = static_cast<int>(static_cast<unsigned>(app));
    const int next = ((idx + delta) % count + count) % count;
    app = static_cast<App>(static_cast<unsigned>(next));
  }

 public:
  App app = App::Face;
  Page page = Page::Play;
  bool header = true;
  unsigned persona = 1;
  unsigned brightness = 100;
  unsigned poke = 0;
  bool waterTouch = false;
  int waterX = 0;
  int waterY = 0;
  uint32_t pokes = 0;
  uint32_t swipes = 0;

  void prevApp() { selectOffset(-1); }
  void nextApp() { selectOffset(+1); }

  void cancel() {
    active = false;
    blocked = true;
    startedInControl = false;
    poke = 0;
    waterTouch = false;
  }

  void update(Touch t, uint32_t now) {
    poke = 0;
    waterTouch = false;
    if (!t.valid || (t.down && (t.x < 0 || t.x >= width || t.y < 0 || t.y >= height))) {
      cancel();
      return;
    }
    if (blocked) {
      if (!t.down) blocked = false;
      return;
    }
    if (t.down) {
      if (!active) {
        active = true;
        startX = lastX = t.x;
        startY = lastY = t.y;
        started = now;
        target = hit(t.x, t.y);
        startedInControl = (target >= 0);
        moved = false;
      } else {
        lastX = t.x;
        lastY = t.y;
        if (std::abs(lastX - startX) > 12 || std::abs(lastY - startY) > 12) moved = true;
        if (target >= 0 && hit(t.x, t.y) != target) target = -1;
        // Deliberate top-center hold restores the header in Performance.
        if (!header && contains(restoreZone, startX, startY) && !moved &&
            uint32_t(now - started) >= 800) {
          header = true;
          page = Page::Software;
          cancel();
          return;
        }
      }
      if (page == Page::Play && app == App::Water && !startedInControl) {
        waterTouch = true;
        waterX = t.x;
        waterY = t.y;
      }
      return;
    }
    if (!active) return;
    active = false;
    const bool wasControl = startedInControl;
    startedInControl = false;
    const int dx = lastX - startX;
    const int dy = lastY - startY;
    const uint32_t elapsed = uint32_t(now - started);
    // Decide on release: a gesture never produces an early eye poke or button activation.
    if (std::abs(dy) >= 48 && std::abs(dy) > std::abs(dx) * 3 / 2 && elapsed <= 1500) {
      page = dy > 0 ? Page::Hardware : Page::Software;
      header = true;
      ++swipes;
      return;
    }
    if (std::abs(dx) >= 48 && std::abs(dx) > std::abs(dy) * 3 / 2 && elapsed <= 1500) {
      if (dx < 0) prevApp();
      else nextApp();
      page = Page::Play;
      ++swipes;
      return;
    }
    if (moved) return;
    if (target >= 0 && hit(lastX, lastY) == target) {
      activate(target);
      return;
    }
    if (page == Page::Play && app == App::Face && !wasControl) {
      poke = startX < 100 ? 1 : startX >= 140 ? 2 : 3;
      ++pokes;
    }
  }
};

}  // namespace touch169
