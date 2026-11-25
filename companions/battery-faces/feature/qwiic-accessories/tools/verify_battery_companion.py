"""Live UI/auto-face verification; never starts an actual battery test."""
import json,time
from pathlib import Path
import serial
from serial.tools import list_ports
root=Path(__file__).resolve().parents[1]
p=next(p for p in list_ports.comports() if (p.serial_number or '').lower()=='02:00:00:00:00:01')
lines=[]
with serial.Serial(p.device,115200,timeout=.25) as s:
 s.dtr=True;s.rts=False
 def send(cmd):s.write((cmd+'\n').encode());s.flush()
 def read(prefix,timeout=6):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   line=s.readline().decode(errors='replace').strip()
   if line:lines.append(line)
   if line.startswith(prefix):return line
  raise AssertionError('Missing '+prefix)
 send('status');face=read('FACE ');assert 'build=FeatherFace-v6-two-color-trends' in face and 'tft=135x240' in face
 batt=read('BATT ')
 if 'valid=0' in batt:time.sleep(3);send('status');read('FACE ');batt=read('BATT ')
 assert 'valid=1' in batt,batt
 diag=read('DIAG ');assert 'auto=1' in diag and 'outcome=0' in diag,diag
 pct=float(batt.split('pct=')[1].split()[0]);expected='happy' if pct>70 else 'neutral' if pct>=45 else 'sorry' if pct>=20 else 'angry'
 time.sleep(2);send('status');face=read('FACE ');assert f'mood={expected}' in face,face
 send('view watch');read('OK view watch');send('status');read('FACE ');read('BATT ');diag=read('DIAG ');assert 'page=2' in diag
 send('view runtime');read('OK view runtime');send('test start');action=read('OK test action ')
 assert any(x in action for x in ['WAIT 2 MIN','UNPLUG USB']),action
 send('status');read('FACE ');read('BATT ');diag=read('DIAG ');assert 'outcome=0' in diag,diag
 send('view trend');read('OK view trend');send('battery on');read('OK battery on');send('status');final=read('FACE ');batt=read('BATT ');diag=read('DIAG ')
 assert 'auto=1' in diag and 'page=1' in diag and 'outcome=0' in diag,diag
(root/'outputs/battery-companion-v6-live.log').write_text('\n'.join(lines)+'\n')
(root/'outputs/battery-companion-v6-live.json').write_text(json.dumps({'auto_mood':expected,'battery':batt,'final':final,'diagnostics':diag,'runtime_start_blocked':action,'actual_tests_started':False,'limits':'No 7-minute physical charging conclusion, full-charge cycle, battery health certification, or runtime endurance test.'},indent=2)+'\n')
print('PASS live battery face, both test pages, guarded runtime start, auto face restored; no battery test started')
print(batt);print(diag)
