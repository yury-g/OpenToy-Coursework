// SPDX-License-Identifier: MIT
#include "BoardAxes.h"
#include "Metadata.h"
#include "Qmi8658.h"
#include "Scene.h"
#include "Telemetry.h"
#include "WaterView.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace touch169;

namespace {

struct FakeBus {
  struct Device {
    bool present = false;
    std::map<uint8_t, uint8_t> regs;
    bool resetSeen = false;
    int resetPolls = 0;
    bool corruptConfigReadback = false;
  };
  std::map<uint8_t, Device> devs;
  std::vector<std::pair<uint8_t, uint8_t>> writes;
  bool failAllReads = false;
  uint32_t delayedMs = 0;

  void delayMs(uint32_t ms) { delayedMs += ms; }

  bool readReg(uint8_t addr, uint8_t reg, uint8_t* out, size_t n) {
    if (failAllReads) return false;
    auto it = devs.find(addr);
    if (it == devs.end() || !it->second.present) return false;
    auto& d = it->second;
    for (size_t i = 0; i < n; ++i) {
      uint8_t r = uint8_t(reg + i);
      if (r == qmi8658::kRegRstResult && d.resetSeen) {
        ++d.resetPolls;
        out[i] = (d.resetPolls >= 1) ? qmi8658::kRstResultDone : 0x00;
      } else {
        auto rit = d.regs.find(r);
        out[i] = (rit != d.regs.end()) ? rit->second : 0x00;
      }
    }
    return true;
  }

  bool writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
    auto it = devs.find(addr);
    if (it == devs.end() || !it->second.present) return false;
    writes.push_back({reg, val});
    auto& d = it->second;
    if (reg == qmi8658::kRegReset && val == qmi8658::kResetCmd) {
      d.resetSeen = true;
      return true;
    }
    if (d.corruptConfigReadback && reg == qmi8658::kRegCtrl2) {
      d.regs[reg] = 0x00;
      return true;
    }
    d.regs[reg] = val;
    return true;
  }
};

struct MockSerialPort {
  bool connected = true;
  int capacity = 0;
  std::string output;
  explicit operator bool() const { return connected; }
  int availableForWrite() { return capacity; }
  size_t write(const unsigned char* data, size_t n) {
    assert(n <= size_t(capacity));
    output.append(reinterpret_cast<const char*>(data), n);
    capacity -= int(n);
    return n;
  }
};

void putLe16(uint8_t* dst, int16_t v) {
  uint16_t u = static_cast<uint16_t>(v);
  dst[0] = uint8_t(u & 0xff);
  dst[1] = uint8_t((u >> 8) & 0xff);
}

bool near(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps;
}

}  // namespace

int main() {
  // 1. QMI8658 WHO_AM_I identity check BEFORE any reset/configuration writes.
  {
    FakeBus bus;
    bus.devs[0x6b].present = true;
    bus.devs[0x6b].regs[qmi8658::kRegWhoAmI] = 0x42;  // Wrong chip ID!
    bus.devs[0x6a].present = true;
    bus.devs[0x6a].regs[qmi8658::kRegWhoAmI] = 0xFF;  // Wrong chip ID!

    qmi8658::ImuState st;
    assert(!qmi8658::probeAndConfigure(bus, st, 100));
    assert(!st.present);
    assert(bus.writes.empty());  // Must never write RESET/CTRL to an unidentified chip!
  }

  // 2. Fallback probe from 0x6b (missing/wrong) -> 0x6a (valid WHO_AM_I 0x05).
  {
    FakeBus bus;
    bus.devs[0x6b].present = false;
    bus.devs[0x6a].present = true;
    bus.devs[0x6a].regs[qmi8658::kRegWhoAmI] = qmi8658::kExpectedWhoAmI;
    bus.devs[0x6a].regs[qmi8658::kRegRevision] = 0x7c;

    qmi8658::ImuState st;
    assert(qmi8658::probeAndConfigure(bus, st, 100));
    assert(st.present && st.address == 0x6a);
    assert(st.whoAmI == 0x05 && st.revision == 0x7c);
    assert(bus.devs[0x6a].resetSeen);
    assert(bus.devs[0x6a].regs[qmi8658::kRegCtrl1] == qmi8658::kCtrl1Value);
    assert(bus.devs[0x6a].regs[qmi8658::kRegCtrl2] == qmi8658::kCtrl2Value);
    assert(bus.devs[0x6a].regs[qmi8658::kRegCtrl3] == qmi8658::kCtrl3Value);
    assert(bus.devs[0x6a].regs[qmi8658::kRegCtrl5] == qmi8658::kCtrl5Value);
    assert(bus.devs[0x6a].regs[qmi8658::kRegCtrl7] == qmi8658::kCtrl7Value);
  }

  // 3. Config readback verification catches corrupted configuration.
  {
    FakeBus bus;
    bus.devs[0x6b].present = true;
    bus.devs[0x6b].regs[qmi8658::kRegWhoAmI] = qmi8658::kExpectedWhoAmI;
    bus.devs[0x6b].corruptConfigReadback = true;

    qmi8658::ImuState st;
    assert(!qmi8658::probeAndConfigure(bus, st, 100));
    assert(!st.present);
  }

  // 4. QMI8658 raw decoding & scaling (+4g, +512dps, negative 2's complement).
  {
    uint8_t raw[12] = {};
    putLe16(raw + 0, 8192);    // +1.0 g
    putLe16(raw + 2, -8192);   // -1.0 g
    putLe16(raw + 4, -32768);  // -4.0 g
    putLe16(raw + 6, 6400);    // +100.0 dps (6400 * 512 / 32768 = 100)
    putLe16(raw + 8, -16384);  // -256.0 dps
    putLe16(raw + 10, 0);

    qmi8658::Sample s;
    qmi8658::decodeSample(raw, s);
    assert(near(s.ax, 1.0f));
    assert(near(s.ay, -1.0f));
    assert(near(s.az, -4.0f));
    assert(near(s.gx, 100.0f));
    assert(near(s.gy, -256.0f));
    assert(near(s.gz, 0.0f));
  }

  // 5. Data-ready bit gating & stale rejection + rollover.
  {
    FakeBus bus;
    bus.devs[0x6b].present = true;
    bus.devs[0x6b].regs[qmi8658::kRegWhoAmI] = qmi8658::kExpectedWhoAmI;
    bus.devs[0x6b].regs[qmi8658::kRegRevision] = 0x12;

    qmi8658::ImuState st;
    assert(qmi8658::probeAndConfigure(bus, st, 1000));
    assert(!st.isFresh(1000));  // No sample read yet!

    // Only accel bit set (0x01) -> not both accel+gyro ready.
    bus.devs[0x6b].regs[qmi8658::kRegStatus0] = 0x01;
    assert(!qmi8658::pollSample(bus, st, 1010));
    assert(st.samples == 0 && !st.isFresh(1010));

    // Both accel and gyro ready (0x03).
    bus.devs[0x6b].regs[qmi8658::kRegStatus0] = 0x03;
    uint8_t pkt[12] = {};
    putLe16(pkt + 0, 8192);   // ax = +1.0 g (upright)
    putLe16(pkt + 2, 0);      // ay = 0
    putLe16(pkt + 4, 0);      // az = 0
    putLe16(pkt + 6, 320);    // gx = +5 dps
    putLe16(pkt + 8, -320);   // gy = -5 dps
    putLe16(pkt + 10, 0);
    for (int i = 0; i < 12; ++i) bus.devs[0x6b].regs[uint8_t(qmi8658::kRegAxL + i)] = pkt[i];

    assert(qmi8658::pollSample(bus, st, 1020));
    assert(st.samples == 1 && st.isFresh(1020));
    assert(near(st.ax, 1.0f));
    assert(st.isFresh(1020 + qmi8658::kStaleMs - 1));
    assert(!st.isFresh(1020 + qmi8658::kStaleMs));  // Stale boundary!

    // Rollover freshness check.
    assert(qmi8658::pollSample(bus, st, 0xffffffc0u));
    assert(st.isFresh(0x00000010u));
    assert(!st.isFresh(0x00000200u));

    // Read failures increment errors and eventually require re-probe after threshold.
    bus.failAllReads = true;
    for (unsigned i = 0; i < qmi8658::kMaxConsecutiveErrors; ++i) {
      assert(!qmi8658::pollSample(bus, st, 0x00000300u + i * 10));
    }
    assert(st.errors == qmi8658::kMaxConsecutiveErrors);
    assert(!st.present);
  }

  // 6. BoardAxes regression: reverse both directions after owner reported inverted water.
  {
    auto upright = board_axes::screenDown(-1.0f, 0.0f);
    assert(near(upright.x, 0.0f) && near(upright.y, 1.0f));
    auto right = board_axes::screenDown(0.0f, 1.0f);
    assert(near(right.x, 1.0f) && near(right.y, 0.0f));
    auto left = board_axes::screenDown(0.0f, -1.0f);
    assert(near(left.x, -1.0f) && near(left.y, 0.0f));
    auto inverted = board_axes::screenDown(1.0f, 0.0f);
    assert(near(inverted.x, 0.0f) && near(inverted.y, -1.0f));
  }

  // 7. WaterView simulation, full-height bounds (0..280), stale rejection, and native RGB565 colors.
  {
    WaterView water;
    // Tilt right for 1.5s with fresh IMU -> pool shifts right.
    uint32_t t = 0;
    for (int i = 0; i < 90; ++i) {
      t += 16;
      water.sampleImu(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, t);
      water.step(true, false, 0, 0, t);
    }
    long long sumX = 0;
    for (int i = 0; i < water.pool().count(); ++i) sumX += water.pool().particle(i).x;
    float meanX = float(sumX) / water.pool().count() / 256.0f;
    assert(meanX > 150.0f);  // Pool moved toward right wall (x up to 240).

    // Tilt UP (ax = +1.0 -> down = {0, -1}) -> particles reach top region (y < 48).
    for (int i = 0; i < 180; ++i) {
      t += 16;
      water.sampleImu(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, t);
      water.step(true, false, 0, 0, t);
    }
    int topBeads = 0;
    for (int i = 0; i < water.pool().count(); ++i) {
      const auto& p = water.pool().particle(i);
      assert(p.x >= WaterView::Pool::radius && p.x <= width * 256 - WaterView::Pool::radius);
      assert(p.y >= WaterView::Pool::radius && p.y <= height * 256 - WaterView::Pool::radius);
      if (p.y / 256 < 48) ++topBeads;
    }
    assert(topBeads > 100);

    std::vector<uint16_t> fb(width * height + 2, 0xdead);
    water.render(fb.data() + 1);
    assert(fb.front() == 0xdead && fb.back() == 0xdead);
    int topBright = 0;
    for (int y = 0; y < 48; ++y) {
      for (int x = 0; x < width; ++x) {
        uint16_t c = fb[1 + y * width + x];
        int g = (c >> 5) & 63;
        if (g > 30) ++topBright;
      }
    }
    assert(topBright > 500);  // Beads render into top edge [0, 48).

    // Tilt DOWN (ax = -1.0 -> down = {0, +1}) -> particles reach bottom region (y >= 232).
    for (int i = 0; i < 180; ++i) {
      t += 16;
      water.sampleImu(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, t);
      water.step(true, false, 0, 0, t);
    }
    int bottomBeads = 0;
    for (int i = 0; i < water.pool().count(); ++i) {
      if (water.pool().particle(i).y / 256 >= 232) ++bottomBeads;
    }
    assert(bottomBeads > 100);

    // Now simulate IMU becoming stale -> gravity force stops, pool coasts to sleep.
    for (int i = 0; i < 180; ++i) {
      t += 16;
      water.step(false, false, 0, 0, t);
    }
    assert(water.pool().sleeping());

    // Touch repulsion still works even when IMU is stale, including near top/bottom edges.
    const auto before = water.pool().particle(200);
    int px = before.x / 256;
    int py = before.y / 256;
    t += 16;
    water.step(false, true, px, py, t);
    assert(!water.pool().sleeping());

    // Render into guarded buffer: full 240x280 must be drawn with native RGB565 blue/bead colors.
    std::fill(fb.begin(), fb.end(), uint16_t(0xdead));
    water.render(fb.data() + 1);
    assert(fb.front() == 0xdead && fb.back() == 0xdead);
    int bluePixels = 0;
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        uint16_t c = fb[1 + y * width + x];
        assert(c != 0xdead);
        int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
        if (b > r && (g >= 4 || b >= 4)) ++bluePixels;
      }
    }
    assert(bluePixels > 15000);
  }

  // 8. Telemetry command parser & bounded nonblocking queue.
  {
    SerialCommandParser parser;
    assert(!parser.feed('a'));
    assert(!parser.feed('\n'));
    assert(parser.feed('?'));
    for (char ch : std::string("status\n")) {
      bool triggered = parser.feed(static_cast<uint8_t>(ch));
      assert(triggered == (ch == '\n'));
    }
    for (char ch : std::string("  STATUS \r")) {
      bool triggered = parser.feed(static_cast<uint8_t>(ch));
      assert(triggered == (ch == '\r'));
    }
    // Oversized garbage must not trigger status even if ending with "status\n".
    for (int i = 0; i < 24; ++i) assert(!parser.feed('x'));
    for (char ch : std::string("status\n")) assert(!parser.feed(static_cast<uint8_t>(ch)));
    // Recovery on fresh short command after newline.
    assert(parser.feed('?'));

    TelemetryQueue q;
    MockSerialPort port;
    Interaction ui;
    ui.app = App::Water;
    ui.page = Page::Play;
    Diagnostics d;
    d.touch = true;
    d.frames = 42;
    d.heap = 123456;
    d.imuPresent = true;
    d.imuFresh = true;
    d.imuAddr = 0x6b;
    d.imuWho = 0x05;
    d.imuRev = 0x01;
    d.imuSamples = 100;
    d.imuErrors = 0;
    d.imuAgeMs = 8;
    d.ax = 0.01f;
    d.ay = -0.99f;
    d.az = 0.02f;
    d.downX = 0.99f;
    d.downY = 0.01f;
    d.gravityStrength = 0.99f;
    d.waterShakes = 2;
    d.waterEnergy = 55;

    formatStatusReport(q, ui, d);
    assert(q.pending());
    q.pump(port);  // capacity == 0 -> nonblocking, writes nothing.
    assert(q.pending() && port.output.empty());
    for (int i = 0; i < 64; ++i) {
      port.capacity = 8;
      q.pump(port);
    }
    assert(!q.pending());
    assert(port.output.find("version=0.2.2-dev") != std::string::npos);
    assert(port.output.find("app=WATER") != std::string::npos);
    assert(port.output.find("addr=0x6b") != std::string::npos);
    assert(port.output.find("who=0x05") != std::string::npos);
    assert(port.output.find("frames=42") != std::string::npos);
    assert(port.output.find("heap=123456") != std::string::npos);

    // Disconnect discards pending buffer cleanly.
    q.append("stale");
    port.connected = false;
    q.pump(port);
    assert(!q.pending());
  }

  std::cout << "QMI8658, axes, water viewport, stale rejection and telemetry checks passed\n";
}
