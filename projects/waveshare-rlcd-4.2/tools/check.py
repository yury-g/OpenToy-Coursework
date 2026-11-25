#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""RLCD adapter checks and existing Eyes regression tests; no USB access."""
import os, shlex, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rlcd-host-') as tmp:
 for source in sorted((root/'tests').glob('*.cpp')):
  out=Path(tmp)/source.stem
  subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14','-Wall','-Wextra',str(source),'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True)
subprocess.run(['python3',str(root.parent/'animatronic-eyes/tools/check.py')],check=True)
