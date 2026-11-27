#!/usr/bin/env python3
"""Host tests, including undefined behavior and buffer bounds; no USB access."""
from pathlib import Path
import subprocess, tempfile

root = Path(__file__).resolve().parents[1]
eyes = root.parent / 'animatronic-eyes'
water = root.parent / 'gravity-water/firmware/GravityWaterNesso'
includes = [
    root / 'firmware/Curious169',
    eyes / 'firmware/NessoEyes',
    eyes / 'firmware/AnimatronicEyes',
    eyes / 'shared/opentoy',
    eyes / 'shared',
    water,
]
with tempfile.TemporaryDirectory() as tmp:
  for test_name in ['interaction', 'water_imu']:
    exe = Path(tmp) / test_name
    subprocess.run(
        [
            'g++',
            '-std=c++17',
            '-Wall',
            '-Wextra',
            '-Werror',
            '-Wno-misleading-indentation',
            '-fsanitize=address,undefined',
            '-g',
            *[f'-I{x}' for x in includes],
            str(root / f'tests/{test_name}.cpp'),
            '-o',
            str(exe),
        ],
        check=True,
    )
    subprocess.run([str(exe)], check=True)
