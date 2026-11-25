"""Regression: attached USB hosts that stop reading must not stall faces/radio."""
from device_io import *
from contextlib import ExitStack
from pathlib import Path
import json,datetime
with ExitStack() as stack:
 ports={}
 for c in MACS:
  try:ports[c]=stack.enter_context(open_device(c))
  except AssertionError:assert c=='green',c
 before={c:status(s) for c,s in ports.items()}
 for c,s in ports.items():
  if c in ('orange','yellow','rainbow'):command(s,'stream on')
 time.sleep(20) # deliberately do not consume any USB debug output
 after={}
 for c,s in ports.items():
  command(s,'stream off');after[c]=status(s)
  frames=int(fields(after[c]['FACE'])['frames'])-int(fields(before[c]['FACE'])['frames'])
  sent=int(fields(after[c]['RADIO'])['sent'])-int(fields(before[c]['RADIO'])['sent'])
  assert frames>=150 and sent>=8,(c,frames,sent)
  assert all(fields(p)['fresh']=='1' for p in after[c]['PEER'].values()),after[c]
  print('PASS no-reader USB',c,'frames',frames,'radio samples',sent)
 result=dict(recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),before=before,after=after,host_read_pause_seconds=20,battery_tests_started=False)
 Path('outputs/usb-idle-v15-live.json').write_text(json.dumps(result,indent=2)+'\n')
