#!/usr/bin/env python3
"""Portable controller, original renderer, and USB queue checks. No hardware access."""
import os,shlex,subprocess,tempfile,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
for item in json.loads((root/'docs/upstream-files.json').read_text()):
 assert hashlib.sha256((root/item['file']).read_bytes()).hexdigest()==item['sha256'],item['file']
with tempfile.TemporaryDirectory(prefix='animatronic-tests-') as tmp:
 for source in sorted((root/'tests').glob('*.cpp')):
  out=Path(tmp)/source.stem
  subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14',str(source),'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True)
print('PASS all Animatronic Eyes host checks and unchanged upstream renderer/telemetry digests')
