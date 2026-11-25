// SPDX-License-Identifier: MIT
#include "ParallelPanel.h"

#if !defined(ARDUINO_UNO_Q) || !defined(OPENTOY_UNOQ_MAR2808)
#error "Build for Arduino UNO Q only after verifying a compatible 3.3 V MAR2808-style shield."
#endif

#if !defined(OPENTOY_UNOQ_DIAGNOSTIC)
#include "UnoQEyesApp.h"
#endif

namespace {
using opentoy::uno_q::ParallelPanel;
ParallelPanel panel;

#if defined(OPENTOY_UNOQ_DIAGNOSTIC)
void drawPattern() {
  constexpr uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
  panel.startPixels(0, 0, ParallelPanel::kWidth, ParallelPanel::kHeight);
  for (int y = 0; y < ParallelPanel::kHeight; ++y) {
    for (int x = 0; x < ParallelPanel::kWidth; ++x) {
      const auto color = colors[x / 80];
      const bool border = x < 6 || x >= 314 || y < 6 || y >= 234;
      const bool marker = x >= 12 && x < 48 && y >= 12 && y < 28;
      panel.pixel(border || marker ? 0x0000 : color);
    }
  }
  panel.endPixels();
}
#else
using namespace opentoy::uno_q;

// Fit the framebuffer and sketch code in the original loader's 128 KiB heap.
constexpr uint16_t kRenderWidth = 240;
constexpr uint16_t kRenderHeight = 180;
uint16_t pixels[size_t(kRenderWidth) * kRenderHeight];
UnoQEyesApp eyes(kRenderWidth, kRenderHeight);
bool panelReady;
#endif
}  // namespace

void setup() {
#if defined(OPENTOY_UNOQ_DIAGNOSTIC)
  if (panel.begin()) {
    drawPattern();
  }
#else
  panelReady = panel.begin();
#endif
}

void loop() {
#if defined(OPENTOY_UNOQ_DIAGNOSTIC)
  delay(1000);
#else
  if (!panelReady) {
    delay(1000);
    return;
  }
  const uint32_t now = millis();
  eyes.render(eyes.step(now), now, pixels);
  panel.startPixels(0, 0, ParallelPanel::kWidth, ParallelPanel::kHeight);
  for (uint16_t y = 0; y < ParallelPanel::kHeight; ++y) {
    const auto* row = pixels + size_t(y * 3 / 4) * kRenderWidth;
    for (uint16_t x = 0; x < ParallelPanel::kWidth; ++x) {
      panel.pixel(row[x * 3 / 4]);
    }
  }
  panel.endPixels();
  delay(1);
#endif
}
