"""New-member app-only install; backup digest, table and same-ROM-handle identity guards."""
from pathlib import Path
import sys,json,hashlib,contextlib,esptool
from serial.tools import list_ports
root=Path(__file__).resolve().parents[1]
name=sys.argv[1]
mac,folder,sketch,offset,capacity={
 'pink':('02:00:00:00:00:08','pink-14c19fd5978c','FeatherFace',0x10000,0x100000),
 'camera':('02:00:00:00:00:0b','camera-10b41dd2a2c0','FaceCamera',0x1d0000,0x1c0000),
}[name]
backup=root/'private-backup'/folder;m=json.loads((backup/'manifest.json').read_text());data=(backup/'original-flash.bin').read_bytes()
assert len(data)==0x800000 and hashlib.sha256(data).hexdigest()==m['sha256']
assert 'digest matched' in (backup/'verify-original.log').read_text()
assert any(p['type']==0 and p['offset']==offset and p['size']==capacity for p in m['partitions'])
if name=='camera':assert m['active_app_offset']==offset
binary=root/f'outputs/build-{name}/{sketch}.ino.bin';assert binary.stat().st_size<capacity,binary.stat().st_size
ports=[p for p in list_ports.comports() if (p.serial_number or '').lower()==mac and p.vid==0x303a];assert len(ports)==1,ports
with (root/f'outputs/upload-v17-{name}.log').open('w') as log,contextlib.redirect_stdout(log):
 esp=esptool.get_default_connected_device([ports[0].device],ports[0].device,7,115200,chip='esp32s3',before='usb_reset')
 try:
  actual=':'.join(f'{v:02x}' for v in esp.read_mac());assert actual==mac,actual
  esptool.main(['--chip','esp32s3','--baud','921600','--before','no_reset','--after','watchdog_reset','write_flash',hex(offset),str(binary)],esp=esp)
 finally:esp._port.close()
print('Installed app only:',name,mac,hex(offset))
