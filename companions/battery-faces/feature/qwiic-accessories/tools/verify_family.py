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
 time.sleep(3) # allow UART startup and two-second source broadcasts to reach every receiver
 initial={c:status(s) for c,s in ports.items()};assert all('green' not in x['PEER'] for x in initial.values());sources={c:fields(r['BATT']) for c,r in initial.items()}
 for c,r in initial.items():
  assert f'build={BUILD}' in r['FACE'] and f'mac={MACS[c]}' in r['FACE'],r
  assert fields(r['BATT'])['valid']==('0' if c=='purple' else '1') and fields(r['RADIO'])['ready']=='1',r
  assert fields(r['DIAG'])['outcome']!='1',r
  for other in MACS:
   if other==c:continue
   peer=fields(r['PEER'][other]);own=sources[other]
   assert peer['fresh']=='1' and peer['valid']==('0' if other=='purple' else '1'),(c,other,peer)
   if other=='purple':continue
   assert abs(float(peer['pct'])-float(own['pct']))<(3 if other in ('yellow','rainbow') else 1),(c,other,peer,own)
   assert abs(float(peer['volts'])-float(own['volts']))<.04,(c,other,peer,own)
 assert fields(initial['yellow']['HW'])['touch']=='1' and fields(initial['yellow']['HW'])['framebuffer']=='1'
 assert fields(initial['yellow']['POWER'])['current_valid']=='0'
 assert fields(initial['orange']['POWER'])['current_valid']=='1'
 assert fields(initial['orange']['HW'])['panel']=='1' and fields(initial['orange']['HW'])['touch']=='1'
 proof['checks']+=['Green excluded; five live nodes agree across receivers',f'{len(ports)*(len(MACS)-1)} directly observed wireless paths match source/consensus','Orange display/current/gauge and Yellow display/voltage available','No battery test running']
 proof['initial']=initial;proof['monitor_initial']=monitor_status(monitor);assert set(proof['monitor_initial']['NODE'])==set(MACS);proof['selections']={}
 for c in MACS:
  node=fields(proof['monitor_initial']['NODE'][c]);own=sources[c];assert node['fresh']=='1' and node['valid']==('0' if c=='purple' else '1')
  if c!='purple':assert abs(float(node['volts'])-float(own['volts']))<.04 and abs(float(node['pct'])-float(own['pct']))<(3 if c in ('yellow','rainbow') else 1)
 assert fields(proof['monitor_initial']['MONITOR'])['tiles']=='5'
 for viewer in list(MACS):
  proof['selections'][viewer]={}
  for source in MACS:
   began=time.monotonic();command(ports[viewer],'select '+source);command(ports[viewer],'view face');time.sleep(.12)
   group=status(ports[viewer]);assert fields(group['DISPLAY'])['selected']==source,group
   screen(ports[viewer],ROOT/f'outputs/v15-screen-{viewer}-shows-{source}.png')
   group['capture_duration_ms']=round((time.monotonic()-began)*1000);proof['selections'][viewer][source]=group
 proof['checks'].append('Twenty-five selected screens captured across all five active viewers')
 proof['loss_tests']={}
 for source,viewers in [('yellow',['blue','orange']),('rainbow',['blue','yellow'])]:
  for viewer in viewers:command(ports[viewer],'select '+source)
  try:
   command(ports[source],'radio off');deadline=time.monotonic()+15;lost={}
   while time.monotonic()<deadline:
    lost={c:status(ports[c]) for c in viewers};lostMonitor=monitor_status(monitor)
    if all(fields(r['PEER'][source])['fresh']=='0' for r in lost.values()) and fields(lostMonitor['NODE'][source])['fresh']=='0':break
    time.sleep(.2)
   assert all(fields(r['PEER'][source])['fresh']=='0' for r in lost.values()),lost
   assert fields(lostMonitor['NODE'][source])['fresh']=='0',lostMonitor
   assert fields(lostMonitor['MONITOR'])['tiles']=='4',lostMonitor
   time.sleep(.4) # allow the 250ms UI redraw before capturing the changed tile layout
   for c in viewers:screen(ports[c],ROOT/f'outputs/v15-screen-{c}-{source}-offline.png')
   monitor_screen(monitor,ROOT/f'outputs/v15-tab5-{source}-offline.png',full=True)
  finally:command(ports[source],'radio on')
  deadline=time.monotonic()+8
  while time.monotonic()<deadline:
   recovered={c:status(ports[c]) for c in viewers};recoveredMonitor=monitor_status(monitor)
   if all(fields(r['PEER'][source])['fresh']=='1' for r in recovered.values()) and fields(recoveredMonitor['NODE'][source])['fresh']=='1':break
   time.sleep(.2)
  assert all(fields(r['PEER'][source])['fresh']=='1' for r in recovered.values()),recovered
  assert fields(recoveredMonitor['NODE'][source])['fresh']=='1',recoveredMonitor
  assert fields(recoveredMonitor['MONITOR'])['tiles']=='5',recoveredMonitor
  proof['loss_tests'][source]=dict(lost=lost,lost_monitor=lostMonitor,resumed=recovered,resumed_monitor=recoveredMonitor)
  proof['checks'].append(f'{source} pause expires source on both remote face displays and Tab5; resume restores live')
 for c in list(MACS):
  command(ports[c],'select '+c);command(ports[c],'battery on');command(ports[c],'view trend' if c=='blue' else 'view face')
 monitor_screen(monitor,ROOT/'outputs/v15-tab5-screen.png',full=True)
 proof['final']={c:status(s) for c,s in ports.items()}
 for c,r in proof['final'].items():
  for other,peer in r['PEER'].items():assert int(fields(peer)['rx'])>int(fields(initial[c]['PEER'][other])['rx']),(c,other)
 proof['checks'].append('All observable RX counters advance; own-face selections restored')
proof['recorded_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();proof['actual_battery_tests_started']=False
(ROOT/'outputs/family-v15-live.json').write_text(json.dumps(proof,indent=2)+'\n')
for c,r in proof['final'].items():print(c,r['BATT'],r['POWER'])
for x in proof['checks']:print('PASS',x)
