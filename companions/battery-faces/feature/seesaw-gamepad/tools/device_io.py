"""Fresh USB readback helpers for the three known battery companions."""
import serial,time,struct,zlib,binascii
from serial.tools import list_ports
MACS={'blue':'02:00:00:00:00:01','green':'02:00:00:00:00:02','pink':'02:00:00:00:00:05','yellow':'02:00:00:00:00:06','rainbow':'02:00:00:00:00:07','purple':'02:00:00:00:00:04','orange':'02:00:00:00:00:08','silver':'02:00:00:00:00:09'}
BUILD='BatteryFaces-v18-green-silver-faces'
def fields(line):return dict(x.split('=',1) for x in line.split()[1:] if '=' in x)
def open_device(color):
 if color=='purple':ps=[p for p in list_ports.comports() if p.vid==0x1a86 and p.pid==0x7523]
 else:ps=[p for p in list_ports.comports() if (p.serial_number or '').lower()==MACS[color] and p.vid==(0x303a if color in ('orange','yellow','rainbow','pink','silver') else 0x239a)]
 assert len(ps)==1,(color,ps)
 s=serial.Serial(port=None,baudrate=460800 if color=='purple' else 115200,timeout=.2);s.rts=False;s.dtr=color!='purple';s.port=ps[0].device;s.open();return s
def command(s,c):s.write((c+'\n').encode());s.flush()
def read_line(s):
 # Serial.readline() reads one byte at a time; native framebuffers are megabytes.
 # Buffer chunks so captures finish promptly without losing any line boundaries.
 if not hasattr(s,'_face_rx_buffer'):s._face_rx_buffer=bytearray()
 b=s._face_rx_buffer
 while True:
  n=b.find(b'\n')
  if n>=0:
   line=bytes(b[:n+1]);del b[:n+1];return line
  chunk=s.read(min(s.in_waiting,16384) or 1)
  if not chunk:return b''
  b.extend(chunk)
def status(s):
 # Serialized frame response is a barrier against previously queued reports.
 end=time.monotonic()+7;lastSend=0
 while time.monotonic()<end:
  if time.monotonic()-lastSend>=1:command(s,'frame');lastSend=time.monotonic()
  if read_line(s).startswith(b'FRAME '):break
 else:raise RuntimeError('No current command response')
 command(s,'status');end=time.monotonic()+5;out={}
 while time.monotonic()<end:
  x=read_line(s).decode(errors='replace').strip()
  if not x:continue
  key=x.split()[0]
  if key=='FACE':out={'PEER':{},'ACK':{},'FACE':x}
  if 'FACE' not in out:continue
  if key=='PEER':out['PEER'][fields(x)['color']]=x
  elif key=='ACK':out['ACK'][fields(x)['color']]=x
  elif key in ['FACE','BATT','DIAG','MICRO','DISPLAY','RADIO','POWER','HW','HAPTIC','BREATH','CREATURE','QWIIC_BUTTON','GAMEPAD','PERF','CAMERA','CAMERA_GATEWAY','YRADIO','GATEWAY','DEVICE']:out[key]=x
  if key=='DEVICE':return out
 raise RuntimeError(out)
def png(rows,path,scale=3,width=135,height=240):
 data=[]
 for row in rows:
  rgb=bytearray()
  for i in range(0,len(row),4):
   c=int(row[i:i+4],16);rgb.extend(bytes(((c>>11)*255//31,((c>>5)&63)*255//63,(c&31)*255//31))*scale)
  for _ in range(scale):data.append(b'\0'+rgb)
 def chunk(k,v):return struct.pack('>I',len(v))+k+v+struct.pack('>I',binascii.crc32(k+v)&0xffffffff)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width*scale,height*scale,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(data)))+chunk(b'IEND',b''))
def screen(s,path):
 command(s,'screen');end=time.monotonic()+15;start=False;rows=[];width=height=0
 while time.monotonic()<end:
  x=read_line(s).decode(errors='replace').strip()
  if x.startswith('SCREEN_BEGIN '):
   _,w,h=x.split();width,height=int(w),int(h);assert (width,height) in [(135,240),(240,280),(200,150)];start=True;rows=[]
  elif x=='SCREEN_END' and start:break
  elif start:rows.append(x)
 assert len(rows)==height and all(len(r)==width*4 for r in rows),(len(rows),width,height)
 png(rows,path,scale=1,width=width,height=height)
