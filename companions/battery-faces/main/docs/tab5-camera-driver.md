# Tab5 onboard camera driver

`firmware/FaceMonitor/TabCamera.h` implements a real ESP32-P4 camera path using the native CSI and ISP libraries already bundled and linked in Arduino-ESP32 **3.3.10 / ESP-IDF 5.5.4**. It does not require adding `esp_video`, `esp_cam_sensor`, V4L2, M5Unified or a new ESP-IDF project. Its sensor mode table is Apache-2.0 code from the manufacturer's pinned source; the copyright and modification notice remain in the header. See `LICENSES/Apache-2.0.txt`.

The parent hardware check observed PID`0xeb52`, a24MHz configured clock and fresh completed image captures on the connected Tab5. The full end-to-end acceptance record is retained in `outputs/camera-family-v17-live.json`. A fresh frame is returned only after a completed application-buffer DMA transaction; the implementation does not generate substitute pictures.

Sensor address is defined in the manufacturer’s [public SC202CS header](https://github.com/m5stack/M5Tab5-UserDemo/blob/b4e356bc491ca070d54004718dad789c07d5fc93/platforms/tab5/components/esp_cam_sensor/sensors/sc202cs/include/sc202cs.h). Do not substitute a common SmartSens address.

## Verified source findings

The [M5Stack Tab5 demo](https://github.com/m5stack/M5Tab5-UserDemo/tree/b4e356bc491ca070d54004718dad789c07d5fc93) names its driver `SC202CS`, but its [720p sensor table](https://github.com/m5stack/M5Tab5-UserDemo/blob/b4e356bc491ca070d54004718dad789c07d5fc93/platforms/tab5/components/esp_cam_sensor/sensors/sc202cs/private_include/sc202cs_settings.h) explicitly identifies the mode as SC2356. The [corresponding driver](https://github.com/m5stack/M5Tab5-UserDemo/blob/b4e356bc491ca070d54004718dad789c07d5fc93/platforms/tab5/components/esp_cam_sensor/sensors/sc202cs/sc202cs.c) expects PID **0xeb52** from 16-bit SCCB registers **0x3107 / 0x3108**, at 7-bit address **0x36**. Consequently the status model is `SC2356/SC202CS` when that ID is actually observed; software naming alone does not identify a different physical sensor.

The pinned manufacturer configuration uses 1280×720, RAW8 BGGR, **one CSI lane at 576 Mbps**, and a 24 MHz reference clock on **GPIO 36**. Its [BSP implementation](https://github.com/m5stack/M5Tab5-UserDemo/blob/b4e356bc491ca070d54004718dad789c07d5fc93/platforms/tab5/components/m5stack_tab5/m5stack_tab5.c) supplies that clock. The [current official Espressif Tab5 BSP](https://github.com/espressif/esp-bsp/blob/f259a6abd5808019b1903a422092a4234ca95a51/bsp/m5stack_tab5/include/bsp/m5stack_tab5.h) also names SC202CS and identifies camera enable/reset as **expander 0x43 bit 6**. These are references, not authority to run the BSP's unrelated board initialization.

The installed SDK includes `esp_driver_cam`, CSI headers, `esp_driver_isp`, clock output, clock tree, cache and FreeRTOS APIs. `flags/ld_libs` already links their libraries. The direct integration follows [Espressif's CSI/ISP example](https://github.com/espressif/esp-idf/blob/v5.5.4/examples/peripherals/camera/mipi_isp_dsi/main/mipi_isp_dsi_main.c) and the actual [CSI implementation](https://github.com/espressif/esp-idf/blob/v5.5.4/components/esp_driver_cam/csi/src/esp_cam_ctlr_csi.c), while keeping the existing M5GFX display active.

## Display, charger and clock coexistence

Call `begin()` **after a successful existing display initialization**. The existing display owns the shared MIPI PHY LDO. In this checkout M5GFX requests LDO channel 3 at **2500 mV**; the driver neither acquires nor adjusts nor releases that rail. It does not initialize or reset DSI, change brightness, initialize M5Unified power, access charger registers, or access expander **0x44** at all. The hosted C6 Wi-Fi firmware is untouched.

Camera enable uses read/modify/write only on **bit 6 of expander 0x43**, skipping writes if its output level/direction/impedance already match. Other bits are preserved and the result is read back. Sensor configuration writes go only to the positively identified sensor at 0x36. An unknown ID fails before the mode table is written.

A clock issue discovered during the port matters: the P4 LEDC peripheral shares a global source across its timers. Arduino's display backlight normally uses the 40 MHz XTAL, which cannot generate a 24 MHz one-bit PWM; selecting an 80 MHz PLL for another LEDC timer would conflict with the running display. This implementation therefore does **not use LEDC**. It uses [the public clock-output API](https://github.com/espressif/esp-idf/blob/v5.5.4/components/esp_hw_support/esp_clock_output.c) to route the existing 480 MHz SPLL through a divider of 20 to GPIO 36. The existing PLL's source and frequency remain unchanged. GPIO 36 is held as an input until the route and divider are ready, then released, so an undivided clock is not exposed at the pad. Only that camera clock route is released on driver shutdown. No pre-display hook or display-clock change is required.

## API

```cpp
#include "TabCamera.h"
TabCamera tabCamera;                       // global/static owner; do not copy
uint8_t localCameraPixels[4800];

// After existing display.init() succeeds:
bool initialized = tabCamera.begin();

// From one non-UI worker, at the chosen still-image cadence:
if (tabCamera.capture(localCameraPixels)) {
    uint32_t capturedAt = tabCamera.lastCaptureAt();
    // Publish these 4,800 RGB332 bytes as the local camera source.
}
```

`begin()` is idempotent while ready. It programs the verified 720p mode, allocates CSI/ISP resources and leaves the sensor in standby with this camera's CSI stopped. `capture(uint8_t *out, size_t capacity=4800, uint32_t timeoutMs=450)` starts only the camera CSI, queues a fresh application buffer, then starts sensor streaming. The first full sensor frame goes into the separate driver backup buffer and is discarded. The next full frame completes the requested buffer and signals the callback. The sensor and camera CSI return to standby immediately; the display and shared PHY remain active. No old application frame is queued between requests or reused after a timeout. The output array is valid only when the call returns true.

The DMA-completion callback records `esp_timer_get_time()/1000`, the same uptime timebase as Arduino `millis()`. `lastCaptureAt()` and `framesCaptured()` advance only after the application frame completes, passes size/freshness checks and converts successfully. The driver checks the completed frame is less than **500 ms old**. Capture wait is capped at 450 ms, with a 20 ms queue timeout; normal capture needs about two 30 fps sensor frames plus control overhead. Use a non-UI worker, as initialization and error paths also perform synchronous I2C work.

ISP output is native little-endian RGB565. The driver samples it into **80×45** pixels and places those pixels in rows **7–51** of an **80×60** RGB332 image; the seven top and eight bottom rows are black. This preserves the 16:9 field of view. There is no stretch, crop or assumed optical rotation. The physical board test should determine whether any orientation adjustment is wanted. Color/exposure use the fixed manufacturer mode and baseline native ISP conversion; this small port does not include the manufacturer's automatic exposure/white-balance application stack.

Status accessors:

| API | Meaning |
| --- | --- |
| `ready()` | Camera resources initialized, available for capture |
| `model()` | `SC2356/SC202CS` only after observing 0xeb52; otherwise `unidentified` |
| `chipId()` | Last observed raw 16-bit sensor ID |
| `error()` | Last `esp_err_t` result |
| `stage()` | Named initialization/capture stage, including a specific failure stage |
| `clockHz()` | Configured camera reference frequency, 24,000,000 while owned |
| `framesCaptured()` | Successfully returned fresh stills |
| `captureErrors()` | Capture queue, timeout, streaming, DMA/cache or conversion failures |
| `lastCaptureAt()` | Milliseconds at the last successful application-buffer completion |
| `end()` | Release own sensor streaming, CSI/ISP, DMA buffers and camera clock route |
| `static readChipId(uint16_t &id)` | Read-only register identification using the existing M5GFX bus 1 |

`readChipId()` does not initialize a bus or modify power, clocks, resets, sensor configuration or charger state. The sensor must already be released/clocked to respond reliably; an unsuccessful unclocked read is not proof of a missing camera. `begin()` configures the camera clock before identification and preserves the observed raw ID even when it is unsupported.

Keep one owner and serialize `begin`, `capture`, `end` and status snapshots in that worker. A global/static object keeps its semaphore/ISR state in internal RAM; framebuffer pixels live in PSRAM. The callback signals only this driver's application buffer, not the backup buffer. A timeout releases its resources and permits a later explicit `begin()` retry, preventing a late completed transaction from being mistaken for a new request.

## Memory and build

The native application and backup buffers each occupy 1280×720×2 = **1,843,200 bytes**, approximately **3.69 MB** total PSRAM before controller metadata. Application DMA allocation aligns to this installed P4 profile's **128-byte L2 cache line**. It explicitly synchronizes DMA memory back to CPU cache before conversion. This fits the verified Tab5 32 MB PSRAM alongside the existing display/canvas allocations; the physical board now captures images alongside its display and hosted Wi-Fi; see the end-to-end record.

No extra component build flags are needed. The established profile remains:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:m5stack_tab5:ChipVariant=prev3,PSRAM=enabled,USBMode=hwcdc,CDCOnBoot=cdc \
  --library vendor/M5GFX --build-path /tmp/build-tab-camera-probe \
  vendor/tab5-camera-probe
```

The ignored probe includes explicit Tab5 M5GFX display initialization and this camera API. It is a build/link test, not an installer. It must not be used to replace the working monitor. No USB, flash, reset, partition-table change or C6 modem update was performed in this driver task.

The ignored `vendor/tab5-camera-tests/letterbox_test.cpp` tests the real conversion using an independently constructed native RGB565 fixture: missing input, short buffers, preserved aspect ratio, all black bars, known RGB primaries, bottom-right image sample and output bounds. It was observed failing against a stub before implementation, then passing with warnings treated as errors. Hardware-dependent stages are verified by the compiled probe and must additionally be exercised on the board. Source snapshots and pinned tree metadata remain in ignored `vendor/tab5-camera-reference/` for inspection.

## Physical acceptance and limits

Use the actual board's status to establish PID 0xeb52, clock 24 MHz, successful fresh still counts and absence of timeouts. Compare a known color/orientation target and confirm content changes between requests, including after a pause. Keep the existing display visible, verify it remains responsive at unchanged brightness, confirm Wi-Fi still transports the family data, and check the charger control bits remain unchanged. If the observed sensor ID differs, preserve its value and obtain the correct manufacturer's register table; this implementation deliberately does not guess a different sensor configuration.

The archived actual native frame shows a green color cast under the available lighting. The port intentionally does not claim calibrated colors, automatic white balance or verified optical orientation. XIAO remains the default feed. The end-to-end record distinguishes serial page selections/gesture unit tests from physical touch tests; no new owner touch verification is inferred.
