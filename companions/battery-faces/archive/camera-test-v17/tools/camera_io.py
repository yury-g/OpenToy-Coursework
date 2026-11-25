"""Fresh, nonce-barrier camera diagnostics; queued command replies are ignored."""
from device_io import *
import json,uuid
def camera_status(s):
 token=uuid.uuid4().hex[:12];command(s,'sync '+token);end=time.monotonic()+5;fresh=False
 while time.monotonic()<end:
  line=read_line(s).decode(errors='replace').strip()
  if line=='SYNC '+token:fresh=True
  if fresh and line.startswith('{'):
   d=json.loads(line);assert d['mac']==MACS['teal'] and d['role']=='camera';return d
 raise RuntimeError('No current camera diagnostic response')
