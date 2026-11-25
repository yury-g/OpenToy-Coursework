#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import asyncio,struct,hashlib
from pathlib import Path
from bleak import BleakClient
from ble_client import Toy,find
async def main():
 key=bytes.fromhex((Path(__file__).resolve().parents[1]/'backup/ota-key.hex').read_text().strip())
 images=list((Path(__file__).resolve().parents[1]/'firmware/ToyBLE/build').glob('*/ToyBLE.ino.bin'))
 data=max(images,key=lambda p:p.stat().st_mtime).read_bytes()[:1024]
 async with BleakClient(await find()) as c:
  t=Toy(c);await t.start()
  assert await t.begin(data,bytes(32))=='ERR auth';print('PASS wrong authentication key rejected',flush=True)
  assert await t.begin(data,key)=='START 0'
  assert await t.op(b'\3')=='ERR incomplete';print('PASS incomplete image cannot activate',flush=True)
  assert await t.op(b'\2'+struct.pack('<I',1)+data[:20])=='ERR offset';print('PASS out-of-order bytes rejected',flush=True)
  assert await t.op(b'\4')=='ABORTED';print('PASS explicit abort',flush=True)
  assert await t.begin(data,key,digest=bytes(32))=='START 0'
  assert await t.op(b'\2'+struct.pack('<I',0)+data[:20])=='DATA 20'
  # Continue remaining data in small packets.
  for o in range(20,len(data),120):
   part=data[o:o+120];assert await t.op(b'\2'+struct.pack('<I',o)+part)==f'DATA {o+len(part)}'
  assert await t.op(b'\3')=='ERR sha256';print('PASS corrupted digest rejected before activation',flush=True)
  assert await t.begin(data,key)=='START 0'
  for o in range(0,len(data),120):
   part=data[o:o+120];assert await t.op(b'\2'+struct.pack('<I',o)+part)==f'DATA {o+len(part)}'
  r=await t.op(b'\3');assert r.startswith('ERR image'),r;print('PASS truncated firmware image rejected by image validation',r,flush=True)
  assert await t.begin(data,key)=='START 0'
  assert await t.op(b'\2'+struct.pack('<I',0)+data[:20])=='DATA 20'
  print('Disconnect during partial upload',flush=True)
 async with BleakClient(await find()) as c:
  t=Toy(c);await t.start();r=await t.command(b'status;','STATUS');assert 'ota=0' in r,r
  assert await t.begin(data,key)=='START 0';assert await t.op(b'\4')=='ABORTED'
  print('PASS disconnected transfer aborted; same firmware reconnects',r,flush=True)
 print('OTA NEGATIVE TESTS PASS',flush=True)
asyncio.run(main())
