#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse,time,json
from pathlib import Path
import serial
p=argparse.ArgumentParser();p.add_argument('--port',default=json.loads((Path(__file__).resolve().parents[1]/'device.json').read_text())['port']);p.add_argument('--seconds',type=float,default=8);p.add_argument('--out');p.add_argument('--send');a=p.parse_args()
lines=[]
with serial.Serial(a.port,115200,timeout=.2) as s:
 s.rts=False;s.dtr=True
 if a.send:s.write((a.send+';').encode())
 end=time.monotonic()+a.seconds
 while time.monotonic()<end:
  b=s.readline()
  if b:lines.append(b.decode(errors='replace').rstrip())
text='\n'.join(lines)+'\n'
if a.out:Path(a.out).write_text(text)
print(text)
if not any('HEARTBEAT' in x or 'LED t=' in x for x in lines):raise SystemExit('FAIL: no runtime heartbeat')
if any('panic' in x or 'Guru Meditation' in x for x in lines):raise SystemExit('FAIL: firmware crash')
