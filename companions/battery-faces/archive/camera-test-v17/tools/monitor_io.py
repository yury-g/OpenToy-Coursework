"""Read-only Tab5 status and framebuffer access."""
from device_io import *
import uuid
def open_monitor():
 ps=[p for p in list_ports.comports() if (p.serial_number or '').lower()=='02:00:00:00:00:0a']
 assert len(ps)==1,ps
 s=serial.Serial(port=None,baudrate=115200,timeout=.25);s.rts=False;s.dtr=True;s.port=ps[0].device;s.open();return s
def monitor_status(s):
 token=uuid.uuid4().hex[:12];command(s,'sync '+token);end=time.monotonic()+8;fresh=False;r={'NODE':{},'CAMERA_SOURCE':{}}
 while time.monotonic()<end:
  line=read_line(s).decode(errors='replace').strip()
  if line=='SYNC '+token:fresh=True
  if not fresh:continue
  if line.startswith('CAMERA_SOURCE '):r['CAMERA_SOURCE'][fields(line)['source']]=line
  if line.startswith('TAB_POWER '):r['TAB_POWER']=line
  if line.startswith('TAB_CAMERA '):r['TAB_CAMERA']=line
  if line.startswith('CAMERA_TCP '):r['CAMERA_TCP']=line
  if line.startswith('CAMERA '):r['CAMERA']=line
  if line.startswith('MONITOR '):r['MONITOR']=line
  elif line.startswith('NODE '):r['NODE'][fields(line)['color']]=line
  if 'MONITOR' in r and len(r['NODE'])==len(MACS):return r
 raise RuntimeError(r)
def _monitor_screen(s,path,full=False):
 command(s,'screen full' if full else 'screen');end=time.monotonic()+60;start=False;rows=[]
 width,height=(1280,720) if full else (320,180)
 while time.monotonic()<end:
  line=read_line(s).decode(errors='replace').strip()
  if line==f'SCREEN_BEGIN {width} {height}':start=True;rows=[]
  elif line=='SCREEN_END' and start:break
  elif start:rows.append(line)
 assert len(rows)==height and all(len(x)==width*4 for x in rows),len(rows)
 png(rows,path,scale=1 if full else 3,width=width,height=height)

def monitor_screen(s,path,full=False):
 for attempt in range(3):
  monitor_status(s) # command barrier before a potentially large USB export
  try:return _monitor_screen(s,path,full)
  except AssertionError:
   if attempt==2:raise
