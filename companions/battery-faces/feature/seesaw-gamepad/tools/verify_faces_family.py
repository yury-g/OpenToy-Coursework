"""Battery-only v18 family: replacementGreen, monochromeSilver, live telemetry/pages.
No flashing, battery tests, camera commands or retired-hardware access.
"""
from device_io import *
from monitor_io import *
from pathlib import Path
from contextlib import ExitStack
import json,datetime
ROOT=Path(__file__).resolve().parents[1]
proof={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'unavailable':[],'checks':[]}
with ExitStack() as stack:
 ports={}
 for c in MACS:
  try:ports[c]=stack.enter_context(open_device(c))
  except AssertionError:proof['unavailable'].append(c)
 assert {'blue','green','orange','purple','silver'}<=ports.keys()
 mon=stack.enter_context(open_monitor());time.sleep(4)
 def read():return {'devices':{c:status(s) for c,s in ports.items()},'monitor':monitor_status(mon)}
 try:
  end=time.monotonic()+20
  while True:
   initial=read()
   if all(fields(d['PEER'][target])['fresh']=='1' for c,d in initial['devices'].items() for target in ('green','silver') if target!=c) and all(fields(initial['monitor']['NODE'][target])['fresh']=='1' for target in ('green','silver')):break
   assert time.monotonic()<end,initial;time.sleep(.5)
  proof['initial']=initial
  for c,d in initial['devices'].items():
   assert fields(d['FACE'])['build']==BUILD and fields(d['FACE'])['mac']==MACS[c]
   assert fields(d['DISPLAY'])['selected']==c and fields(d['DISPLAY'])['page']=='0'
   assert fields(d['DEVICE'])['color']==c and fields(d['RADIO'])['ready']=='1'
   assert 'CAMERA' not in d and 'teal' not in d['PEER']
   assert fields(d['DIAG'])['outcome']!='1'
  assert fields(initial['devices']['green']['DEVICE'])['output']=='tft'
  assert fields(initial['devices']['green']['BATT'])['valid']=='1'
  assert fields(initial['devices']['silver']['HW'])['ready']=='1' and fields(initial['devices']['silver']['BATT'])['valid']=='1'
  h=fields(initial['devices']['orange']['HW']);assert h['cfg_before']==h['cfg_after']
  for target in ('green','silver'):
   v=float(fields(initial['devices'][target]['BATT'])['volts'])
   for c,d in initial['devices'].items():
    if c!=target:assert abs(float(fields(d['PEER'][target])['volts'])-v)<.07
   assert abs(float(fields(initial['monitor']['NODE'][target])['volts'])-v)<.07
  assert 'teal' not in initial['monitor']['NODE']
  proof['checks'].append('All connected devices start on their own battery face; replacementGreen TFT/gauge and Silver RLCD/voltage report correctly; both new batteries reach every connected peer and Tab5')
  for c,s in ports.items():screen(s,ROOT/f'outputs/v18-screen-{c}-shows-{c}.png')
  monitor_screen(mon,ROOT/'outputs/v18-tab5-screen.png',full=True)
  for c,s in ports.items():
   for target in ('green','silver'):
    command(s,'select '+target);time.sleep(.1);d=status(s);assert fields(d['DISPLAY'])['selected']==target
   command(s,'view trend');d=status(s);assert fields(d['DISPLAY'])['page']=='1'
   command(s,'view default')
  proof['checks'].append('Every connected companion browses Green/Silver, opens trend page and returns to own face; no camera pages remain')
  before=read();time.sleep(20);after=read();proof['idle']={'before':before,'after':after}
  for c,d in after['devices'].items():
   assert int(fields(d['FACE'])['frames'])>int(fields(before['devices'][c]['FACE'])['frames'])+50
   assert int(fields(d['RADIO'])['sent'])>=int(fields(before['devices'][c]['RADIO'])['sent'])+6
   assert fields(d['DISPLAY'])['page']=='0' and fields(d['DISPLAY'])['selected']==c
  for target in ('green','silver'):assert fields(after['monitor']['NODE'][target])['fresh']=='1'
  assert int(fields(after['monitor']['MONITOR'])['packets'])>int(fields(before['monitor']['MONITOR'])['packets'])
  proof['checks'].append('20seconds without USB reading: battery samples, face rendering and Tab5 updates continue; no battery test started')
  proof['passed']=True
 finally:
  for s in ports.values():command(s,'view default')
  proof['final']=read();(ROOT/'outputs/faces-family-v18-live.json').write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps({'passed':proof.get('passed',False),'checks':proof['checks'],'USB_unavailable':proof['unavailable']},indent=2))
