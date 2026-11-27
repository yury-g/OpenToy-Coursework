#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Render firmware-generated 240x280 Water full-screen PNG and GIF previews."""
import argparse
import os
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont


def load_font(size: int):
  for path in [
      os.environ.get('FONT_REGULAR', ''),
      '/System/Library/Fonts/Supplemental/Arial.ttf',
      '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
  ]:
    if path and Path(path).exists():
      return ImageFont.truetype(path, size)
  return ImageFont.load_default()


def main():
  p = argparse.ArgumentParser()
  p.add_argument('raw_frames', type=Path)
  a = p.parse_args()

  out_dir = Path(__file__).resolve().parents[1] / 'previews'
  raw = np.fromfile(a.raw_frames, dtype='<u2').reshape(-1, 280, 240)
  frames = []
  for f in raw:
    rgb = np.stack(
        [((f >> 11) & 31) * 255 // 31, ((f >> 5) & 63) * 255 // 63, (f & 31) * 255 // 31],
        axis=-1,
    ).astype('uint8')
    frames.append(Image.fromarray(rgb))

  frames[0].save(
      out_dir / 'water-fullscreen.gif',
      save_all=True,
      append_images=frames[1:],
      duration=66,
      loop=0,
      optimize=True,
  )

  canvas = Image.new('RGB', (1120, 440), '#10191e')
  d = ImageDraw.Draw(canvas)
  font = load_font(18)
  small = load_font(15)
  picks = [
      (12, 'Water + overlay (BAT --)'),
      (36, 'Tilted under overlay'),
      (70, 'Performance (overlays off)'),
      (84, 'Face full screen + overlay'),
  ]
  for i, (idx, label) in enumerate(picks):
    x = 20 + i * 280
    d.rounded_rectangle((x, 57, x + 256, 353), 18, fill='#253b46')
    canvas.paste(frames[min(idx, len(frames) - 1)], (x + 8, 65))
    d.text((x, 22), label, font=font, fill='#ecf5f8')
  d.text(
      (20, 386),
      'OpenToy / Waveshare 1.69 (0.2.2-dev)   |   Firmware simulation — not a device recording',
      font=small,
      fill='#b0c1c9',
  )
  canvas.save(out_dir / 'water-fullscreen.png')


if __name__ == '__main__':
  main()
