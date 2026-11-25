#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Synthetic controller inputs -> real BLE -> independent USB/OLED bus evidence."""
import asyncio,threading,time,re,json,sys
from pathlib import Path
import serial
from bleak import BleakClient
from ble_client import Toy,find,require
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'xiao-toy/scripts'))
from controller_input import GamepadMapper
async def main():
 profile=json.loads((ROOT/'device.json').read_text());lines=[];stop=threading.Event()
 def reader():
  with serial.Serial(profile['port'],115200,timeout=.1) as s:
   s.rts=False;s.dtr=True
   while not stop.is_set():
    b=s.readline()
    if b:lines.append(b.decode(errors='replace').strip())
 thread=threading.Thread(target=reader,daemon=True);thread.start();await asyncio.sleep(.5)
 try:
  async with BleakClient(await find(profile)) as c:
   toy=Toy(c,profile);await toy.start();await toy.command(b'touch off;');mapper=GamepadMapper();hashes={};latencies=[]
   async def check(pose):
    begin=len(lines);start=time.monotonic();r=await toy.command((pose.command()+';').encode());latencies.append((time.monotonic()-start)*1000);seq=re.search(r'seq=(\d+)',r).group(1);end=time.monotonic()+3
    fields=[f'x={pose.x} ',f'y={pose.y} ',f'left={pose.left} ',f'right={pose.right} ',f'expression={pose.expression}']
    while time.monotonic()<end:
     face=next((x for x in lines[begin:] if x.startswith('FACE seq='+seq+' ') and all(f in x for f in fields)),None)
     periph=next((x for x in lines[begin:] if x.startswith('PERIPH seq='+seq+' ')),None)
     if face and periph:
      require('i2c_ok=1' in periph and 'errors=0' in periph,periph);require('txerr=0' in r,r)
      digest=re.search(r'frame=([0-9a-f]+)',periph).group(1);print('PASS synthetic:',pose.command(),'BLE:',r,'USB:',face,'frame='+digest,flush=True);return digest
     await asyncio.sleep(.01)
    raise RuntimeError('USB did not corroborate face command '+pose.command())
   neutral=await check(mapper.sample(0,0,0,0,{}));require(neutral=='a1bf2f0d','Default eye buffer changed unexpectedly')
   for x,y,left,right in [(-1,0,0,0),(1,0,0,0),(0,1,0,0),(0,-1,0,0),(0,0,.5,0),(0,0,0,.5),(0,0,1,1)]:
    digest=await check(mapper.sample(x,y,left,right,{}));require(digest!=neutral,'Direction or lid did not change eye buffer')
   for button,expr in [('A','happy'),('B','angry'),('X','surprised'),('Y','neutral')]:
    hashes[expr]=await check(mapper.sample(0,0,0,0,{button:True}));mapper.sample(0,0,0,0,{})
   require(len(set(hashes.values()))==4,'Expressions do not have distinct buffers')
   for i in range(20):
    start=time.monotonic();await check(mapper.sample((i%5-2)/2,0,(i%4)/3,0,{}));await asyncio.sleep(max(0,.1-(time.monotonic()-start)))
   for cmd in ['face 101 0 0 0 neutral','face 0 0 0 101 neutral','face 0 0 0 0 bogus','face 999999999999999999 0 0 0 neutral']:
    print('PASS rejection',cmd,await toy.command((cmd+';').encode(),'ERR command'),flush=True)
   await check(mapper.disconnected());await toy.command(b'touch on;')
   await check(mapper.sample(1,1,.5,.5,{'B':True}));begin=len(lines);await c.disconnect();end=time.monotonic()+5
   neutral=None
   while time.monotonic()<end:
    neutral=next((x for x in lines[begin:] if x.startswith('FACE ') and all(v in x for v in ['x=0 ','y=0 ','left=0 ','right=0 ','expression=neutral'])),None)
    if neutral:break
    await asyncio.sleep(.05)
   require(neutral is not None,'USB did not confirm face neutral after BLE disconnect')
   print('PASS physical BLE disconnect restores neutral face; USB:',neutral,flush=True)
   print('FACE BLE + USB TESTS PASS; physical controller NOT tested; ACK ms min/max=',round(min(latencies)),round(max(latencies)),flush=True)
 finally:
  stop.set();thread.join(2);(ROOT/'evidence/face-usb-stream.log').write_text('\n'.join(lines)+'\n')
asyncio.run(main())
