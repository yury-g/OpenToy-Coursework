#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Host parser/uploader/mapping tests. Does not connect to or flash hardware."""
import os,shlex,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def run(argv):subprocess.run(argv,check=True,cwd=ROOT)
with tempfile.TemporaryDirectory(prefix='opentoy-tests-') as d:
 for project in ['xiao-toy','xiao-class','nesso-n1','gravity-water','face-feather']:
  tests=ROOT/'projects'/project/'tests'
  if any(tests.glob('*test.py')): run([sys.executable,'-O','-m','unittest','discover','-s',str(tests),'-p','*test.py'])
  for source in tests.glob('*.cpp'):
   out=Path(d)/(project+'-'+source.stem);run(shlex.split(os.environ.get('CXX','c++'))+[str(source),'-o',str(out)]);run([str(out)])
 run([sys.executable,'-O','-m','unittest','discover','-s',str(ROOT/'tests'),'-p','*test.py'])
print('OpenToy host checks passed. Physical controller and radio tests are separate.')
