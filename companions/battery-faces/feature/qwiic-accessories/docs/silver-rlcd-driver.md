# Silver reflective LCD driver

`firmware/FeatherFace/SilverRlcd.h` is an isolated display driver for the Waveshare ESP32-S3-RLCD-4.2. It accepts a **200×150 landscape RGB565 canvas** and scales it uniformly by **2× to 400×300**, filling the reflective display without margins, stretch or crop. Every nonzero source pixel becomes black ink; zero becomes white. The proven ST7305 **300×400 portrait RAM mode, 15 KB transfer and initialization remain unchanged**; only the image coordinates are rotated into that native mode. The logical input occupies 60 KB rather than allocating a native-size RGB565 image.

The implementation adapts the manufacturer's [Arduino LVGL display driver](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/blob/d8bf7813d3b53251577ff03c03596ad6ad3acae0/examples/arduino/examples/09_LVGL_V9_Test/display_bsp.cpp), pinned to commit `d8bf7813d3b53251577ff03c03596ad6ad3acae0`. Its source repository [LICENSE](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/blob/d8bf7813d3b53251577ff03c03596ad6ad3acae0/LICENSE) is **Apache-2.0, Copyright 2026 Waveshare**. That attribution, license identifier, pinned source and modification notice remain in the new header. Retain `LICENSES/Apache-2.0.txt` when distributing it. The family's other original source keeps its own license.

## Verified hardware

The [factory configuration](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/blob/d8bf7813d3b53251577ff03c03596ad6ad3acae0/examples/factory/main/user_config.h), [standalone U8g2 example](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/blob/d8bf7813d3b53251577ff03c03596ad6ad3acae0/examples/arduino/examples/10_U8G2_Test/10_U8G2_Test.ino) and [manufacturer schematic](https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf) agree on the display wiring:

| Function | GPIO | Driver behavior |
| --- | ---: | --- |
| LCD SCK | 11 | SPI clock |
| LCD MOSI/SDA | 12 | SPI output |
| LCD CS | 40 | Active-low, manually controlled |
| LCD DC/RS | 5 | Low for command, high for data |
| LCD RESET | 41 | Manufacturer high/low/high reset sequence |
| LCD TE | 6 | Unused; no input/pull/interrupt setup |

The panel has two monochrome states and no backlight. The driver creates a dedicated **SPI3_HOST / Arduino HSPI** instance, matching the manufacturer's host. It uses SPI mode 0, MSB first and **10 MHz**, the official LVGL driver rate. The newer U8g2 example uses 24 MHz, but a higher clock is not required for this application's capped paint cadence.

The schematic was visually inspected, including its power section. A separate **ECJ23001-4FCBD6** pushbutton controller drives the power FET through the `OUTH` net; that path has no MCU power-hold/enable GPIO. The standalone manufacturer display example likewise performs no power-hold initialization. Consequently this driver configures only the five LCD pins and asserts no guessed power pin.

Related board inputs, for integration outside this display class:

| Function | Verified source | Meaning |
| --- | --- | --- |
| Battery ADC | Schematic `BAT_ADC → GPIO4`; official ADC example uses ADC1 channel 3 | R21=200 kΩ over R23=100 kΩ, so calibrated ADC pin voltage ×3 gives battery voltage |
| KEY | Factory `button_bsp.c`, GPIO18 | Active low with pull-up |
| BOOT | Factory `button_bsp.c`, GPIO0 | Active low with pull-up; also the chip's boot strap |
| Physical power button | Schematic Key3 → dedicated power controller | Separate from KEY18/BOOT0 |

The display driver performs no ADC, button, audio, microphone, SD, RTC, I2C or charger initialization. The board's battery voltage can support a clearly labeled voltage estimate; it does not establish a current measurement or fuel-gauge percentage.

## Native byte format and initialization

The native framebuffer is **15,000 bytes**. Each byte describes four adjacent columns across two adjacent rows. For native portrait pixel `(px,py)`:

```text
byte index = (py / 2) × 75 + (px / 4)
bit number = 7 - (2 × (px % 4) + (py % 2))
1 = white, 0 = black
```

The 400×300 landscape raster maps into that storage as `px = 299 - yLandscape`, `py = xLandscape`. Its top-left pixel therefore lands at native `(299,0)`; landscape right advances down native rows and landscape down advances left across native columns. Each logical pixel covers exactly 2×2 native pixels. This rotates the content clockwise within the portrait buffer, so the physical board is viewed a quarter-turn counterclockwise from its established portrait orientation.

This is the direct portrait mapping in the manufacturer's non-LUT implementation. The new driver avoids the large per-pixel lookup tables and all LVGL/U8g2 dependencies. It preserves the complete manufacturer sequence of **27 commands / 58 parameter bytes**, including hardware reset high 50 ms, low 20 ms, high 50 ms, and the LVGL example's **200 ms** wait after Sleep Out (`0x11`). It retains memory-access control `0x36=0x48`, pixel format `0x3A=0x11`, inversion command `0x21`, column window `0x2A: 0x12..0x2A`, page window `0x2B: 0x00..0xC7`, and the other manufacturer voltage/timing settings. It does not invent or tune panel voltages.

Every presentation resets those same native windows, sends command `0x2C`, and writes exactly 15,000 bytes synchronously. At 10 MHz the raw pixel transfer takes about 12 ms before software overhead; `lastTransferMicros()` reports the measured whole transfer. A white frame is sent once during successful `begin()`.

## API and integration

```cpp
#include "SilverRlcd.h"
SilverRlcd silver;

bool initialized = silver.begin();
// Logical landscape canvas: 200*150 native RGB565 words, row major:
bool sent = silver.present(logicalRgb565);
```

`WIDTH=400`, `HEIGHT=300`, `LOGICAL_WIDTH=200`, and `LOGICAL_HEIGHT=150` describe the landscape output and input. `NATIVE_WIDTH=300` and `NATIVE_HEIGHT=400` describe unchanged RAM coordinates.

`begin()` returns false if SPI initialization fails and is idempotent once ready. `present(const uint16_t *logical, size_t pixels=30000)` returns false before initialization or for null/short input. Source storage must remain valid until the synchronous call returns. The class owns its packed 15 KB buffer; it uses no dynamic image allocation. Keep a single owner and call it from one task. Do not initialize another driver or SD interface on the same SPI host.

`ready()` and `stage()` describe software/bus initialization. This write-only SPI wiring does not read a controller ID or acknowledge panel reception, so successful initialization alone is not proof of a visible image. Diagnostics include `framesPresented()`, `lastPresentAt()` in milliseconds and `lastTransferMicros()`. The startup white frame is included in the presentation counter.

The caller caps the draw rate at **5 frames/s or lower**. For simple expression changes or blinking, a slower cadence is appropriate. There is no hidden driver throttle and no asynchronous queue of old images.

## Verification and build

`tests/silver_rlcd_test.cpp` was updated before the landscape encoder and observed failing against the previous portrait implementation. It then passed with `-Wall -Wextra -Werror`, checking:

- Hand-derived rotated ST7305 byte values for each corner and an interior sample, tested independently with no production helper used for expected values.
- Full black-ink coverage of all 400×300 pixels, with no retained portrait margins.
- Nonzero red, green, blue, white and minimum-value RGB565 pixels all becoming black.
- Exactly four native ink pixels per logical sample, uniform 2× mapping, all orientation boundaries, untouched output on invalid input and sentinels outside the native buffer.

The isolated, ignored probe in `vendor/silver-probe` compiles with the installed ESP32 Arduino core **3.3.10**; it needs only the core's SPI library:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB \
  --build-path /tmp/battery-faces-silver-landscape-probe-build vendor/silver-probe
```

This command compiles only. No USB access, board reset, flashing, partition/bootloader changes or peripheral actions were performed by the driver task. The generic N16R8 build profile does not authorize replacing the board's existing partition layout. Source snapshots, license and schematic render remain in ignored `vendor/silver-reference/`.

The owner confirmed the earlier portrait driver was physically working. Landscape acceptance remains with the integration workflow: check that the image is upright in the intended orientation, fills the 400×300 display, remains correctly polarized, retains expression proportions, supports key navigation, and keeps the measured draw rate capped. If the physical orientation differs, adjust it deliberately using the manufacturer's verified mapping rather than guessing another panel initialization.
