// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ST7789.h>
#include "Qmi8658.h"
#include "Scene.h"
#include "Telemetry.h"

// Waveshare ESP32-S3-Touch-LCD-1.69 ONLY, not LCD-only or round 1.85C.
namespace {
using namespace touch169;

constexpr int LCD_CS = 5, LCD_DC = 4, LCD_RST = 8, LCD_CLK = 6, LCD_MOSI = 7, LCD_BL = 15;
constexpr int TOUCH_SDA = 11, TOUCH_SCL = 10, TOUCH_RST = 13;
constexpr int PWR_SENSE = 40, PWR_HOLD = 41, BAT_ADC = 1;

Adafruit_ST7789 lcd(&SPI, LCD_CS, LCD_DC, LCD_RST);
Interaction ui;
Scene scene;
Diagnostics diagnostics;
qmi8658::ImuState imuState;
SerialCommandParser serialParser;
TelemetryQueue telemetry;
uint16_t* pixels = nullptr;
int lastX = 0, lastY = 0;
bool ready = false;
bool serialRequested = false;

struct WireI2cBus {
  bool readReg(uint8_t addr, uint8_t reg, uint8_t* out, size_t n) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(true) != 0) return false;
    if (Wire.requestFrom(addr, n) != n) {
      while (Wire.available()) Wire.read();
      return false;
    }
    for (size_t i = 0; i < n; ++i) out[i] = Wire.read();
    return true;
  }

  bool writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission(true) == 0;
  }

  void delayMs(uint32_t ms) { delay(ms); }
} wireBus;

Touch sampleTouch() {
  uint8_t p[5] = {};
  bool ok = wireBus.readReg(0x15, 0x02, p, sizeof p);
  if (!ok || (p[0] & 15) > 1) {
    ++diagnostics.errors;
    diagnostics.touch = false;
    diagnostics.touchDown = false;
    return {false, false, 0, 0};
  }
  diagnostics.touch = true;
  bool down = (p[0] & 15) == 1;
  if (down) {
    lastX = ((p[1] & 15) << 8) | p[2];
    lastY = ((p[3] & 15) << 8) | p[4];
  }
  diagnostics.touchDown = down;
  diagnostics.touchX = lastX;
  diagnostics.touchY = lastY;
  return {true, down, lastX, lastY};
}

void sampleImuAndStepWater(uint32_t now) {
  if (imuState.present) {
    if (qmi8658::pollSample(wireBus, imuState, now)) {
      scene.water().sampleImu(imuState.ax, imuState.ay, imuState.az, imuState.gx, imuState.gy,
                              imuState.gz, now);
    }
  } else if (uint32_t(now - imuState.lastProbeMs) >= qmi8658::kRetryIntervalMs) {
    qmi8658::probeAndConfigure(wireBus, imuState, now);
  }
  if (ui.app == App::Water && ui.page == Page::Play) {
    scene.water().step(imuState.isFresh(now), ui.waterTouch, ui.waterX, ui.waterY, now);
  }
}

void syncDiagnostics(uint32_t now) {
  diagnostics.heap = ESP.getFreeHeap();
  diagnostics.imuPresent = imuState.present;
  diagnostics.imuFresh = imuState.isFresh(now);
  diagnostics.imuAddr = imuState.address;
  diagnostics.imuWho = imuState.whoAmI;
  diagnostics.imuRev = imuState.revision;
  diagnostics.imuSamples = imuState.samples;
  diagnostics.imuErrors = imuState.errors;
  diagnostics.imuAgeMs = imuState.ageMs(now);
  diagnostics.ax = imuState.ax;
  diagnostics.ay = imuState.ay;
  diagnostics.az = imuState.az;
  diagnostics.gx = imuState.gx;
  diagnostics.gy = imuState.gy;
  diagnostics.gz = imuState.gz;
  const auto down = scene.water().gravity().down();
  diagnostics.downX = down.x;
  diagnostics.downY = down.y;
  diagnostics.gravityStrength = scene.water().gravity().strength();
  diagnostics.waterShakes = scene.water().motion().sequence;
  diagnostics.waterHeat = scene.water().pool().heat();
  diagnostics.waterEnergy = scene.water().pool().meanEnergy();
  diagnostics.waterSleeping = scene.water().pool().sleeping();
}

void pumpSerial(uint32_t now) {
  static uint32_t reportAt = 0;
  for (int i = 0; i < 32 && Serial.available(); ++i) {
    int ch = Serial.read();
    if (ch >= 0 && serialParser.feed(uint8_t(ch))) {
      serialRequested = true;
    }
  }
  if (Serial && !telemetry.pending() &&
      (serialRequested || uint32_t(now - reportAt) >= 2000)) {
    serialRequested = false;
    reportAt = now;
    syncDiagnostics(now);
    formatStatusReport(telemetry, ui, diagnostics);
  }
  telemetry.pump(Serial);
}

void powerButton(uint32_t now) {
  // Electrical power control only; ignore the initial power-on press.
  static bool armed = false, pressed = false;
  static uint32_t since = 0;
  if (digitalRead(PWR_SENSE) == HIGH) {
    armed = true;
    pressed = false;
    return;
  }
  if (!armed) return;
  if (!pressed) {
    pressed = true;
    since = now;
  }
  if (uint32_t(now - since) >= 2000) {
    digitalWrite(LCD_BL, LOW);
    digitalWrite(PWR_HOLD, LOW);
  }
}
}  // namespace

void setup() {
  pinMode(PWR_HOLD, OUTPUT);
  digitalWrite(PWR_HOLD, HIGH);
  pinMode(PWR_SENSE, INPUT_PULLUP);
  pinMode(42, OUTPUT);
  digitalWrite(42, LOW);  // Keep the unused buzzer quiet.
  Serial.setTxTimeoutMs(0);
  Serial.begin(115200);  // Do not wait for USB: battery operation must start.
  Wire.begin(TOUCH_SDA, TOUCH_SCL, 400000);
  Wire.setTimeOut(15);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(120);
  wireBus.writeReg(0x15, 0xfe, 0x01);  // CST816: disable auto-sleep.
  qmi8658::probeAndConfigure(wireBus, imuState, millis());
  SPI.begin(LCD_CLK, -1, LCD_MOSI, LCD_CS);
  lcd.init(240, 280);
  lcd.setRotation(0);
  lcd.invertDisplay(true);
  lcd.setSPISpeed(40000000);
  lcd.fillScreen(0);
  pinMode(LCD_BL, OUTPUT);
  analogWrite(LCD_BL, 100);
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC, ADC_11db);
  pixels = (uint16_t*)malloc(width * height * sizeof(uint16_t));
  ready = pixels != nullptr;
}

void loop() {
  uint32_t now = millis();
  powerButton(now);
  pumpSerial(now);
  if (!ready) {
    delay(10);
    return;
  }
  static uint32_t inputAt = 0, frameAt = 0, batteryAt = 0;
  static unsigned applied = 0;
  if (uint32_t(now - inputAt) >= 10) {
    inputAt = now;
    ui.update(sampleTouch(), now);
    scene.input(ui, now);
    sampleImuAndStepWater(now);
  }
  if (applied != ui.brightness) {
    applied = ui.brightness;
    analogWrite(LCD_BL, applied);
  }
  if (uint32_t(now - batteryAt) >= 2000) {
    batteryAt = now;
    uint32_t mv = 0;
    for (unsigned i = 0; i < 4; ++i) mv += analogReadMilliVolts(BAT_ADC);
    diagnostics.rawBatteryMv = mv * 3 / 4;
    // ADC is a terminal voltage, not proof that a battery is present. Qualify on hardware first.
    diagnostics.batteryMv = -1;
  }
  if (uint32_t(now - frameAt) >= 33) {
    frameAt = now;
    syncDiagnostics(now);
    scene.render(pixels, ui, diagnostics, now);
    lcd.drawRGBBitmap(0, 0, pixels, width, height);
    ++diagnostics.frames;
  }
  delay(1);
}
