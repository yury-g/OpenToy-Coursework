#pragma once

#include <Adafruit_NeoPixel.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <algorithm>
#include "OrangeShade.h"
#include "Reactions.h"
#include "src/opentoy/DoodlePixels.h"

class OrangeDisplay {
 public:
  void begin(const OrangeShade &shade) {
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, LOW);
    SPI.begin(SCK, MISO, MOSI, TFT_CS);
    tft_.init(135, 240);
    tft_.setRotation(1);
    tft_.setSPISpeed(24000000);
    tft_.fillScreen(background);
    motionPanel_.cp437(true);
    analogWrite(TFT_BACKLITE, 150);

    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
    delay(2);
    pixel_.begin();
    colorize(shade, 24, millis());
  }

  void colorize(const OrangeShade &shade, uint8_t level, uint32_t now) {
    const bool changed = shade.step() != shadeStep_;
    if (changed) {
      shadeStep_ = shade.step();
      ink_ = tft_.color565(255, shade.screenGreen(), 0);
      colorChanged_ = true;
      showShade_ = true;
      shadeAt_ = now;
    }
    if (!changed && level == glow_) return;
    glow_ = level;
    pixel_.setPixelColor(0, level, shade.pixelGreen(level), 0);
    pixel_.show();
  }

  void draw(const FaceState &pose, uint32_t now, uint8_t rotation, int x, int y) {
    animation_.tick(pose, now, true);
    uint8_t pixels[1024];
    drawDoodle(animation_.current, pose.expression, pixels,
               animation_.wiggle(0, now), animation_.wiggle(1, now), animation_.wiggle(2, now));
    const bool rotated = rotation != tft_.getRotation();
    if (rotated) tft_.setRotation(rotation);
    if (frames && !rotated && !colorChanged_ && x == previousX_ && y == previousY_ &&
        memcmp(pixels, previous_, sizeof pixels) == 0) return;
    memcpy(previous_, pixels, sizeof pixels);
    previousX_ = x;
    previousY_ = y;
    ++frames;
    colorChanged_ = false;

    // Like Nearby Buddy, unpack the OpenToy page bitmap into RGB565 scanlines.
    const int width = tft_.width();
    const int height = tft_.height() - motionHeight;
    const bool landscape = width == screenWidth;
    const int faceWidth = landscape ? 178 : 112;
    const int faceHeight = landscape ? 63 : 69;
    const int left = (width - faceWidth) / 2 + x;
    const int top = (height - faceHeight) / 2 + y;
    uint16_t row[screenWidth];
    tft_.startWrite();
    tft_.setAddrWindow(0, 0, width, height);
    for (int dy = 0; dy < height; ++dy) {
      std::fill_n(row, width, background);
      if (dy >= top && dy < top + faceHeight) {
        const int sy = (dy - top) * 64 / faceHeight;
        const uint8_t *page = pixels + (sy / 8) * 128;
        for (int dx = 0; dx < faceWidth; ++dx) {
          if (page[dx * 128 / faceWidth] & (1 << (sy % 8))) row[left + dx] = ink_;
        }
      }
      tft_.writePixels(row, width);
    }
    tft_.endWrite();
  }

  void drawMotion(const MotionSample *sample, uint32_t now) {
    const uint8_t rotation = tft_.getRotation();
    if (rotation == motionRotation_ && now - lastMotionAt_ < 125) return;
    motionRotation_ = rotation;
    lastMotionAt_ = now;

    const int width = tft_.width();
    const bool landscape = width == screenWidth;
    const int textScale = landscape ? 2 : 1;
    const int rightEdges[] = {landscape ? 106 : 79, landscape ? 168 : 105,
                              landscape ? 230 : 131};
    motionPanel_.fillScreen(panelBackground);
    motionPanel_.setTextSize(1);
    motionPanel_.setTextColor(ink_);
    motionPanel_.setCursor(6, 2);
    motionPanel_.print("6DoF");
    for (int axis = 0; axis < 3; ++axis) {
      motionPanel_.setCursor(rightEdges[axis] - 9, 2);
      motionPanel_.write("XYZ"[axis]);
    }
    motionPanel_.setTextColor(panelText);
    motionPanel_.setCursor(6, 14);
    motionPanel_.print("Accel g");
    motionPanel_.setCursor(6, 30);
    motionPanel_.print("Gyro ");
    motionPanel_.write(0xf8);  // Degree symbol in the built-in CP437 font.
    motionPanel_.print("/s");

    const float accel[] = {sample ? sample->x : 0, sample ? sample->y : 0, sample ? sample->z : 0};
    const float gyro[] = {sample ? sample->gx : 0, sample ? sample->gy : 0, sample ? sample->gz : 0};
    motionPanel_.setTextSize(textScale);
    char value[12];
    for (int axis = 0; axis < 3; ++axis) {
      if (sample) snprintf(value, sizeof value, "%+.1f", accel[axis]);
      else strcpy(value, "--");
      motionPanel_.setCursor(rightEdges[axis] - int(strlen(value)) * 6 * textScale,
                             landscape ? 12 : 14);
      motionPanel_.print(value);
      if (sample) snprintf(value, sizeof value, "%+.0f", gyro[axis]);
      motionPanel_.setCursor(rightEdges[axis] - int(strlen(value)) * 6 * textScale,
                             landscape ? 28 : 30);
      motionPanel_.print(value);
    }

    motionPanel_.setTextSize(1);
    motionPanel_.setTextColor(ink_);
    motionPanel_.setCursor(6, 45);
    if (showShade_ && now - shadeAt_ >= 1800) showShade_ = false;
    if (showShade_) {
      motionPanel_.print("Orange ");
      motionPanel_.print(shadeStep_ + 1);
      motionPanel_.print('/');
      motionPanel_.print(OrangeShade::steps);
      motionPanel_.print(" D1-/D2+");
    } else {
      motionPanel_.print(sample ? "Stand up. Turn/shake." : "Check IMU cable");
    }
    uint16_t *buffer = motionPanel_.getBuffer();
    if (!buffer) return;
    tft_.startWrite();
    tft_.setAddrWindow(0, tft_.height() - motionHeight, width, motionHeight);
    for (int row = 0; row < motionHeight; ++row) {
      tft_.writePixels(buffer + row * screenWidth, width);
    }
    tft_.endWrite();
    ++motionFrames;
  }

  unsigned long frames = 0;
  unsigned long motionFrames = 0;

 private:
  static constexpr int screenWidth = 240;
  static constexpr int motionHeight = 54;
  static constexpr uint16_t panelBackground = 0x30a0;
  static constexpr uint16_t panelText = 0xff5a;
  static constexpr uint16_t background = 0x1840;
  uint16_t ink_ = 0xfd20;
  uint32_t shadeAt_ = 0;
  uint8_t shadeStep_ = OrangeShade::standard;
  bool colorChanged_ = false;
  bool showShade_ = false;
  Adafruit_ST7789 tft_{TFT_CS, TFT_DC, TFT_RST};
  Adafruit_NeoPixel pixel_{1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800};
  DoodleAnimator animation_;
  GFXcanvas16 motionPanel_{screenWidth, motionHeight};
  uint32_t lastMotionAt_ = 0;
  uint8_t motionRotation_ = 0xff;
  uint8_t glow_ = 0;
  int previousX_ = 0;
  int previousY_ = 0;
  uint8_t previous_[1024] = {};
};
