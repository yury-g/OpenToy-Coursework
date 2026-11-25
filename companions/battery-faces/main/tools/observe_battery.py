import serial,time,json
from pathlib import Path
from serial.tools import list_ports
end=time.monotonic()+50
while time.monotonic()<end:
 p=next((p for p in list_ports.comports() if (p.serial_number or '').lower()=='02:00:00:00:00:01' and p.vid==0x239a),None)
 if p:break
 time.sleep(.5)
else:raise SystemExit('App USB has not appeared; physical Reset still needed')
lines=[]
with serial.Serial(p.device,115200,timeout=.3) as s:
 s.dtr=True;s.rts=False
 s.write(b'\nstatus\n')
 end=time.monotonic()+12
 while time.monotonic()<end:
  line=s.readline().decode(errors='replace').strip()
  if line.startswith(('FACE ','BATT ','BUTTON ')):lines.append(line);print(line,flush=True)
assert any('build=FeatherFace-v4-large-readout' in x and 'tft=135x240' in x for x in lines)
assert any('BATT valid=1' in x for x in lines),lines
Path('outputs/battery-live-v4.log').write_text('\n'.join(lines)+'\n')
