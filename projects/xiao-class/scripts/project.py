#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Reproducible project operations. No Arduino IDE needed."""
import argparse,glob,hashlib,json,os,re,secrets,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
(ROOT/'evidence').mkdir(exist_ok=True)
FQBN='esp32:esp32:XIAO_ESP32S3:EraseFlash=none'
EXPECTED_MAC=json.loads((ROOT/'device.json').read_text())['base_mac']
def run(args,**kw):
 if kw.get("text"):kw.setdefault("encoding","utf-8")
 return subprocess.run(args,check=True,**kw)
def esptool():
 tools=glob.glob(str(Path.home()/'Library/Arduino15/packages/esp32/tools/esptool_py/5.3.0/esptool'))
 return [tools[0]] if tools else [sys.executable,'-m','esptool']
def identify(port):
 r=run(esptool()+['--port',port,'chip-id'],capture_output=True,text=True)
 if 'ESP32-S3' not in r.stdout or EXPECTED_MAC not in r.stdout.lower():raise SystemExit('Refusing write: expected ESP32-S3/MAC not identified.')
 print('Confirmed expected ESP32-S3 on',port)
p=argparse.ArgumentParser();p.add_argument('action',choices=['build','upload-usb','restore-original','new-key']);p.add_argument('--port',default=json.loads((ROOT/'device.json').read_text())['port']);p.add_argument('--label',default='Workshop');p.add_argument('--rollback-test',action='store_true');a=p.parse_args()
if not re.fullmatch('[A-Za-z0-9_-]{1,32}',a.label):raise SystemExit('Invalid build label')
if a.action=='new-key':
 if (ROOT/'firmware/ToyBLE/Secrets.h').exists():raise SystemExit('Key already exists; refusing to change the installed device key.')
 key=secrets.token_bytes(32);(ROOT/'backup').mkdir(exist_ok=True)
 (ROOT/'backup/ota-key.hex').write_text(key.hex()+'\n');os.chmod(ROOT/'backup/ota-key.hex',0o600)
 (ROOT/'firmware/ToyBLE/Secrets.h').write_text('#pragma once\nstatic const unsigned char OTA_KEY[32] = {'+','.join(str(b) for b in key)+'};\n');os.chmod(ROOT/'firmware/ToyBLE/Secrets.h',0o600)
elif a.action=='build':
 installed=json.loads(run(['arduino-cli','core','list','--format','json'],capture_output=True,text=True).stdout)
 if not any(x['id']=='esp32:esp32' and x.get('installed_version')=='3.3.10' for x in installed['platforms']):raise SystemExit('Required ESP32 core 3.3.10 is not installed')
 if json.loads((ROOT/'device.json').read_text()).get('peripherals'):
  required={'Adafruit SH110X':'2.1.14','Adafruit GFX Library':'1.12.6','Adafruit BusIO':'1.17.4'}
  libs=json.loads(run(['arduino-cli','lib','list','--format','json'],capture_output=True,text=True).stdout)
  actual={x['library']['name']:x['library']['version'] for x in libs['installed_libraries']}
  if any(actual.get(n)!=v for n,v in required.items()):raise SystemExit('Required pinned Adafruit display libraries are not installed')
 if not (ROOT/'firmware/ToyBLE/Secrets.h').exists():raise SystemExit('Run new-key for a new device first; existing device needs its original key.')
 (ROOT/'firmware/ToyBLE/BuildConfig.h').write_text('// SPDX-License-Identifier: MIT\n#pragma once\n#define TOY_BUILD "'+a.label+'"\n#define TOY_FAIL_BOOT '+str(int(a.rollback_test))+'\n')
 out=ROOT/'firmware/ToyBLE/build'/a.label
 run(['arduino-cli','compile','--fqbn',FQBN,'--output-dir',str(out),str(ROOT/'firmware/ToyBLE')])
 b=(out/'ToyBLE.ino.bin').read_bytes();digest=hashlib.sha256(b).hexdigest();print('APP SHA256',digest)
 (out/'manifest.json').write_text(json.dumps({'target':'esp32:esp32:XIAO_ESP32S3','core':'3.3.10','build':a.label,'bytes':len(b),'sha256':digest,'rollback_test':a.rollback_test,'device_mac':EXPECTED_MAC,'peripherals':True},indent=2)+'\n')
elif a.action=='upload-usb':
 identify(a.port)
 run(['arduino-cli','upload','--fqbn',FQBN,'--port',a.port,'--input-dir',str(ROOT/'firmware/ToyBLE/build'/a.label),str(ROOT/'firmware/ToyBLE')])
elif a.action=='restore-original':
 b=ROOT/'backup/original-flash-8MB.bin';expected=(ROOT/'backup/SHA256.txt').read_text().split()[0]
 if b.stat().st_size!=8388608 or hashlib.sha256(b.read_bytes()).hexdigest()!=expected:raise SystemExit('Backup validation failed')
 identify(a.port)
 run(esptool()+['--chip','esp32s3','--port',a.port,'write-flash','0x0',str(b)])
