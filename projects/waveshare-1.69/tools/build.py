#!/usr/bin/env python3
"""Stage shared OpenToy behavior and compile this target; never upload/flash."""
import argparse, shutil, subprocess, tempfile
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-path', type=Path, required=True)
p.add_argument('--config-file', type=Path)
p.add_argument('--cli', default='arduino-cli')
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
repo = root.parents[1]
build = a.build_path.resolve()
if build.is_relative_to(repo):
  p.error('Build output must be outside the repository')
build.parent.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='touch169-', dir=build.parent) as temp:
  sketch = Path(temp) / 'Curious169'
  shutil.copytree(root / 'firmware/Curious169', sketch)
  eyes = repo / 'projects/animatronic-eyes'
  for group, names in [
      ('firmware/NessoEyes', ['TouchReaction.h', 'Idle.h', 'Presentation.h']),
      ('firmware/AnimatronicEyes', ['Companion.h', 'Controls.h', 'Personality.h']),
      ('shared/opentoy', ['Protocol.h', 'FaceProtocol.h', 'EyePixels.h']),
      ('shared', ['TinyFont3x5.h']),
  ]:
    for name in names:
      shutil.copy2(eyes / group / name, sketch / name)
  water = repo / 'projects/gravity-water/firmware/GravityWaterNesso'
  shutil.copy2(water / 'NessoPixels.h', sketch / 'NessoPixels.h')
  (sketch / 'src/fluid').mkdir(parents=True, exist_ok=True)
  for name in ['Particles.h', 'Gravity.h', 'Motion.h']:
    shutil.copy2(water / 'src/fluid' / name, sketch / 'src/fluid' / name)
  cmd = [a.cli]
  if a.config_file:
    cmd += ['--config-file', str(a.config_file.resolve())]
  subprocess.run(
      cmd + [
          'compile',
          '--fqbn',
          'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB',
          '--build-property',
          'compiler.cpp.extra_flags=-std=gnu++17',
          '--build-path',
          str(build),
          str(sketch),
      ],
      check=True,
  )
