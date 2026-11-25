"""Record physical user touch events only; no synthetic swipes or battery tests."""
from device_io import *
from pathlib import Path
import json,datetime
log=[]
with open_device('purple') as s:
 before=status(s);command(s,'select purple');command(s,'view face')
 end=time.monotonic()+60
 while time.monotonic()<end:
  line=read_line(s).decode(errors='replace').strip()
  if line.startswith(('TOUCH ','GESTURE ')):log.append(line);print(line,flush=True)
 after=status(s)
 command(s,'select purple');command(s,'view face')
r=dict(recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),before=before,after=after,events=log,physical_test_only=True)
Path('outputs/purple-touch-v15.json').write_text(json.dumps(r,indent=2)+'\n')
print('Recorded',len(log),'physical events; own face restored',flush=True)
