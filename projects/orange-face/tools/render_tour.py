#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export the Orange Face README tour from synthetic inputs and the firmware renderer."""
import argparse
import html
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / "firmware/OrangeFace"
OUTPUT = ROOT / "docs/images/orange-face-tour.svg"
STEP_MS = 120
FRAMES_PER_SCENE = 12

# title, two caption lines, IMU pose (g, deg/s), pads, button (pressed masks)
SCENES = [
    ("Blink and rest", "The TFT shows six live readings.", "Gravity is visible even at rest.", 0, 0, -1, 0, 0, 0),
    ("Turn to portrait", "Hold it upright and turn slowly.", "The face follows gravity.", 0, -1, 0, 0, 0, 0),
    ("Turn it upside down", "Turn another quarter-turn.", "The screen can use all four edges.", -1, 0, 0, 0, 0, 0),
    ("Other portrait view", "The face stays upright again.", "Lay it flat to hold this view.", 0, 1, 0, 0, 0, 0),
    ("Tickle pads 0–3", "The left eye winks.", "The mouth pulls into a crooked laugh.", 0, 0, -1, 0, 1, 0),
    ("Tickle pads 4–7", "Both eyes scrunch.", "The mouth opens wide.", 0, 0, -1, 0, 32, 0),
    ("Tickle pads 8–11", "The right eye winks.", "The laugh pulls the other way.", 0, 0, -1, 0, 1024, 0),
    ("Give it a quick shake", "Surprise turns into a dizzy wobble.", "The rear orange light gets brighter.", 0, 0, -1, 260, 0, 0),
    ("Tickle + shake", "Hold a pad while shaking.", "Orange Face giggles and glows brighter.", 0, 0, -1, 260, 1025, 0),
    ("D1 · deeper orange", "Each tap shifts one shade redder.", "The TFT and glow change together.", 0, 0, -1, 0, 0, 2),
    ("D2 · lighter orange", "Each tap shifts one shade more golden.", "There are nine steps to try.", 0, 0, -1, 0, 0, 4),
    ("D0 · original orange", "Tap BOOT while the face is running.", "It returns to shade 5/9.", 0, 0, -1, 0, 0, 1),
]

HARNESS = r"""
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include "OrangeShade.h"
#include "Orientation.h"
#include "Reactions.h"
#include "src/opentoy/DoodlePixels.h"
using std::max;
static Reactions reactions;
static Orientation orientation;
static OrangeShade shade;
static Mood mood;
template <typename T> T constrain(T v, T lo, T hi) { return std::max(lo, std::min(v, hi)); }
struct Inputs { bool motionOk = true; MotionSample motion{}; } inputs;
struct Renderer {
  DoodleAnimator animator;
  unsigned light = 0;
  bool emit = false;
  void colorize(const OrangeShade &, uint8_t level, uint32_t) { light = level; }
  void draw(const FaceState &pose, uint32_t now, uint8_t rotation, int x, int y) {
    animator.tick(pose, now, true);
    if (!emit) return;
    uint8_t pixels[1024];
    drawDoodle(animator.current, pose.expression, pixels,
               animator.wiggle(0, now), animator.wiggle(1, now), animator.wiggle(2, now));
    printf("%u,%d,%d,%u,%u,%u\n", unsigned(rotation), x, y,
           unsigned(shade.step()), light, unsigned(mood));
    fwrite(pixels, 1, sizeof pixels, stdout);
  }
  void drawMotion(const MotionSample *, uint32_t) {}
} face;
__ANIMATE__
struct Scene { float x, y, z, spin; uint16_t pads; uint8_t button; };
int main() {
  const Scene scenes[] = {__SCENES__};
  uint32_t now = 3200;
  for (unsigned scene = 0; scene < sizeof scenes / sizeof *scenes; ++scene) {
    const auto &s = scenes[scene];
    const bool turnBack = scene == 4;
    for (uint32_t t = 0; t < __LENGTH__; t += 20, now += 20) {
      inputs.motion = {s.x, s.y, s.z, 0, 0, 0, 0};
      if (turnBack && t < 320) inputs.motion = {1, 0, 0, 0, 0, 0, 0};
      const bool shake = s.spin && t >= 340 && t < 560;
      if (shake) {
        inputs.motion.z = -1.55f;
        inputs.motion.spin = s.spin;
        inputs.motion.gx = s.spin;
      }
      const auto &m = inputs.motion;
      orientation.sample(now, m.x, m.y, m.z, m.spin);
      const uint16_t pads = t >= 280 && t < 660 ? s.pads : 0;
      reactions.sample(now, pads, &m);
      const bool push = (t >= 300 && t < 420) ||
                        (s.button != 1 && t >= 640 && t < 760) ||
                        (s.button == 4 && t >= 980 && t < 1100);
      shade.sample(now, push ? s.button : 0);
      face.emit = t % __STEP__ == 0;
      mood = reactions.mood(now);
      if (t % 40 == 0) animate(mood, now);
    }
  }
}
"""


def run_renderer():
    app = (FIRMWARE / "OrangeFace.cpp").read_text()
    animate = app[app.index("static void animate("):app.index("void setup()")]
    rows = ",".join("{" + ",".join(map(str, scene[3:])) + "}" for scene in SCENES)
    source = (HARNESS.replace("__ANIMATE__", animate).replace("__SCENES__", rows)
              .replace("__STEP__", str(STEP_MS)).replace("__LENGTH__", str(STEP_MS * FRAMES_PER_SCENE)))
    with tempfile.TemporaryDirectory() as folder:
        cpp = Path(folder) / "render.cpp"
        binary = cpp.with_suffix("")
        cpp.write_text(source)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-I", str(FIRMWARE),
                        str(cpp), "-o", str(binary)], check=True)
        with subprocess.Popen([str(binary)], stdout=subprocess.PIPE) as process:
            for _ in range(len(SCENES) * FRAMES_PER_SCENE):
                numbers = tuple(map(int, process.stdout.readline().split(b",")))
                pixels = process.stdout.read(1024)
                if len(numbers) != 6 or len(pixels) != 1024:
                    raise RuntimeError("Incomplete renderer frame")
                yield (*numbers, pixels)
            if process.stdout.read() or process.wait():
                raise RuntimeError("Renderer returned unexpected output")


def path_for(pixels):
    commands = []
    for y in range(64):
        row = pixels[(y // 8) * 128:(y // 8 + 1) * 128]
        mask = 1 << (y % 8)
        x = 0
        while x < 128:
            if row[x] & mask:
                left = x
                while x < 128 and row[x] & mask:
                    x += 1
                commands.append(f"M{left} {y}h{x-left}v1h{left-x}z")
            else:
                x += 1
    return "".join(commands)


def shown(start, end, count, duration):
    if start == 0:
        values, keys = "1;0;1", f"0;{end/count:.8f};1"
    elif end == count:
        values, keys = "0;1;0", f"0;{start/count:.8f};1"
    else:
        values, keys = "0;1;0;0", f"0;{start/count:.8f};{end/count:.8f};1"
    return (f'<g opacity="{int(start == 0)}"><animate attributeName="opacity" '
            f'values="{values}" keyTimes="{keys}" calcMode="discrete" '
            f'dur="{duration:.2f}s" repeatCount="indefinite"/>')


def text(value, x, y, size, fill="#382417", extra=""):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" {extra}>{html.escape(str(value))}</text>'


def screen(frame, scene_index, tick):
    scene = SCENES[scene_index]
    rotation, x, y, shade, glow, _, pixels = frame
    hue = f"#ff{84 + shade * 20:02x}00"
    portrait = rotation in (0, 2)
    width, height = (170, 302) if portrait else (302, 170)
    px, py = 296 - width / 2, 282 - height / 2
    face_width, face_height = (141, 87) if portrait else (224, 79)
    face_left = (width - face_width) / 2 + x * 1.26
    face_top = (height - 68 - face_height) / 2 + y * 1.26
    tilt_x, tilt_y, tilt_z, twist, _, button = scene[3:]
    if scene_index == 4 and tick * STEP_MS < 320:
        tilt_x, tilt_y, tilt_z = 1, 0, 0
    shaking = twist and 340 <= tick * STEP_MS < 560
    accel = (tilt_x, tilt_y, -1.55 if shaking else tilt_z)
    gyro = (twist if shaking else 0, 0, 0)
    light_width = max(5, round(glow * 1.35))
    angle = {0: -90, 1: 0, 2: 90, 3: 180}[rotation]
    parts = [
        f'<use href="#board" transform="translate(296 282) rotate({angle})"/>',
        f'<g transform="translate({px} {py})">',
        f'<rect width="{width}" height="{height}" rx="4" fill="#1b0800"/>',
        f'<path d="{path_for(pixels)}" fill="{hue}" transform="translate({face_left} {face_top}) scale({face_width/128:.5f} {face_height/64:.5f})"/>',
        f'<g transform="translate(0 {height - 68})" font-family="ui-monospace, Menlo, monospace">',
        f'<rect width="{width}" height="68" rx="3" fill="#351500"/>',
        text("6DoF", 8, 13, 10, hue),
        text("Accel g", 8, 30, 10, "#ffedd1"),
        text("Gyro °/s", 8, 48, 10, "#ffedd1"),
    ]
    end = 'text-anchor="end"'
    for edge, axis, acceleration, spin in zip((99, 131, 163) if portrait else (134, 211, 289),
                                               "XYZ", accel, gyro):
        parts.extend((text(axis, edge - 3, 13, 10, hue, end),
                      text(f"{acceleration:+.1f}", edge, 30, 10 if portrait else 17, "#ffedd1", end),
                      text(f"{spin:+.0f}", edge, 48, 10 if portrait else 17, "#ffedd1", end)))
    hint = f"Orange {shade + 1}/9" if button else "Stand up. Turn/shake."
    parts.extend((text(hint, 8, 61, 10, hue), "</g></g>",
                  f'<rect x="681" y="460" width="{light_width}" height="8" rx="4" fill="{hue}"/>',
                  f'<circle cx="660" cy="464" r="7" fill="{hue}"/>'))
    return parts


def make_svg(frames):
    duration = len(frames) * STEP_MS / 1000
    parts = [f'''<svg xmlns="http://www.w3.org/2000/svg" width="960" height="600" viewBox="0 0 960 600" role="img" aria-labelledby="title desc">
<title id="title">Orange Face behavior tour</title>
<desc id="desc">An illustrated Feather with firmware-rendered facial expressions shows gravity rotation, motion readings, three touch zones, shake reactions and three shade controls. Motion and readings are illustrative.</desc>
<defs>
  <g id="board">
    <rect x="-189" y="-109" width="378" height="225" rx="21" fill="#d6c1ae"/>
    <rect x="-189" y="-112" width="378" height="224" rx="21" fill="#302833" stroke="#54434d" stroke-width="3"/>
    <rect x="-158" y="-91" width="316" height="182" rx="8" fill="#14121a"/>
    <path d="M-131 -101H131 M-131 101H131" stroke="#a68959" stroke-width="6" stroke-linecap="round" stroke-dasharray="0.1 16"/>
    <path d="M-175 -92h.1m350 0h.1m0 184h.1m-350 0h.1" stroke="#d9b678" stroke-width="11" stroke-linecap="round"/>
    <rect x="-194" y="-24" width="18" height="48" rx="5" fill="#c6b9b0"/>
    <path d="M-188 -13v26" stroke="#5d5456" stroke-width="3" stroke-linecap="round"/>
    <path d="M173 -39h.1m0 39h.1m0 39h.1" stroke="#c7b3b9" stroke-width="12" stroke-linecap="round"/>
  </g>
</defs>
<rect width="960" height="600" rx="26" fill="#fff0df"/>
<rect x="32" y="85" width="528" height="394" rx="22" fill="#f7ddc5"/>
<rect x="585" y="85" width="343" height="394" rx="22" fill="#fff8ed"/>
<g font-family="Arial, Helvetica, sans-serif">
{ text("Orange Face", 34, 55, 38, extra='font-weight="700"') }
{ text("OpenToy  /  v0.4", 925, 53, 16, "#754b34", 'text-anchor="end"') }
{ text("MPR121 TOUCH ZONES", 617, 248, 12, "#754b34", 'font-weight="700" letter-spacing="2"') }
{ text("FEATHER BUTTONS", 617, 344, 12, "#754b34", 'font-weight="700" letter-spacing="2"') }
{ text("REAR NEOPIXEL", 617, 443, 12, "#754b34", 'font-weight="700" letter-spacing="2"') }
<rect x="681" y="460" width="120" height="8" rx="4" fill="#f1ddcb"/>
<path d="M35 511H925" stroke="#d8b89e"/>
{ text("Turn upright   ·   Tap a touch pad   ·   Shake   ·   Adjust the orange", 35, 546, 20, extra='font-weight="700"') }
{ text("Illustrated demonstration  /  Firmware-rendered faces  /  Example IMU readings", 35, 574, 15, "#754b34") }''']
    count = len(frames)
    for idx, frame in enumerate(frames):
        parts.append(shown(idx, idx + 1, count, duration))
        parts.extend(screen(frame, idx // FRAMES_PER_SCENE, idx % FRAMES_PER_SCENE))
        parts.append("</g>")
    for index, scene in enumerate(SCENES):
        title, line1, line2, *_, pads, button = scene
        parts.extend((shown(index * FRAMES_PER_SCENE, (index + 1) * FRAMES_PER_SCENE, count, duration),
                      text(title, 617, 135, 26, extra='font-weight="700"'),
                      text(line1, 617, 171, 15), text(line2, 617, 193, 15)))
        for zone, label in enumerate(("0–3", "4–7", "8–11")):
            active = bool(pads & (0xf << (zone * 4)))
            left = 617 + zone * 93
            parts.extend((f'<rect x="{left}" y="262" width="83" height="36" rx="8" fill="{"#ffbe7c" if active else "#f4e3d3"}"/>',
                          text(label, left + 41, 285, 15, extra='text-anchor="middle" font-weight="700"')))
        for bit, label in enumerate(("D0 Reset", "D1 Deeper", "D2 Lighter")):
            left = 617 + bit * 93
            parts.extend((f'<rect x="{left}" y="358" width="83" height="36" rx="8" fill="{"#ffbe7c" if button & (1 << bit) else "#f4e3d3"}"/>',
                          text(label, left + 41, 381, 12, extra='text-anchor="middle" font-weight="700"')))
        parts.append("</g>")
    return "\n".join([*parts, "</g></svg>\n"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    frames = list(run_renderer())
    rotations = {frame[0] for frame in frames}
    shades = {frame[3] for frame in frames}
    moods = {frame[5] for frame in frames}
    if rotations != {0, 1, 2, 3} or not {2, 4, 5} <= shades or moods != {0, 1, 2, 3, 4}:
        raise RuntimeError("Tour is missing a rotation, shade or expression")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(make_svg(frames), encoding="utf-8")
    print(f"Wrote {len(frames)} frames to {args.output}")


if __name__ == "__main__":
    main()
