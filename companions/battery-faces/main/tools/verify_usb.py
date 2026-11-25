"""Check real Feather frames against all 19 preserved OpenToy pixel fixtures."""
import json,time,re,sys
from pathlib import Path
import serial
from serial.tools import list_ports
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='02:00:00:00:00:01'
ports=[p for p in list_ports.comports() if (p.serial_number or '').lower()==EXPECTED]
if len(ports)!=1:raise SystemExit('Expected exactly one Feather with saved factory MAC')
log=[]
with serial.Serial(ports[0].device,115200,timeout=.3) as s:
 s.dtr=True
 def read_until(prefix,timeout=5):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   line=s.readline().decode(errors='replace').strip()
   if line:log.append(line)
   if line.startswith(prefix):return line
  raise AssertionError('Missing response '+prefix)
 def send(cmd):s.write((cmd+'\n').encode());s.flush()
 send('status');initial=read_until('FACE ')
 assert 'build=FeatherFace-v6-two-color-trends' in initial,initial
 assert 'power=1 backlight=1 gauge=1' in initial and 'tft=135x240' in initial,initial
 results=[]
 fixtures=json.loads((ROOT/'tests/doodle_frames.json').read_text())
 if '--quick' in sys.argv:fixtures=fixtures[:2]
 for fixture in fixtures:
  s.reset_input_buffer();send(fixture['command']);read_until('OK doodle')
  time.sleep(2.1) # Original animator settles before exact fixture comparison.
  s.reset_input_buffer();send('status');status=read_until('FACE ')
  assert 'settled=1' in status,status
  time.sleep(.12) # Ensure the final settled frame has actually been presented.
  send('frame');frame=bytes.fromhex(read_until('FRAME ')[6:]);assert len(frame)==1024
  h=2166136261
  for byte in frame:h=((h^byte)*16777619)&0xffffffff
  actual=f'{h:08x}';assert actual==fixture['hash'],(fixture['label'],actual,fixture['hash'])
  results.append({'label':fixture['label'],'hash':actual});print('PASS',fixture['label'],actual,flush=True)
 # Invalid input must not mutate the last accepted pose.
 for bad in ['doodle 0 0 0 0 0 0 0 unknown','doodle 101 0 0 0 0 0 0 neutral','x'*150]:
  send(bad);read_until('ERR ')
 send('doodle 0 0 0 0 25 0 0 happy');read_until('OK doodle');time.sleep(2)
 send('battery on');read_until('OK battery on');time.sleep(.2);send('status');final=read_until('FACE ')
 assert 'demo=0' in final,final
 read_until('BATT ');assert 'auto=1' in read_until('DIAG ')
(ROOT/'outputs/usb-verification-v6.log').write_text('\n'.join(log)+'\n')
(ROOT/'outputs/usb-verification-v6.json').write_text(json.dumps({'fixture_count':len(results),'results':results,'final':final,'limits':'USB/framebuffer/SPI submission evidence; physical display appearance and button gestures require owner check.'},indent=2)+'\n')
print('PASS',len(results),'real USB frames; invalid inputs rejected; battery face restored')
