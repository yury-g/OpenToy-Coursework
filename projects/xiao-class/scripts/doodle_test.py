#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Face v2 over real BLE; independently corroborate USB state, pixels and settling.
Controller input in this test is synthetic, never a claim of physical stick movement.
"""
import asyncio,json,re,threading,time,sys
from pathlib import Path
import serial
from bleak import BleakClient
from ble_client import Toy,find,require
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'xiao-toy/scripts'))
from controller_input import DoodleGamepadMapper

def fields(line):return dict(re.findall(r'(\w+)=([^ ]+)',line))
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
   toy=Toy(c,profile);await toy.start();r=await toy.command(b'status;','STATUS');require('doodle=1' in r.split(),'Face v2 capability absent')
   await toy.command(b'touch off;')
   async def check(command,digest=None,expected='OK',settle=False):
    beginning=len(lines);start=time.monotonic();ack=await toy.command((command+';').encode(),expected);latencies.append((time.monotonic()-start)*1000);seq=fields(ack)['seq'];deadline=time.monotonic()+5
    face=periph=None
    while time.monotonic()<deadline:
     faces=[s for s in lines[beginning:] if s.startswith('FACE seq='+seq+' ')];ps=[s for s in lines[beginning:] if s.startswith('PERIPH seq='+seq+' ')]
     face=fields(faces[-1]) if faces else None;periph=fields(ps[-1]) if ps else None
     if face and periph and (not settle or periph.get('settled')=='1'):break
     await asyncio.sleep(.15)
     if settle:await toy.command(b'status;','STATUS')
    require(face and periph and (not settle or periph.get('settled')=='1'),'USB settling/state absent: '+command)
    require(fields(ack)['txerr']=='0' and periph['i2c_ok']=='1' and periph['errors']=='0','BLE/I2C errors: '+str(periph))
    if command.startswith('doodle ') and expected=='OK':
     _,eye,brow,mouth,skew,jaw,left,right,mood=command.split()
     wanted=dict(doodle='1',eye=eye,brow=brow,mouth=mouth,skew=skew,jaw=jaw,left=left,right=right,expression=mood)
     require(all(face.get(k)==v for k,v in wanted.items()),'USB face differs: '+str(face))
    if digest:require(periph['frame']==digest,'Pixel mismatch '+command+': '+periph['frame']+' != '+digest)
    print('PASS',command,'seq='+seq,'USB frame='+periph['frame'],'settled='+periph['settled'],flush=True)
    return face,periph
   fixtures=json.loads((ROOT/'tests/doodle_frames.json').read_text())
   for f in fixtures:await check(f['command'],f['hash'],settle=True)
   before,periph=await check('doodle 100 -100 -100 100 100 100 0 nervous',fixtures[-1]['hash'],settle=True)
   for bad in ['doodle 101 0 0 0 0 0 0 happy','doodle 0 -101 0 0 0 0 0 happy','doodle 0 0 0 0 101 0 0 happy','doodle 0 0 0 0 0 101 0 happy','doodle 0 0 0 0 0 0 0 bogus']:
    after,_=await check(bad,periph['frame'],expected='ERR command',settle=True)
    require({k:v for k,v in after.items() if k!='seq'}=={k:v for k,v in before.items() if k!='seq'},'Invalid input changed target')
   # Observe multiple intermediate frames as a procedural transition evolves.
   beginning=len(lines);await check(fixtures[0]['command']);await asyncio.sleep(.15)
   await check(fixtures[1]['command'],fixtures[1]['hash'],settle=True)
   hashes={fields(s)['frame'] for s in lines[beginning:] if s.startswith('PERIPH ')}
   require(len(hashes)>3,'No USB evidence of evolving animation')
   print('PASS evolving animation:',len(hashes),'distinct USB framebuffer hashes',flush=True)
   # A hidden moving face must settle and stop transfers, on off and test screens.
   for mode,pose in [('off',fixtures[5]),('test',fixtures[3])]:
    _,moving=await check(pose['command']);require(moving['settled']=='0','Hidden-screen regression must start during motion');_,hidden=await check('screen '+mode)
    await asyncio.sleep(3);_,later=await check('status',expected='STATUS')
    require(later['settled']=='1' and later['frames']==hidden['frames'],'Hidden face kept drawing: '+str(later))
    print('PASS screen',mode,'during motion settles without continuous transfers',flush=True)
   mapper=DoodleGamepadMapper()
   for i in range(20):
    pose=mapper.sample((i%5-2)/2,i%3-1,(i%3-1),(i%5-2)/2,(i%4)/3,(i%5)/4,{'A':i==2,'RB':i==10},dpad=(i%3-1,0))
    await check(pose.command());await asyncio.sleep(.03)
   await check(fixtures[0]['command'],fixtures[0]['hash'],settle=True)
   _,idle=await check('status',expected='STATUS');await asyncio.sleep(2.3);_,later=await check('status',expected='STATUS');require(later['frames']==idle['frames'],'Idle face keeps transferring')
   await check(fixtures[2]['command']);beginning=len(lines);await c.disconnect();deadline=time.monotonic()+4
   found=None
   while time.monotonic()<deadline:
    found=next((s for s in lines[beginning:] if s.startswith('FACE ') and all(fields(s).get(k)==v for k,v in dict(doodle='1',expression='neutral',eye='0',brow='0',mouth='0',skew='0',jaw='0',left='0',right='0').items())),None)
    if found:break
    await asyncio.sleep(.05)
   require(found,'USB did not confirm disconnected neutral face')
   print('FACE V2 BLE + USB PASS:',len(fixtures),'exact pixel references, 20 synthetic controller poses, invalid atomicity, evolving/settled/hidden/idle frames, disconnect reset; ACK %.0f..%.0fms'%(min(latencies),max(latencies)),flush=True)
 finally:
  stop.set();thread.join(2);(ROOT/'evidence/doodle-usb-stream.log').write_text('\n'.join(lines)+'\n')
if __name__=='__main__':asyncio.run(main())
