raise SystemExit("Retired verifier: Green is on a board-heat hold. Use verify_family.py for the active family.")
"""Read-only monitor/source comparison and native rendered framebuffer capture."""
from device_io import *
from monitor_io import *
import datetime,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
with open_monitor() as s:
 before=monitor_status(s);m=fields(before['MONITOR'])
 assert m['connected']=='1' and m['panel']=='1' and m['width']=='1280' and m['height']=='720',before
 assert all(fields(v)['fresh']=='1' and fields(v)['valid']=='1' for v in before['NODE'].values()),before
 sources={};unavailable=[]
 for c in MACS:
  try:
   with open_device(c) as source:sources[c]=status(source)
  except AssertionError:
   assert c=='green',c
   unavailable.append(c)
 observations={c:fields(r['BATT']) for c,r in sources.items()}
 if 'green' in unavailable:
  consensus=[fields(x['PEER']['green']) for x in sources.values()]
  assert all(x['fresh']=='1' and x['valid']=='1' for x in consensus),consensus
  assert max(float(x['volts']) for x in consensus)-min(float(x['volts']) for x in consensus)<.04,consensus
  sources['green']={'evidence':'Three independent wireless receptions; Green USB unavailable','consensus':consensus};observations['green']=consensus[0]
 for c,own in observations.items():
  remote=fields(before['NODE'][c]);assert abs(float(own['pct'])-float(remote['pct']))<(3 if c=='yellow' else 1),(c,own,remote)
  assert abs(float(own['volts'])-float(remote['volts']))<.04,(c,own,remote)
 monitor_screen(s,ROOT/'outputs/v13-tab5-screen.png',full=True)
 after=monitor_status(s);assert int(fields(after['MONITOR'])['packets'])>int(m['packets']),after
 result=dict(recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),before=before,after=after,sources=sources,usb_unavailable=unavailable,passed=['Tab5 connected to Blue gateway','All four live readings match source USB/wireless evidence','1280x720 panel initialized','Actual native framebuffer captured','Packet counter advances'],actual_battery_tests_started=False)
 (ROOT/'outputs/tab5-v13-live.json').write_text(json.dumps(result,indent=2)+'\n')
 for c,x in after['NODE'].items():print(x)
 print('PASS',result['passed'])
