raise SystemExit("Retired verifier: Green is on a board-heat hold. Use verify_family.py for the active family.")
"""Actual telemetry/UI/loss tests on all three known devices. No battery tests or resets."""
from device_io import *
from pathlib import Path
from contextlib import ExitStack
import json,datetime
ROOT=Path(__file__).resolve().parents[1]
proof={'checks':[]}
with ExitStack() as stack:
 ports={c:stack.enter_context(open_device(c)) for c in MACS}
 initial={c:status(s) for c,s in ports.items()}
 for c,r in initial.items():
  assert f'build={BUILD}' in r['FACE'] and f'mac={MACS[c]}' in r['FACE'],r
  assert fields(r['BATT'])['valid']=='1' and fields(r['RADIO'])['ready']=='1',r
  assert fields(r['DIAG'])['outcome']!='1',r
  for other in MACS:
   if other==c:continue
   peer=fields(r['PEER'][other]);own=fields(initial[other]['BATT'])
   assert peer['fresh']=='1' and peer['valid']=='1',(c,other,r)
   assert abs(float(peer['pct'])-float(own['pct']))<1,(c,other,peer,own)
   assert abs(float(peer['volts'])-float(own['volts']))<.04,(c,other,peer,own)
 assert 'backlight=0' in initial['green']['HW'] and 'power=1' in initial['green']['HW'],initial['green']
 assert fields(initial['orange']['POWER'])['current_valid']=='1',initial['orange']
 assert fields(initial['orange']['HW'])['panel']=='1' and fields(initial['orange']['HW'])['touch']=='1',initial['orange']
 proof['checks']+=['Three identities/builds/gauges valid','All six direct wireless paths match source battery','Green backlight off/shared I2C rail on','Orange native display/touch/current available','No battery test running']
 proof['initial']=initial;proof['selections']={}
 for viewer in ['orange','blue']:
  proof['selections'][viewer]={}
  for battery in ['orange','blue','green']:
   command(ports[viewer],'select '+battery);command(ports[viewer],'view face');time.sleep(.12)
   group=status(ports[viewer]);assert fields(group['DISPLAY'])['selected']==battery,group
   screen(ports[viewer],ROOT/f'outputs/v11-screen-{viewer}-shows-{battery}.png')
   proof['selections'][viewer][battery]=group
 proof['checks'].append('All three full-size battery/face views captured on Orange and Blue')
 # Test stale handling from an actual transmitter pause; never interrupt charging.
 command(ports['orange'],'select green');command(ports['blue'],'select green')
 try:
  command(ports['green'],'radio off');deadline=time.monotonic()+14;lost={}
  while time.monotonic()<deadline:
   lost={c:status(ports[c]) for c in ['orange','blue']}
   if all(fields(r['PEER']['green'])['fresh']=='0' for r in lost.values()):break
   time.sleep(.25)
  assert all(fields(r['PEER']['green'])['fresh']=='0' for r in lost.values()),lost
  for c in ['orange','blue']:screen(ports[c],ROOT/f'outputs/v11-screen-{c}-green-offline.png')
  proof['lost']=lost
 finally:command(ports['green'],'radio on')
 deadline=time.monotonic()+7
 while time.monotonic()<deadline:
  recovered={c:status(ports[c]) for c in ['orange','blue']}
  if all(fields(r['PEER']['green'])['fresh']=='1' for r in recovered.values()):break
  time.sleep(.25)
 assert all(fields(r['PEER']['green'])['fresh']=='1' for r in recovered.values()),recovered
 proof['resumed']=recovered;proof['checks'].append('Green transmitter pause -> offline on both displays, resume -> live')
 for c in ['orange','blue']:
  command(ports[c],'select '+c);command(ports[c],'battery on');command(ports[c],'view face' if c=='orange' else 'view trend')
 proof['final']={c:status(s) for c,s in ports.items()}
 for c,r in proof['final'].items():
  for other,peer in r['PEER'].items():assert int(fields(peer)['rx'])>int(fields(initial[c]['PEER'][other])['rx']),(c,other)
 proof['checks'].append('All six RX counters advance; selections restored')
proof['recorded_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
proof['actual_battery_tests_started']=False
(ROOT/'outputs/three-faces-v11-live.json').write_text(json.dumps(proof,indent=2)+'\n')
for c,r in proof['final'].items():print(c,r['BATT'],r['POWER'])
for x in proof['checks']:print('PASS',x)
