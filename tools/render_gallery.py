#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Pixel-accurate OLED simulations. Uses the firmware's pure C++ renderer; no BLE."""
import argparse,html,json,os,shlex,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--fixtures',type=Path,default=ROOT/'projects/xiao-class/tests/eye_frames.json');p.add_argument('--output',type=Path,default=ROOT/'docs/images/v1-eyes.svg');p.add_argument('--title',default='OpenToy v1 · eyes');a=p.parse_args()
fixtures=json.loads(a.fixtures.read_text())
if a.fixtures.name=='eye_frames.json':
 indices=[0,1,4,7,10,13]
 labels=['Neutral · centered pupils','Left stick · whites up','D-pad · pupils right','Right stick · left wink','Right stick down · top lids','B · angry preset']
 fixtures=[dict(fixtures[i],label=label) for i,label in zip(indices,labels)]
with tempfile.TemporaryDirectory(prefix='opentoy-gallery-') as d:
 exe=Path(d)/'frames'
 subprocess.run(shlex.split(os.environ.get('CXX','c++'))+[str(ROOT/'tools/frame_dump.cpp'),'-o',str(exe)],check=True)
 cards=[]
 for i,f in enumerate(fixtures):
  result=subprocess.check_output([str(exe),f['command']],text=True).split()
  assert result[:3]==['P1','128','64']
  pixels=list(map(int,result[3:]));assert len(pixels)==8192
  buffer=bytearray(1024)
  for y in range(64):
   for x in range(128):
    if pixels[y*128+x]:buffer[(y//8)*128+x]|=1<<(y%8)
  digest=2166136261
  for b in buffer:digest=((digest^b)*16777619)&0xffffffff
  assert f'{digest:08x}'==f['hash'],f['command']
  x0=32+(i%2)*568;y0=110+(i//2)*365
  path=''.join(f'M{x} {y}h1v1h-1z' for y in range(64) for x in range(128) if pixels[y*128+x])
  cards.append(f'<g transform="translate({x0} {y0})"><text y="0" class="label">{html.escape(f.get("label",f["command"]))}</text><rect y="20" width="512" height="256" rx="12" fill="#080d14"/><path transform="translate(0 20) scale(4)" d="{path}" fill="#e8f7fa"/><text y="301" class="command">{html.escape(f["command"])}</text><text y="324" class="meta">Frame {f["hash"]} · 128 × 64 · enlarged 4×</text></g>')
 height=135+((len(fixtures)+1)//2)*365
 svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="1160" height="{height}" viewBox="0 0 1160 {height}"><style>text{{font-family:system-ui,sans-serif;fill:#16243a}}.label{{font-size:21px;font-weight:600}}.command{{font-family:monospace;font-size:13px}}.meta{{font-size:14px;fill:#546779}}</style><rect width="1160" height="{height}" fill="#f2f6fa"/><text x="32" y="42" font-size="29" font-weight="700">{html.escape(a.title)}</text><text x="32" y="73" font-size="16">Simulated OLED screenshots from the firmware renderer; not photos of the physical display.</text>{"".join(cards)}</svg>'
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(svg)
 print(f'Saved {len(fixtures)} pixel/hash-checked simulations to {a.output}')
