#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""New eye layers over real BLE, with independent USB and pixel reference hashes."""
import asyncio,json,re,threading,time,sys
from pathlib import Path
import serial
from bleak import BleakClient
from ble_client import Toy,find,require
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'xiao-toy/scripts'))
from controller_input import LayeredGamepadMapper

async def main():
 profile=json.loads((ROOT/'device.json').read_text());lines=[];stop=threading.Event();latencies=[]
 def reader():
  with serial.Serial(profile['port'],115200,timeout=.1) as s:
   s.rts=False;s.dtr=True
   while not stop.is_set():
    raw=s.readline()
    if raw:lines.append(raw.decode(errors='replace').strip())
 thread=threading.Thread(target=reader,daemon=True);thread.start();await asyncio.sleep(.4)
 try:
  async with BleakClient(await find(profile)) as c:
   toy=Toy(c,profile);await toy.start();r=await toy.command(b'status;','STATUS');require('eyes=1' in r,'Eye-layer firmware capability missing')
   await toy.command(b'touch off;')
   async def check(command,expected_hash=None,expected='OK'):
    beginning=len(lines);start=time.monotonic();r=await toy.command((command+';').encode(),expected);latencies.append((time.monotonic()-start)*1000);seq=re.search(r'seq=(\d+)',r).group(1);deadline=time.monotonic()+4
    face=periph=None
    while time.monotonic()<deadline:
     face=next((s for s in lines[beginning:] if s.startswith('FACE seq='+seq+' ')),None)
     periph=next((s for s in lines[beginning:] if s.startswith('PERIPH seq='+seq+' ')),None)
     if face and periph:break
     await asyncio.sleep(.01)
    require(face and periph,'USB did not corroborate '+command)
    require('txerr=0' in r and 'i2c_ok=1' in periph and 'errors=0' in periph,r+' '+periph)
    if expected=='OK':
     _,x,y,px,py,left,right,edge,expression=command.split()
     values=[f'x={x} ',f'y={y} ',f'px={px} ',f'py={py} ',f'left={left} ',f'right={right} ',f'edge={edge}',f'expression={expression} ', 'pupils=1 ']
     require(all(v in face for v in values),face)
    digest=re.search(r'frame=([0-9a-f]+)',periph).group(1)
    if expected_hash:require(digest==expected_hash,f'Pixel reference mismatch: {digest} != {expected_hash} for {command}')
    print('PASS BLE:',command,r,'USB:',face,'frame='+digest,flush=True)
    return face,digest
   fixtures=json.loads((ROOT/'tests/eye_frames.json').read_text())
   for f in fixtures:await check(f['command'],f['hash'])
   mapper=LayeredGamepadMapper()
   for i in range(20):
    begin=time.monotonic();pose=mapper.sample((i%5-2)/2,(i%3-1),(i%5-2)/2,{},dpad=(i%3-1,(i+1)%3-1));await check(pose.command());await asyncio.sleep(max(0,.1-(time.monotonic()-begin)))
   face,digest=await check('eyes 0 -50 100 100 100 50 bottom happy')
   fields=re.sub(r'^FACE seq=\d+ ','',face)
   for bad in ['eyes 0 0 101 0 0 0 bottom neutral','eyes 0 0 0 -101 0 0 top neutral','eyes 0 0 0 0 101 0 bottom neutral','eyes 0 0 0 0 0 0 sideways neutral','eyes 0 0 0 0 0 0 top bogus']:
    after,_=await check(bad,digest,'ERR command');require(re.sub(r'^FACE seq=\d+ ','',after)==fields,'Invalid command changed eye state')
   await check('eyes 0 0 0 0 0 0 bottom neutral',fixtures[0]['hash'])
   await toy.command(b'touch on;')
   await check('eyes 0 -50 100 100 100 50 bottom happy');beginning=len(lines);await c.disconnect();deadline=time.monotonic()+4
   while time.monotonic()<deadline:
    found=next((s for s in lines[beginning:] if s.startswith('FACE ') and all(v in s for v in ['x=0 ','y=0 ','left=0 ','right=0 ','expression=neutral ','px=0 ','py=0 ','pupils=1 ','edge=bottom'])),None)
    if found:break
    await asyncio.sleep(.05)
   require(found,'USB did not confirm neutral pupil state after disconnect')
   print('PASS physical BLE disconnect neutralizes new eye layers; USB:',found,flush=True)
   print(f'EYE LAYERS BLE + USB PASS: {len(fixtures)} exact pixel references, 20 streamed poses, invalid-input atomicity; ACK {min(latencies):.0f}..{max(latencies):.0f}ms. Controller inputs here are synthetic.',flush=True)
 finally:
  stop.set();thread.join(2);(ROOT/'evidence/eye-layers-usb-stream.log').write_text('\n'.join(lines)+'\n')

if __name__=='__main__':asyncio.run(main())
