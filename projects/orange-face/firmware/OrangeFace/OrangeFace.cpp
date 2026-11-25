#include "Face.h"
#include "Inputs.h"
#include "Orientation.h"

static OrangeDisplay face;
static Inputs inputs;
static Reactions reactions;
static Orientation orientation;
static OrangeShade shade;

// D0 doubles as BOOT on reset. D1 and D2 are active high.
static constexpr uint8_t resetShadePin = 0;
static constexpr uint8_t deeperShadePin = 1;
static constexpr uint8_t lighterShadePin = 2;

static const char *moodName(Mood mood) {
  switch (mood) {
    case Mood::Smile: return "smile";
    case Mood::Startled: return "startled";
    case Mood::Dizzy: return "dizzy";
    case Mood::Giggle: return "giggle";
    default: return "idle";
  }
}

static void animate(Mood mood, uint32_t now) {
  FaceState pose = restingDoodle();
  pose.mouthBend = 22;
  uint8_t glow = 24;
  int x = reactions.gazeX();
  int y = reactions.gazeY();
  const uint8_t rotation = orientation.rotation();
  const int landscapeX = x;
  switch (rotation) {
    case 0: x = -y; y = landscapeX; break;
    case 2: x = y; y = -landscapeX; break;
    case 3: x = -x; y = -y; break;
  }
  const float wobble = sinf(float(now % 600) * 0.010472f);
  switch (mood) {
    case Mood::Idle:
      pose.browBend = abs(x) * 2;
      if (now % 3600 < 210) pose.left = pose.right = 100;
      break;
    case Mood::Smile:
    case Mood::Giggle: {
      const bool extra = mood == Mood::Giggle;
      const float laugh = sinf(float(now % 410) * 0.015325f);
      pose.expression = HAPPY;
      pose.jaw = unsigned(60 + laugh * 27);
      y += int(laugh * (extra ? 7 : 4));
      glow = uint8_t((extra ? 70 : 50) + laugh * 13);
      switch (reactions.tickle()) {
        case Tickle::Left:
          pose.left = 95;
          pose.browBend = 32;
          pose.mouthSkew = -44 + int(laugh * 12);
          x -= 5;
          break;
        case Tickle::Middle:
          pose.left = pose.right = unsigned(48 + laugh * 36);
          pose.eyeBend = -22;
          pose.browBend = -31;
          pose.jaw = unsigned(70 + laugh * 28);
          break;
        case Tickle::Right:
          pose.right = 95;
          pose.browBend = 32;
          pose.mouthSkew = 44 + int(laugh * 12);
          x += 5;
          break;
        case Tickle::AllOver:
          pose.left = unsigned(50 + laugh * 48);
          pose.right = unsigned(50 - laugh * 48);
          pose.mouthSkew = int(laugh * 62);
          x += int(laugh * 7);
          glow += 10;
          break;
      }
      break;
    }
    case Mood::Startled:
      pose.expression = SURPRISED;
      pose.browBend = -20;
      pose.jaw = 90;
      glow = 65;
      break;
    case Mood::Dizzy:
      pose.expression = NERVOUS;
      pose.mouthSkew = int(wobble * 55);
      pose.left = 42;
      x = int(wobble * 10);
      y = int(wobble * 5);
      glow = 36;
      break;
  }
  face.colorize(shade, glow, now);
  face.draw(pose, now, rotation, constrain(x, -11, 11), constrain(y, -9, 9));
  face.drawMotion(inputs.motionOk ? &inputs.motion : nullptr, now);
}

void setup() {
  Serial.begin(115200);
  pinMode(resetShadePin, INPUT_PULLUP);
  pinMode(deeperShadePin, INPUT_PULLDOWN);
  pinMode(lighterShadePin, INPUT_PULLDOWN);
  inputs.begin();
  face.begin(shade);
}

void loop() {
  static uint32_t lastSample = 0;
  static uint32_t lastFrame = 0;
  static uint32_t lastStatus = 0;
  static uint16_t lastPads = 0;
  static Mood lastMood = Mood::Idle;
  static uint8_t lastRotation = 1;
  static uint8_t lastShade = OrangeShade::standard;
  const uint32_t now = millis();
  const uint8_t pressed = (digitalRead(resetShadePin) == LOW) |
                          ((digitalRead(deeperShadePin) == HIGH) << 1) |
                          ((digitalRead(lighterShadePin) == HIGH) << 2);
  shade.sample(now, pressed);
  if (now - lastSample >= 20) {
    lastSample = now;
    inputs.poll();
    if (inputs.motionOk) {
      const auto &motion = inputs.motion;
      orientation.sample(now, motion.x, motion.y, motion.z, motion.spin);
    }
    reactions.sample(now, inputs.pads, inputs.motionOk ? &inputs.motion : nullptr);
  }
  const Mood mood = reactions.mood(now);
  if (now - lastFrame >= 33) {
    lastFrame = now;
    animate(mood, now);
  }
  if (Serial && (now - lastStatus >= 1000 || inputs.pads != lastPads ||
                 mood != lastMood || orientation.rotation() != lastRotation || shade.step() != lastShade)) {
    lastRotation = orientation.rotation();
    lastShade = shade.step();
    lastStatus = now;
    lastPads = inputs.pads;
    lastMood = mood;
    inputs.printScan();
    Serial.printf("Orange Face v0.4 frames=%lu motionFrames=%lu mood=%s rotation=%u pixel=%d power=%d orange=%u buttons=0x%x "
                  "touch=0x%02x/%d pads=0x%03x oor=0x%03x imu=0x%02x/%d zone=%u "
                  "g=%.2f,%.2f,%.2f gyro=%.1f,%.1f,%.1f spin=%.1f gaze=%d,%d\n",
                  face.frames, face.motionFrames, moodName(mood), unsigned(lastRotation),
                  PIN_NEOPIXEL, digitalRead(NEOPIXEL_POWER), unsigned(lastShade + 1), unsigned(pressed),
                  unsigned(inputs.touchAddress), inputs.touchOk, unsigned(inputs.pads), unsigned(inputs.touchOutOfRange),
                  unsigned(inputs.imuAddress), inputs.motionOk, unsigned(reactions.tickle()),
                  inputs.motion.x, inputs.motion.y, inputs.motion.z,
                  inputs.motion.gx, inputs.motion.gy, inputs.motion.gz, inputs.motion.spin,
                  reactions.gazeX(), reactions.gazeY());
  }
  delay(1);
}
