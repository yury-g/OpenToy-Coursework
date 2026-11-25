"""Install CYD app only; compare MAC on the same open ROM handle before writing."""
from pathlib import Path
import esptool,hashlib,json,contextlib
from serial.tools import list_ports
root=Path(__file__).resolve().parents[1]
original=root/'private-backup/purple-d4e9f4f3c5a0-original-flash.bin'
manifest=json.loads((root/'private-backup/purple-manifest.json').read_text())
assert original.stat().st_size==0x400000
assert hashlib.sha256(original.read_bytes()).hexdigest()==manifest['sha256']
assert 'digest matched' in (root/'private-backup/purple-verify-original.log').read_text()
app=[p for p in manifest['partitions'] if p['type']==0 and p['subtype']==0]
assert len(app)==1 and app[0]['offset']=='0x10000' and app[0]['size']=='0x100000'
binary=root/'outputs/build-purple/FeatherFace.ino.bin';assert binary.stat().st_size<0x100000,binary.stat().st_size
ports=[p for p in list_ports.comports() if p.vid==0x1a86 and p.pid==0x7523];assert len(ports)==1,ports
with (root/'outputs/upload-v18-purple.log').open('w') as log,contextlib.redirect_stdout(log):
 esp=esptool.ESP32ROM(ports[0].device,115200)
 try:
  esp.connect('default_reset')
  actual=':'.join(f'{x:02x}' for x in esp.read_mac());assert actual=='02:00:00:00:00:04',actual
  esptool.main(['--chip','esp32','--baud','460800','--before','no_reset','--after','hard_reset','write_flash','0x10000',str(binary)],esp=esp)
 finally:esp._port.close()
print('Installed app only: Purple 02:00:00:00:00:04')
