#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Bluetooth commands with independent USB serial evidence on the same chip."""
import asyncio,threading,time,re,json
from pathlib import Path
import serial
from bleak import BleakClient
from ble_client import Toy,find,require,RX
ROOT=Path(__file__).resolve().parents[1]
async def main():
 profile=json.loads((ROOT/'device.json').read_text());lines=[];stop=threading.Event()
 def reader():
  with serial.Serial(profile['port'],115200,timeout=.2) as s:
   s.rts=False;s.dtr=True
   while not stop.is_set():
    b=s.readline()
    if b:lines.append(b.decode(errors='replace').strip())
 thread=threading.Thread(target=reader,daemon=True);thread.start();await asyncio.sleep(.5)
 try:
  async with BleakClient(await find()) as c:
   t=Toy(c);await t.start();hashes={}
   for cmd,fields in [('screen eyes',['screen=eyes']),('screen closed',['screen=closed']),('screen test',['screen=test']),('contrast 45',['contrast=45']),('touch threshold 1000',['threshold=1000','mask=15']),('touch threshold 2000000',['threshold=2000000','mask=0']),('touch off',['touch=0','mask=0']),('touch on',['touch=1']),('touch threshold 80000',['threshold=80000']),('screen off',['screen=off']),('screen eyes',['screen=eyes']),('contrast 127',['contrast=127'])]:
    beginning=len(lines);r=await t.command((cmd+';').encode());seq=re.search(r'seq=(\d+)',r).group(1)
    deadline=time.monotonic()+5;usb=None
    while time.monotonic()<deadline:
     usb=next((x for x in lines[beginning:] if x.startswith('PERIPH ') and 'seq='+seq+' ' in x and all(f in x for f in fields)),None)
     if usb:break
     await asyncio.sleep(.05)
    require(usb is not None,'USB did not confirm '+cmd);require('i2c_ok=1' in usb and 'errors=0' in usb,usb)
    raw=list(map(int,re.search(r'raw=([\d,]+)',usb).group(1).split(',')));require(len(raw)==4 and min(raw)>0,usb)
    if cmd.startswith('screen '):hashes[cmd]=re.search(r'frame=([0-9a-f]+)',usb).group(1)
    print('PASS BLE:',r,'USB:',usb,flush=True)
   require(len(set(hashes.values()))==4,'Distinct screen buffers not observed')
   # Use a threshold between the two largest real readings to activate one
   # input without pretending that a person physically touched it.
   ordered=sorted(raw);threshold=(ordered[-1]+ordered[-2])//2
   require(ordered[-1]-ordered[-2]>200,'Readings too close for remote threshold test')
   expected_mask=sum(1<<i for i,v in enumerate(raw) if v>threshold)
   async def match_usb(cmd,fields):
    begin=len(lines);reply=await t.command((cmd+';').encode());sequence=re.search(r'seq=(\d+)',reply).group(1)
    end=time.monotonic()+5
    while time.monotonic()<end:
     match=next((x for x in lines[begin:] if x.startswith('PERIPH seq='+sequence+' ') and all(v in x for v in fields)),None)
     if match:
      require('i2c_ok=1' in match and 'errors=0' in match,match);print('PASS BLE:',reply,'USB:',match,flush=True);return match
     await asyncio.sleep(.05)
    raise RuntimeError('No USB confirmation for '+cmd)
   moved=await match_usb(f'touch threshold {threshold}',[f'mask={expected_mask} '])
   require(re.search(r'frame=([0-9a-f]+)',moved).group(1)!=hashes['screen eyes'],'Enabled touch measurement did not move eye buffer')
   disabled=await match_usb('touch off',['touch=0','mask=0 '])
   require('frame='+hashes['screen eyes'] in disabled,'Disabling touch did not restore neutral eyes')
   await match_usb('touch threshold 80000',['threshold=80000','mask=0 ']);await match_usb('touch on',['touch=1','mask=0 '])
   print('PASS real measurements drive eye buffer; Bluetooth gating restored neutral frame',flush=True)
   for cmd in ['contrast 256','touch threshold 999','touch threshold 2000001','screen bogus']:
    r=await t.command((cmd+';').encode(),'ERR command');print('PASS rejected',cmd,r,flush=True)
   # A packed-command burst must not stall heartbeat, permanently jam BLE, or alter display config.
   try:await c.write_gatt_char(RX,b'reset;'*40,response=True)
   except Exception as e:print('Burst write transport result',type(e).__name__)
   await asyncio.sleep(1);r=await t.command(b'status;','STATUS');print('PASS recovered after packed commands',r,flush=True)
   print('PERIPHERAL BLUETOOTH + USB TESTS PASS',flush=True)
 finally:
  stop.set();thread.join(2);(ROOT/'evidence/peripheral-usb-stream.log').write_text('\n'.join(lines)+'\n')
asyncio.run(main())
