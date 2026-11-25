"""Capture real touchscreen events; no injected touch input or battery tests."""
from device_io import *
from pathlib import Path
import datetime,json
root=Path(__file__).resolve().parents[1]
lines=[]
with open_device('orange') as s:
 before=status(s);command(s,'select orange');command(s,'view face');status(s)
 command(s,'stream on')
 print('READY for physical swipes',flush=True)
 end=time.monotonic()+180
 while time.monotonic()<end:
  line=s.readline().decode(errors='replace').strip()
  if line:lines.append({'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'line':line})
  if line.startswith(('TOUCH ','GESTURE ')):print(line,flush=True)
  gestures=[x for x in lines if x['line'].startswith('GESTURE ')]
  actions=[fields(x['line'])['action'] for x in gestures]
  if actions.count('left')>=4 and actions.count('right')>=1 and actions.count('tap')>=1:break
 command(s,'stream off');after=status(s)
result={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'before':before,'after':after,'events':[x for x in lines if x['line'].startswith(('TOUCH ','GESTURE '))],'physical_input_only':True}
(root/'outputs/orange-touch-v13-live.json').write_text(json.dumps(result,indent=2)+'\n')
(root/'outputs/orange-touch-v13-live.log').write_text('\n'.join(x['recorded_utc']+' '+x['line'] for x in lines)+'\n')
print('CAPTURE SAVED',len(result['events']),'touch/gesture events',flush=True)
