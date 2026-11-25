#!/usr/bin/env python3
"""Host-only checks; never accesses serial or a camera."""
import os, shlex, subprocess, tempfile
from pathlib import Path
root = Path(__file__).resolve().parents[1]
env = dict(os.environ)
eyes = root.parent/'animatronic-eyes'
includes = [eyes/'firmware/AnimatronicEyes', eyes/'firmware/NessoEyes',
            eyes/'shared/opentoy', root/'firmware/reference']
if Path('/Library/Developer/CommandLineTools').exists():
    env.setdefault('DEVELOPER_DIR', '/Library/Developer/CommandLineTools')
with tempfile.TemporaryDirectory(prefix='copy-face-check-') as tmp:
    for source in sorted((root/'tests').glob('*.cpp')):
        exe = Path(tmp)/source.stem
        subprocess.run(shlex.split(env.get('CXX', 'c++')) +
                       ['-std=c++14', '-Wall', '-Wextra'] +
                       ['-I'+str(p) for p in includes] + [str(source), '-o', str(exe)],
                       env=env, check=True)
        subprocess.run([str(exe)], check=True)
