raise SystemExit("Use verify_family.py; the old four-face verifier assumes Green is active.")
"""Actual source routing, selected UIs, source expiry and recovery. No battery tests."""
from device_io import *
from monitor_io import *
from pathlib import Path
from contextlib import ExitStack
import json,datetime
ROOT=Path(__file__).resolve().parents[1]
proof={'checks':[],'usb_unavailable':[]}
with ExitStack() as stack:
 ports={}
 for c in MACS:
  try:ports[c]=stack.enter_context(open_device(c))
  except AssertionError:
   assert c=='green',c
   proof['usb_unavailable'].append(c)
 monitor=stack.enter_context(open_monitor())
 initial={c:status(s) for c,s in ports.items()};sources={c:fields(r['BATT']) for c,r in initial.items()}
 if 'green' not in sources:
  sources['green']=fields(initial['blue']['PEER']['green'])
  proof['green_source_evidence']='USB unavailable; compare independent Blue/Orange/Yellow wireless receptions, not a direct USB measurement'
 for c,r in initial.items():
  assert f'build={BUILD}' in r['FACE'] and f'mac={MACS[c]}' in r['FACE'],r
  assert fields(r['BATT'])['valid']=='1' and fields(r['RADIO'])['ready']=='1',r
  assert fields(r['DIAG'])['outcome']!='1',r
  for other in MACS:
   if other==c:continue
   peer=fields(r['PEER'][other]);own=sources[other]
   assert peer['fresh']=='1' and peer['valid']=='1',(c,other,peer)
   assert abs(float(peer['pct'])-float(own['pct']))<(3 if other=='yellow' else 1),(c,other,peer,own)
   assert abs(float(peer['volts'])-float(own['volts']))<.04,(c,other,peer,own)
 if 'green' in initial:assert 'backlight=0' in initial['green']['HW'] and 'power=1' in initial['green']['HW']
 assert fields(initial['yellow']['HW'])['touch']=='1' and fields(initial['yellow']['HW'])['framebuffer']=='1'
 assert fields(initial['yellow']['POWER'])['current_valid']=='0'
 assert fields(initial['orange']['POWER'])['current_valid']=='1'
 assert fields(initial['orange']['HW'])['panel']=='1' and fields(initial['orange']['HW'])['touch']=='1'
 proof['checks']+=['Four live sources agree across available receivers',f'{len(ports)*3} directly observed wireless paths match source/consensus','Orange display/current/gauge and Yellow display/voltage available','No battery test running']
 proof['initial']=initial;proof['selections']={}
 for viewer in ['orange','blue','yellow']:
  proof['selections'][viewer]={}
  for source in MACS:
   began=time.monotonic();command(ports[viewer],'select '+source);command(ports[viewer],'view face');time.sleep(.12)
   group=status(ports[viewer]);assert fields(group['DISPLAY'])['selected']==source,group
   screen(ports[viewer],ROOT/f'outputs/v13-screen-{viewer}-shows-{source}.png')
   group['capture_duration_ms']=round((time.monotonic()-began)*1000);proof['selections'][viewer][source]=group
 proof['checks'].append('Twelve actual full-size selected screens captured across Blue/Orange/Yellow')
 proof['loss_tests']={}
 for source,viewers in [('yellow',['blue','orange']),('orange',['blue','yellow'])]:
  for viewer in viewers:command(ports[viewer],'select '+source)
  try:
   command(ports[source],'radio off');deadline=time.monotonic()+15;lost={}
   while time.monotonic()<deadline:
    lost={c:status(ports[c]) for c in viewers};lostMonitor=monitor_status(monitor)
    if all(fields(r['PEER'][source])['fresh']=='0' for r in lost.values()) and fields(lostMonitor['NODE'][source])['fresh']=='0':break
    time.sleep(.2)
   assert all(fields(r['PEER'][source])['fresh']=='0' for r in lost.values()),lost
   assert fields(lostMonitor['NODE'][source])['fresh']=='0',lostMonitor
   for c in viewers:screen(ports[c],ROOT/f'outputs/v13-screen-{c}-{source}-offline.png')
   monitor_screen(monitor,ROOT/f'outputs/v13-tab5-{source}-offline.png',full=True)
  finally:command(ports[source],'radio on')
  deadline=time.monotonic()+8
  while time.monotonic()<deadline:
   recovered={c:status(ports[c]) for c in viewers};recoveredMonitor=monitor_status(monitor)
   if all(fields(r['PEER'][source])['fresh']=='1' for r in recovered.values()) and fields(recoveredMonitor['NODE'][source])['fresh']=='1':break
   time.sleep(.2)
  assert all(fields(r['PEER'][source])['fresh']=='1' for r in recovered.values()),recovered
  assert fields(recoveredMonitor['NODE'][source])['fresh']=='1',recoveredMonitor
  proof['loss_tests'][source]=dict(lost=lost,lost_monitor=lostMonitor,resumed=recovered,resumed_monitor=recoveredMonitor)
  proof['checks'].append(f'{source} pause expires source on both remote face displays and Tab5; resume restores live')
 for c in ['orange','blue','yellow']:
  command(ports[c],'select '+c);command(ports[c],'battery on');command(ports[c],'view trend' if c=='blue' else 'view face')
 proof['final']={c:status(s) for c,s in ports.items()}
 for c,r in proof['final'].items():
  for other,peer in r['PEER'].items():assert int(fields(peer)['rx'])>int(fields(initial[c]['PEER'][other])['rx']),(c,other)
 proof['checks'].append('All observable RX counters advance; own-face selections restored')
proof['recorded_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();proof['actual_battery_tests_started']=False
(ROOT/'outputs/four-faces-v13-live.json').write_text(json.dumps(proof,indent=2)+'\n')
for c,r in proof['final'].items():print(c,r['BATT'],r['POWER'])
for x in proof['checks']:print('PASS',x)
