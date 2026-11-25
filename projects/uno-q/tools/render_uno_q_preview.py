#!/usr/bin/env python3
"""Render labelled synthetic UNO Q 2.8-inch TFT source preview without hardware."""
import os
import shlex
import subprocess
import tempfile
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
WORKTREE = ROOT.parents[1]


def main() -> int:
    out_png = ROOT / 'docs/images/uno-q-tft-preview.png'
    out_png.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='uno-q-preview-') as tmp:
        exe = Path(tmp) / 'uno_q_eyes_test'
        ppm = Path(tmp) / 'frame.ppm'
        includes = [
            str(WORKTREE / 'projects/animatronic-eyes/firmware/AnimatronicEyes'),
            str(WORKTREE / 'projects/animatronic-eyes/firmware/NessoEyes'),
        ]
        cxx = shlex.split(os.environ.get('CXX', 'c++'))
        subprocess.run(
            cxx
            + ['-std=c++14', '-Wall', '-Wextra', '-Werror']
            + ['-I' + inc for inc in includes]
            + [str(ROOT / 'tests/uno_q_eyes_test.cpp'), '-o', str(exe)],
            check=True,
        )
        subprocess.run([str(exe), '--dump-ppm', str(ppm)], check=True)
        panel = Image.open(ppm).convert('RGB')
        canvas = Image.new('RGB', (360, 300), '#10161d')
        draw = ImageDraw.Draw(canvas)
        draw.text((20, 8), 'UNO Q 2.8" TFT Source Preview (EYES1 v0.1.0-src)', fill='#80e5ff')
        canvas.paste(panel, (20, 30))
        draw.text((20, 278), 'SOURCE SIMULATION - SHIELD HARDWARE UNQUALIFIED', fill='#ffd577')
        canvas.save(out_png)
    print(f'Rendered {out_png}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
