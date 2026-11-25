raise SystemExit("Retired verifier: Green is on a board-heat hold. Use verify_family.py for the active family.")
"""Read both known boards; briefly pause Green telemetry to verify stale UI. No battery tests/resets."""
import time,json,struct,zlib,binascii,datetime
from pathlib import Path
import serial
from serial.tools import list_ports
ROOT=Path(__file__).resolve().parents[1]
MACS={'blue':'02:00:00:00:00:01','green':'02:00:00:00:00:03'}
BUILD='FeatherFace-v9-d0-battery-toggle'
log=[]
def fields(line):return dict(x.split('=',1) for x in line.split()[1:] if '=' in x)
def command(s,c):s.write((c+'\n').encode());s.flush()
def group(s):
 s.reset_input_buffer();command(s,'frame');barrier=time.monotonic()+6
 while time.monotonic()<barrier:
  x=s.readline().decode(errors='replace').strip()
  if x.startswith('FRAME '):break
 else:raise AssertionError('No fresh command barrier')
 command(s,'status');out={};end=time.monotonic()+5
 while time.monotonic()<end:
  x=s.readline().decode(errors='replace').strip()
  if not x:continue
  log.append(x)
  key=x.split()[0]
  if key=='FACE':out={}
  if key in ['FACE','BATT','DIAG','MICRO','DISPLAY','RADIO','PEER','DEVICE']:out[key]=x
  if key=='DEVICE' and len(out)==8:return out
 raise AssertionError(out)
def png(rows,path,scale=3):
 rgb=[]
 for row in rows:
  rr=bytearray()
  for i in range(0,len(row),4):
   c=int(row[i:i+4],16);pixel=bytes(((c>>11)*255//31,((c>>5)&63)*255//63,(c&31)*255//31));rr.extend(pixel*scale)
  for _ in range(scale):rgb.append(b'\0'+rr)
 def chunk(k,v):return struct.pack('>I',len(v))+k+v+struct.pack('>I',binascii.crc32(k+v)&0xffffffff)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',135*scale,240*scale,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(rgb)))+chunk(b'IEND',b''))
def screen(s,name):
 s.reset_input_buffer();command(s,'screen');end=time.monotonic()+8;started=False;rows=[]
 while time.monotonic()<end:
  x=s.readline().decode(errors='replace').strip()
  if x=='SCREEN_BEGIN 135 240':started=True
  elif x=='SCREEN_END':break
  elif started:rows.append(x)
 assert len(rows)==240 and all(len(x)==540 for x in rows),len(rows)
 png(rows,ROOT/f'outputs/v9-screen-{name}.png')
ports={}
for c,m in MACS.items():
 ps=[p for p in list_ports.comports() if (p.serial_number or '').lower()==m and p.vid==0x239a]
 assert len(ps)==1,ps
 ports[c]=ps[0].device
def open_device(port):
 s=serial.Serial(port=None,baudrate=115200,timeout=.2);s.rts=False;s.dtr=True;s.port=port;s.open();return s
with open_device(ports['blue']) as blue,open_device(ports['green']) as green:
 blue.rts=green.rts=False;blue.dtr=green.dtr=True
 g=group(green);b=group(blue)
 for c,r in [('blue',b),('green',g)]:
  assert f'build={BUILD}' in r['FACE'] and f'mac={MACS[c]}' in r['FACE'],r
  assert fields(r['BATT'])['valid']=='1' and fields(r['RADIO'])['ready']=='1',r
 assert 'power=1 backlight=0 gauge=1' in g['FACE'],g
 assert fields(b['PEER'])['fresh']=='1' and fields(b['PEER'])['valid']=='1',b
 assert abs(float(fields(g['BATT'])['pct'])-float(fields(b['PEER'])['pct']))<.2,(g,b)
 command(blue,'select blue');command(blue,'view trend');time.sleep(.15);screen(blue,'blue')
 command(blue,'select green');time.sleep(2.1);selected=group(blue)
 assert 'selected=green' in selected['DISPLAY'],selected
 screen(blue,'green')
 # D2's shared action must not start a local or remote session in Green view.
 command(blue,'test start');time.sleep(.1);guard=group(blue)
 assert fields(guard['DIAG'])['outcome']!='1',guard
 try:
  command(green,'radio off');deadline=time.monotonic()+13
  while time.monotonic()<deadline:
   lost=group(blue)
   if fields(lost['PEER'])['fresh']=='0':break
   time.sleep(.5)
  assert fields(lost['PEER'])['fresh']=='0',lost
  time.sleep(.15);screen(blue,'offline')
 finally:command(green,'radio on')
 deadline=time.monotonic()+6
 while time.monotonic()<deadline:
  resumed=group(blue)
  if fields(resumed['PEER'])['fresh']=='1':break
  time.sleep(.2)
 assert fields(resumed['PEER'])['fresh']=='1',resumed
 assert int(fields(resumed['PEER'])['rx'])>int(fields(b['PEER'])['rx']),resumed
 command(blue,'select blue');command(blue,'view trend');command(blue,'battery on');time.sleep(.15)
 final=group(blue);gfinal=group(green)
 deadline=time.monotonic()+4
 while fields(gfinal['RADIO'])['paused']!='0' and time.monotonic()<deadline:gfinal=group(green)
 assert fields(gfinal['RADIO'])['paused']=='0',gfinal
 assert fields(final['DISPLAY'])['selected']=='blue'
 result={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'blue':final,'green':gfinal,'selected_green':selected,'green_test_guard':guard,'lost':lost,'resumed':resumed,'passed':['Both known boards and valid gauges','Green LCD backlight off/shared I2C rail on','Blue received live Green readings over ESP-NOW','Full-size blue and green framebuffer captures','Remote view cannot start battery session','Green transmitter pause -> offline after8sec','Green transmitter resume -> live readings return','Blue automatic trend view restored'],'actual_battery_tests_started':False,'physical_D0':'Debounce handler toggles same showGreen state; owner visual/button confirmation pending.'}
 (ROOT/'outputs/wireless-toggle-v9-live.json').write_text(json.dumps(result,indent=2)+'\n')
 (ROOT/'outputs/wireless-toggle-v9-live.log').write_text('\n'.join(log)+'\n')
 for c,r in [('blue',final),('green',gfinal)]:print(c,r['BATT'],r['RADIO'])
 print('PASS:', '; '.join(result['passed']))
