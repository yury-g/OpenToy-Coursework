#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Host-only CrowPanel and reused Eyes behavior checks. No serial access."""
import os,shlex,subprocess,sys,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,'-m','unittest','discover','-s',str(root/'tests'),'-p','*_test.py'],check=True)
with tempfile.TemporaryDirectory(prefix='crowpanel-check-') as tmp:
 for source in sorted((root/'tests').glob('*.cpp')):
  exe=Path(tmp)/source.stem
  subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14',str(source),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
subprocess.run([sys.executable,str(root.parent/'animatronic-eyes/tools/check.py')],check=True)
