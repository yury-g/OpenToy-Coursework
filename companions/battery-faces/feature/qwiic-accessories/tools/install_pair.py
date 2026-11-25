"""Install current application only, on one identity-matched known device."""
import sys,time,subprocess,termios
from pathlib import Path
import serial
from serial.tools import list_ports
ROOT=Path(__file__).resolve().parents[1]
color=sys.argv[1]
mac={'blue':'02:00:00:00:00:01','green':'02:00:00:00:00:02','nesso':'02:00:00:00:00:05','yellow':'02:00:00:00:00:06','rainbow':'02:00:00:00:00:07'}[color]
binary=ROOT/('outputs/build-orange' if color=='nesso' else 'outputs/build-rainbow' if color=='rainbow' else 'outputs/build-yellow' if color=='yellow' else 'outputs/build')/'FeatherFace.ino.bin'
assert binary.stat().st_size < (0x140000 if color in ('nesso','rainbow') else 0x330000 if color=='yellow' else 0x2c0000),binary.stat().st_size
# All preflight checks happen before opening or resetting any device.
backup,verify,size={
 'green':('private-backup/green-ac276eb1cbb0','verify-original.log',0x400000),
 'blue':('private-backup','backup-verify.log',0x400000),
 'nesso':('private-backup/orange-48f6eec4d2bc','verify-consistent.log',0x1000000),
 'yellow':('private-backup/yellow-e8069093fc50','verify-original.log',0x1000000),
 'rainbow':('private-backup/rainbow-e8069093fcf8','verify-original.log',0x1000000),
}[color]
backup=ROOT/backup
assert (backup/'original-flash.bin').stat().st_size==size,'Original full backup required'
assert 'digest matched' in (backup/verify).read_text(),'Hardware digest verification required'
vid=0x303a if color in ('nesso','yellow','rainbow') else 0x239a
ports=[p for p in list_ports.comports() if (p.serial_number or '').lower()==mac and p.vid in ({vid,0x303a} if color in ('blue','green') else {vid})]
assert len(ports)==1,ports
port=ports[0].device
if color not in ('nesso','yellow','rainbow'):
 if ports[0].vid!=0x303a:
  try:
   with serial.Serial(port,1200,timeout=.2) as s:s.dtr=False
  except (OSError,termios.error) as e:
   # Native USB may disappear during open/flush as the 1200-baud reset succeeds.
   code=getattr(e,'errno',None) or (e.args[0] if e.args else None)
   if code!=6:raise
  time.sleep(2)
 ports=[p for p in list_ports.comports() if (p.serial_number or '').lower()==mac and p.vid==0x303a]
 assert len(ports)==1,[(p.device,p.serial_number,p.vid) for p in list_ports.comports()]
 port=ports[0].device
exe='/Users/example/Library/Arduino15/packages/esp32/tools/esptool_py/5.3.0/esptool'
args=[exe,'--port',port,'--before','default-reset' if color=='nesso' else 'no-reset','--after','hard-reset' if color=='nesso' else 'watchdog-reset']
if color=='nesso':args+=['--baud','921600']
else:args+=['--no-stub']
args+=['write-flash','0x10000',str(binary)]
if color in ('yellow','rainbow'):
 backup=ROOT/('private-backup/rainbow-e8069093fcf8' if color=='rainbow' else 'private-backup/yellow-e8069093fc50')
 assert (backup/'original-flash.bin').stat().st_size==0x1000000
 assert 'verify OK (digest matched)' in (backup/'verify-original.log').read_text()
 args=[str(ROOT/'.venv/bin/python'),'-m','esptool','--chip','esp32s3','--port',port,'--baud','921600','--before','usb_reset','--after','watchdog_reset','write_flash','0x10000',str(binary)]
with (ROOT/f'outputs/upload-v26-{color}.log').open('w') as log:subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,check=True)
print('Installed app only:',color,mac)
