# Source and licenses

`PanelInit.h` adapts only the exact ST7305 initialization method from Waveshare's `examples/arduino/examples/10_U8G2_Test/ST7305_U8g2.cpp`, commit [9e754fdf431ef4fce21e4e8b19bd4079ba043e86](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/tree/9e754fdf431ef4fce21e4e8b19bd4079ba043e86), Apache-2.0 (`LICENSE.vendor`). The class/method name changes to our synchronous SPI adapter. Register values and delays are retained. The transport runs at 10 MHz; the vendor U8g2 example uses 24 MHz.

`Display.h` implements the vendor-documented landscape packing (also in the same revision's Arduino `09_LVGL_V9_Test/display_bsp.cpp`) with explicit bounds checks and a renderer adapter. `Board.cpp` uses their exact board pin assignments and calibrated ADC divider ratio; it does not use their linear battery-percentage estimate.

Eyes behavior/geometry, personalities and bounded telemetry are staged by the build from existing OpenToy sources in `projects/animatronic-eyes`; they are not forked here. Original upstream renderer files and their existing license/provenance remain unchanged. Text uses Adafruit GFX 1.12.6 (BSD), installed separately. Arduino-ESP32 is pinned at 3.3.10 in the build instructions. This target introduces no radio keys or network service.

`NativeEyes.h` is an MIT-licensed target presentation adapter derived from the geometric design in OpenToy `EyePixels.h` and `Companion.h`; it evaluates continuous shapes at native pixel centers rather than invoking their historical low-resolution rasterizer. Canonical behavior, source history and existing renderer files remain unchanged.
