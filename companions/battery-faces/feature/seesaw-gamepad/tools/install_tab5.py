"""Identity-checked app-only Tab5 install after verified private backup."""
from pathlib import Path
import subprocess
from serial.tools import list_ports
root=Path(__file__).resolve().parents[1]
ps=[p for p in list_ports.comports() if (p.serial_number or '').lower()=='02:00:00:00:00:0a' and p.vid==0x303a]
assert len(ps)==1,ps
backup=root/'private-backup/tab5-80f1b2d16bf1'
assert (backup/'original-flash.bin').stat().st_size==0x1000000
assert 'verify OK (digest matched)' in (backup/'verify-original.log').read_text()
bin=root/'outputs/build-monitor/FaceMonitor.ino.bin'
assert bin.stat().st_size <0xa00000
with (root/'outputs/upload-v25-tab5.log').open('w') as log:
 subprocess.run([str(root/'.venv/bin/python'),'-m','esptool','--chip','esp32p4','--port',ps[0].device,'--baud','921600','--before','usb_reset','--after','hard_reset','write_flash','0x10000',str(bin)],stdout=log,stderr=subprocess.STDOUT,check=True)
print('Installed app only: Tab5 02:00:00:00:00:0a')
