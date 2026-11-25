// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <M5Unified.h>
#include <BLEDevice.h>
#include <esp_mac.h>
#include <atomic>
#include <cmath>
#include "Controls.h"
#include "Orientation.h"
#include "OrangeShade.h"
#include "Reactions.h"
#include "src/opentoy/DoodlePixels.h"

// Nesso's M5Unified display and flash-backed initialization exceeds the
// Arduino core's 8 KiB loop-task stack on this ESP32-C6.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

namespace {
constexpr char version[] = "nesso-v0.1.1";
constexpr char nusService[] = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char nusRx[] = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char nusTx[] = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr uint16_t background = 0x1840;
constexpr uint16_t paper = 0xff5a;

LiveFace control;
Orientation orientation;
OrangeShade shade;
Reactions reactions;
DoodleAnimator animation;
M5Canvas face(&M5.Display);
MotionSample motion{};
uint16_t pads = 0;
uint8_t rotation = 1;
bool boardReady = false, imuAvailable = false, imuFresh = false;
bool drawing = false, radioReady = false;
unsigned long frames = 0, imuSamples = 0, touchEvents = 0, buttonEvents = 0, sequence = 0;
uint32_t lastMotion = 0;
char identity[18], deviceName[23];
std::atomic<unsigned> connectionEpoch{0};
std::atomic<bool> connected{false}, restartAdvertising{false};
std::atomic<unsigned> queueDrops{0};
BLECharacteristic *tx = nullptr;
QueueHandle_t incoming = nullptr;
FrameParser usbParser, radioParser;
struct RxChunk { unsigned epoch; uint8_t count; uint8_t bytes[64]; };

class ServerEvents : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    ++connectionEpoch;
    connected = true;
  }

  void onDisconnect(BLEServer *) override {
    connected = false;
    ++connectionEpoch;
    restartAdvertising = true;
  }
} serverEvents;

class ReceivesCommands : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    auto packet = characteristic->getValue();
    RxChunk item{connectionEpoch.load(), 0, {}};
    // A maximal valid frame is 63 bytes plus the terminator.
    if (packet.length() <= sizeof item.bytes) {
      item.count = packet.length();
      memcpy(item.bytes, packet.c_str(), item.count);
    }
    if (xQueueSend(incoming, &item, 0) != pdTRUE) ++queueDrops;
  }
} receivesCommands;

void notifyText(const char *line) {
  // Twenty bytes works at the minimum ATT MTU on every chapter's client.
  while (connected && *line) {
    const size_t bytes = std::min<size_t>(20, strlen(line));
    tx->setValue(reinterpret_cast<const uint8_t *>(line), bytes);
    tx->notify();
    line += bytes;
    if (*line) delay(7);
  }
}

void reply(const char *line, bool ble) {
  if (ble) notifyText(line);
  else if (Serial) Serial.print(line);
}

void sendStatus(bool ble, const char *prefix = "STATUS") {
  char line[237];
  snprintf(line, sizeof line,
           "%s build=%s seq=%lu doodle=1 touch=%d ota=0 local=%d display=%d frames=%lu imu=%d samples=%lu pads=0x%03x events=%lu buttons=%lu rotation=%u shade=%u adv=%d drops=%u\n",
           prefix, version, sequence, control.touchEnabled, !control.remote, boardReady, frames, imuFresh,
           imuSamples, unsigned(pads), touchEvents, buttonEvents, unsigned(rotation),
           unsigned(shade.step()+1), radioReady, queueDrops.load());
  reply(line, ble);
  if (!ble) {
    snprintf(line, sizeof line,
             "MOTION g=%.3f,%.3f,%.3f gyro=%.2f,%.2f,%.2f battery=%ld%%\n",
             motion.x, motion.y, motion.z, motion.gx, motion.gy, motion.gz,
             static_cast<long>(M5.Power.getBatteryLevel()));
    reply(line, false);
  }
}

void handleFrame(const Frame &frame, bool ble) {
  if (frame.kind != TEXT) {
    reply("ERR frame\n", ble);
    return;
  }
  const Result result = control.apply(frame.text);
  if (result == INVALID) {
    reply("ERR command\n", ble);
    return;
  }
  if (result == ACCEPTED) ++sequence;
  sendStatus(ble, result == STATUS_QUERY ? "STATUS" : "OK");
  if (!control.remote) return;
  const auto &s = control.pose;
  char line[151];
  snprintf(line, sizeof line,
           "FACE seq=%lu doodle=1 eye=%d brow=%d mouth=%d skew=%d jaw=%u left=%u right=%u expression=%s\n",
           sequence, s.eyeBend, s.browBend, s.mouthBend, s.mouthSkew, s.jaw, s.left, s.right,
           expressionName(s.expression));
  reply(line, ble);
}

void serviceCommands() {
  static unsigned previousEpoch = 0;
  static unsigned previousDrops = 0;
  const unsigned epoch = connectionEpoch.load();
  if (epoch != previousEpoch) {
    radioParser.reset();
    control.remote = false;
    previousEpoch = epoch;
  }
  const unsigned drops = queueDrops.load();
  if (drops != previousDrops) {
    xQueueReset(incoming);
    radioParser.reset();
    previousDrops = drops;
    reply("ERR queue\n", true);
  }
  if (restartAdvertising.exchange(false)) radioReady = BLEDevice::getAdvertising()->start();
  Frame frame;
  RxChunk packet;
  for (int i = 0; incoming && i < 2 && xQueueReceive(incoming, &packet, 0) == pdTRUE; ++i) {
    if (packet.epoch != epoch || !connected) continue;
    if (!packet.count) {
      radioParser.reset();
      reply("ERR frame\n", true);
      continue;
    }
    for (unsigned j = 0; j < packet.count; ++j) {
      if (radioParser.feed(packet.bytes[j], frame)) handleFrame(frame, true);
    }
  }
  for (int i = 0; i < 64 && Serial.available(); ++i) {
    if (usbParser.feed(Serial.read(), frame)) handleFrame(frame, false);
  }
}

bool startRadio() {
  incoming = xQueueCreate(12, sizeof(RxChunk));
  if (!incoming || !BLEDevice::init(deviceName)) return false;
  auto *server = BLEDevice::createServer();
  server->setCallbacks(&serverEvents);
  auto *information = server->createService(BLEUUID(uint16_t(0x180a)));
  auto *serial = information->createCharacteristic(BLEUUID(uint16_t(0x2a25)), BLECharacteristic::PROPERTY_READ);
  serial->setValue(identity);
  information->start();
  auto *uart = server->createService(nusService);
  tx = uart->createCharacteristic(nusTx, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  auto *rx = uart->createCharacteristic(nusRx, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(&receivesCommands);
  uart->start();
  auto *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(nusService);
  advertising->setScanResponse(true);
  return advertising->start();
}

FaceState localPose(uint32_t now) {
  FaceState pose = restingDoodle();
  pose.mouthBend = 20;
  switch (reactions.mood(now)) {
    case Mood::Idle:
      if (now % 3600 < 170) pose.left = pose.right = 100;
      break;
    case Mood::Smile:
    case Mood::Giggle:
      pose.expression = HAPPY;
      pose.jaw = reactions.mood(now) == Mood::Giggle ? 88 : 60;
      if (reactions.tickle() == Tickle::Left) pose.left = 94;
      else if (reactions.tickle() == Tickle::Right) pose.right = 94;
      else pose.left = pose.right = 45;
      break;
    case Mood::Startled:
      pose.expression = SURPRISED;
      pose.jaw = 90;
      break;
    case Mood::Dizzy:
      pose.expression = NERVOUS;
      pose.mouthSkew = int(55 * sinf(float(now % 600) * .010472f));
      pose.left = 40;
      break;
  }
  return pose;
}

void emitTouch(uint32_t now) {
  if (!connected) return;
  const FaceState pose = localPose(now);
  char line[100];
  snprintf(line, sizeof line, "EVENT doodle %d %d %d %d %u %u %u %s\n",
           pose.eyeBend, pose.browBend, pose.mouthBend, pose.mouthSkew,
           pose.jaw, pose.left, pose.right, expressionName(pose.expression));
  notifyText(line);
}

void updateInputs(uint32_t now) {
  M5.update();
  const bool darker = M5.BtnA.wasPressed();
  const bool lighter = M5.BtnB.wasPressed();
  buttonEvents += unsigned(darker) + unsigned(lighter);
  shade.sample(now, (unsigned(M5.BtnA.isPressed()) << 1) | (unsigned(M5.BtnB.isPressed()) << 2));

  if (imuAvailable && M5.Imu.update()) {
    const auto data = M5.Imu.getImuData();
    const auto &a = data.accel;
    const auto &g = data.gyro;
    motion = {a.x, a.y, a.z, sqrtf(g.x*g.x + g.y*g.y + g.z*g.z), g.x, g.y, g.z};
    lastMotion = now;
    ++imuSamples;
    orientation.sample(now, motion.x, motion.y, motion.z, motion.spin);
  }
  imuFresh = imuSamples && now - lastMotion < 250;
  uint16_t next = 0;
  if (control.touchEnabled) {
    for (unsigned index = 0; index < M5.Touch.getCount(); ++index) {
      const auto &point = M5.Touch.getDetail(index);
      if (point.isPressed()) next |= touchZone(point.x, M5.Display.width());
    }
  }
  reactions.sample(now, next, imuFresh ? &motion : nullptr);
  if (next != pads && !control.remote) {
    if (next) {
      ++touchEvents;
      emitTouch(now);
    } else if (connected) {
      notifyText("EVENT doodle 0 0 20 0 0 0 0 neutral\n");
    }
  }
  pads = next;
}

void drawFace(uint32_t now) {
  if (!drawing) return;
  const auto nextRotation = orientation.rotation();
  if (nextRotation != rotation) {
    rotation = nextRotation;
    M5.Display.setRotation(rotation);
    M5.Display.fillScreen(background);
  }
  FaceState pose = control.remote ? control.pose : localPose(now);
  animation.tick(pose, now, true);
  uint8_t pixels[1024];
  drawDoodle(animation.current, pose.expression, pixels, animation.wiggle(0, now),
             animation.wiggle(1, now), animation.wiggle(2, now));
  face.fillSprite(background);
  const auto ink = M5.Display.color565(255, shade.screenGreen(), 0);
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 128; ++x) {
      if (pixels[y / 8 * 128 + x] & (1 << (y % 8))) face.drawPixel(x, y, ink);
    }
  }
  const int width = M5.Display.width();
  const int height = M5.Display.height();
  face.pushSprite((width - 128) / 2, (height - 86) / 2);
  M5.Display.setTextColor(paper, background);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(5, height - 24);
  M5.Display.printf("OPEN TOY / NESSO  %s ", control.remote ? "BLE" : "   ");
  M5.Display.setCursor(5, height - 12);
  M5.Display.printf("Touch / turn / shake ");
  ++frames;
}
}  // namespace

void setup() {
  Serial.begin(115200);
  auto config = M5.config();
  config.internal_imu = true;
  config.internal_spk = false;
  config.internal_mic = false;
  M5.begin(config);
  boardReady = M5.getBoard() == m5::board_t::board_ArduinoNessoN1 &&
               M5.Display.width() > 0;
  imuAvailable = boardReady && M5.Imu.isEnabled();
  if (!boardReady) {
    Serial.println("ERR Nesso board autodetection");
    return;
  }
  M5.Display.setRotation(rotation);
  M5.Display.setBrightness(115);
  M5.Display.fillScreen(background);
  face.setColorDepth(16);
  drawing = face.createSprite(128, 64) != nullptr;
  uint8_t mac[6]{};
  if (esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY) == ESP_OK) {
    snprintf(identity, sizeof identity, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    snprintf(deviceName, sizeof deviceName, "OpenToy Nesso %02X%02X", mac[4],mac[5]);
    radioReady = startRadio();
  }
  sendStatus(false);
}

void loop() {
  static uint32_t lastFrame = 0;
  static uint32_t lastStatus = 0;
  const uint32_t now = millis();
  if (boardReady) {
    updateInputs(now);
    serviceCommands();
    if (now - lastFrame >= 42) {
      lastFrame = now;
      drawFace(now);
    }
  }
  if (now - lastStatus >= 2000) {
    lastStatus = now;
    sendStatus(false);
  }
  delay(3);
}
