#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Real Mac CoreBluetooth client for live control and authenticated BLE OTA."""
import argparse,asyncio,hashlib,hmac,struct,time,json
from pathlib import Path
from bleak import BleakClient,BleakScanner
SERIAL='00002a25-0000-1000-8000-00805f9b34fb'
NUS='6e400001-b5a3-f393-e0a9-e50e24dcca9e'
RX='6e400002-b5a3-f393-e0a9-e50e24dcca9e'
TX='6e400003-b5a3-f393-e0a9-e50e24dcca9e'
OP='8e7c0002-54ad-4da3-97f4-754ac67c0a00'
OS='8e7c0003-54ad-4da3-97f4-754ac67c0a00'
def require(condition,message='Protocol validation failed'):
 if not condition:raise RuntimeError(message)

def load_image(path,profile,allow_test=False):
 path=Path(path);data=path.read_bytes();m=json.loads((path.parent/'manifest.json').read_text())
 require(m.get('device_mac')==profile['base_mac'],'Firmware belongs to a different toy')
 require(m.get('target')=='esp32:esp32:XIAO_ESP32S3','Wrong firmware target')
 require(bool(m.get('peripherals'))==bool(profile.get('peripherals')),'Wrong peripheral configuration')
 require(m.get('bytes')==len(data) and m.get('sha256')==hashlib.sha256(data).hexdigest(),'Firmware manifest hash/size mismatch')
 require(allow_test or not m.get('rollback_test'),'Recovery test image requires explicit expected fallback build')
 return data,m

async def find(profile=None):
 import json
 profile=profile or json.loads((Path(__file__).resolve().parents[1]/'device.json').read_text())
 def match(d,a):
  correct_name=a.local_name==profile['name']
  legacy=profile.get('mac_corebluetooth_id')==d.address and a.local_name==profile.get('legacy_name')
  return (correct_name or legacy) and NUS in a.service_uuids
 d=await BleakScanner.find_device_by_filter(match,timeout=12)
 if not d:raise RuntimeError('Expected XIAO Toy not advertising')
 return d
class Toy:
 def __init__(self,c,profile=None):self.profile=profile;self.c=c;self.q=asyncio.Queue();self.lines=asyncio.Queue();self.partial=b''
 async def start(self):
  await asyncio.sleep(.4)
  if not self.c.services.get_characteristic(SERIAL):raise RuntimeError("Hardware identity service missing")
  if self.c.services.get_characteristic(SERIAL):
   import json
   profile=self.profile or json.loads((Path(__file__).resolve().parents[1]/'device.json').read_text())
   actual=(await self.c.read_gatt_char(SERIAL)).decode()
   if actual!=profile['base_mac']:raise RuntimeError('Hardware identity mismatch')
  await self.c.start_notify(OS,lambda s,b:self.q.put_nowait(bytes(b).decode()))
  def uart(s,b):
   self.partial+=bytes(b)
   while b'\n' in self.partial:
    line,self.partial=self.partial.split(b'\n',1);self.lines.put_nowait(line.decode())
  await self.c.start_notify(TX,uart)
 async def op(self,payload):
  while not self.q.empty():self.q.get_nowait()
  await self.c.write_gatt_char(OP,payload,response=True)
  expected={0:'HELLO ',1:'START ',2:f'DATA {struct.unpack("<I",payload[1:5])[0]+len(payload)-5}' if payload[0]==2 and len(payload)>=5 else 'DATA ',3:'READY',4:'ABORTED',5:'REBOOT'}.get(payload[0],'')
  deadline=time.monotonic()+10
  while True:
   result=await asyncio.wait_for(self.q.get(),max(.01,deadline-time.monotonic()))
   if result.startswith('ERR ') or (result==expected if payload[0] in (2,3,4,5) else result.startswith(expected)):return result
 async def command(self,payload,expected='OK'):
  while not self.lines.empty():self.lines.get_nowait()
  await self.c.write_gatt_char(RX,payload,response=True)
  for _ in range(20):
   line=await asyncio.wait_for(self.lines.get(),5)
   if line.startswith('ERR ') and not expected.startswith('ERR '):raise RuntimeError(line)
   if line.startswith(expected):return line
  raise RuntimeError('No matching UART acknowledgment')
 async def begin(self,data,key,digest=None):
  hello=await self.op(b'\0');require(hello.startswith('HELLO 1 '),hello)
  nonce=bytes.fromhex(hello.split()[2]);meta=struct.pack('<I',len(data))+(digest or hashlib.sha256(data).digest())
  auth=hmac.digest(key,nonce+meta,'sha256')
  return await self.op(b'\1'+meta+auth)
 async def wait_data(self,base,end,timeout=3):
  deadline=time.monotonic()+timeout
  try:
   while True:
    ans=await asyncio.wait_for(self.q.get(),max(.01,deadline-time.monotonic()))
    if ans.startswith('ERR '):raise RuntimeError(ans)
    if ans.startswith('DATA '):
     received=int(ans.split()[1]);require(base<=received<=end,'Unexpected cumulative offset')
     if received==end:return False
  except TimeoutError:
   ans=await self.op(b'\0')
   require(ans.startswith('HELLO 1 ') and int(ans.split()[3])==end,'Transfer stalled: '+ans)
   return True
 async def upload(self,data,key,stop_after=None,progress=None,fast=False):
  require(len(key)==32,'Expected a 32-byte device key')
  ans=await self.begin(data,key);require(ans=='START 0',ans)
  start=time.monotonic();chunk=min(180,self.c.services.get_characteristic(OP).max_write_without_response_size-5)
  print(f'BLE payload={chunk} bytes',flush=True)
  if chunk<20:raise RuntimeError('Insufficient BLE payload size')
  nextlog=0;lastlog=0
  # Four packets per group, with GATT response flow control; firmware queue is 16.
  for base in range(0,len(data),chunk*4):
   ends=[]
   while not self.q.empty():self.q.get_nowait()
   for offset in range(base,min(base+chunk*4,len(data)),chunk):
    part=data[offset:offset+chunk];ends.append(offset+len(part))
    await self.c.write_gatt_char(OP,b'\2'+struct.pack('<I',offset)+part,response=(not fast or offset+len(part)==min(base+chunk*4,len(data))))
   # DATA offsets are cumulative: the firmware accepts only contiguous bytes.
   # A lost intermediate notification must not stall a fully received group.
   if await self.wait_data(base,ends[-1]):
    message=f'Recovered missing acknowledgment at offset {ends[-1]}'
    print(message,flush=True)
    if progress:progress(message)
   offset=ends[-1]
   if offset>=nextlog or time.monotonic()-lastlog>=5:
    message=f'OTA {100*offset/len(data):.1f}% bytes={offset} rate={offset/max(.1,time.monotonic()-start):.0f} B/s'
    print(message,flush=True)
    if progress:progress(message)
    nextlog=offset+len(data)//10;lastlog=time.monotonic()
   if stop_after and offset>=stop_after:return
  ans=await self.op(b'\3');require(ans=='READY',ans)
  print(f'OTA image accepted SHA256={hashlib.sha256(data).hexdigest()} elapsed={time.monotonic()-start:.1f}s',flush=True)
  ans=await self.op(b'\5');require(ans=='REBOOT',ans)
async def main(a):
 expected_build=None
 d=await find();print('BLE peripheral:',d.name,d.address,flush=True)
 async with BleakClient(d) as c:
  toy=Toy(c);await toy.start()
  if a.command:print(await toy.command((a.command+';').encode(),'STATUS' if a.command=='status' else 'OK'))
  if a.upload:
   profile=json.loads((Path(__file__).resolve().parents[1]/'device.json').read_text())
   data,m=load_image(a.upload,profile,allow_test=bool(a.expect));key=bytes.fromhex(Path(a.key).read_text().strip());await toy.upload(data,key,fast=a.fast)
   expected_build=a.expect or m['build']
  if a.test:
   for command,expect,check in [('reset','OK','period=1000'),('period 250','OK','period=250'),('duty 35','OK','duty=35'),('mode off','OK','mode=off'),('period 99','ERR command','period=250'),('period 10001','ERR command','period=250'),('status','STATUS','duty=35')]:
    r=await toy.command((command+';').encode(),expect);require(check in r,r);print('PASS',command,r,flush=True)
   await c.write_gatt_char(RX,b'period ',response=True)
   r=await toy.command(b'700;');require('period=700' in r,r);print('PASS fragmented command',r)
   b=bytes([33,66,54,49]);b+=bytes([(~sum(b))&255]);r=await toy.command(b);require('period=900' in r,r);print('PASS Bluefruit control-pad packet',r)
   r=await toy.command(b'!B61\0','ERR frame');require('period=900' in r,r);print('PASS bad packet rejected',r)
   r=await toy.command(b'reset;');require('period=1000' in r,r);print('PASS reset',r)
   print('LIVE CONTROL TESTS PASS',flush=True)
 if a.upload:
  await asyncio.sleep(2)
  async with BleakClient(await find()) as c:
   toy=Toy(c);await toy.start();line=await toy.command(b'status;','STATUS')
   if expected_build:require(f'build={expected_build} ' in line,'Unexpected running firmware: '+line)
   print('RECONNECTED AND VERIFIED',line,flush=True)
async def entry():
 p=argparse.ArgumentParser();p.add_argument('--fast',action='store_true',help='Four-packet groups, GATT flow control on final packet');p.add_argument('--command');p.add_argument('--upload');p.add_argument('--expect');p.add_argument('--key',default=str(Path(__file__).resolve().parents[1]/'backup/ota-key.hex'));p.add_argument('--test',action='store_true');await main(p.parse_args())
if __name__=='__main__':asyncio.run(entry())
