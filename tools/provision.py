#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Identify and back up one 8 MB XIAO ESP32-S3 before preparing a local profile.
This prepares files only. The separate USB seed command verifies identity again.
"""
import argparse,hashlib,json,os,re,secrets,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def parse_identity(chip,flash):
 if not re.search(r'Chip (?:type|is):?\s*ESP32-S3\b',chip):raise ValueError('Expected ESP32-S3 chip identification')
 mac=re.search(r'MAC:\s*([0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5})\b',chip)
 if not mac:raise ValueError('Factory MAC not identified')
 if not re.search(r'Detected flash size:\s*8\s*MB\b',flash,re.I):raise ValueError('Expected 8 MB flash; inspect other hardware separately')
 return mac.group(1).lower()
def run(args):
 return subprocess.run(args,check=True,capture_output=True,text=True,encoding='utf-8').stdout

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--project',choices=['bare','oled-touch'],required=True);p.add_argument('--port',required=True);p.add_argument('--prepare',action='store_true',help='Read original flash, then create private local profile/key; no upload');a=p.parse_args()
 project=ROOT/'projects'/('xiao-toy' if a.project=='bare' else 'xiao-class')
 cmd=[sys.executable,'-m','esptool','--port',a.port]
 chip=run(cmd+['chip-id']);flash=run(cmd+['flash-id']);mac=parse_identity(chip,flash);print('Identified ESP32-S3, 8 MB flash, factory MAC',mac,'on',a.port)
 if not a.prepare:print('Inspection only. Add --prepare to back up and create a local profile.');return
 if (project/'device.json').exists() or (project/'firmware/ToyBLE/Secrets.h').exists():raise SystemExit('Profile/key already exists: preserve it. Use a separate checkout for another toy.')
 backup=project/'backup';backup.mkdir(exist_ok=True,mode=0o700);os.chmod(backup,0o700)
 original=backup/'original-flash-8MB.bin';partial=backup/'original-flash-8MB.partial'
 if original.exists() or partial.exists():raise SystemExit('Existing backup found: inspect it before continuing.')
 print('Reading original flash using ROM downloader…');run(cmd+['--no-stub','read-flash','0','0x800000',str(partial)])
 if partial.stat().st_size!=8388608:raise SystemExit('Backup size mismatch; no profile/key generated.')
 os.chmod(partial,0o600);digest=hashlib.sha256(partial.read_bytes()).hexdigest();partial.rename(original)
 (backup/'SHA256.txt').write_text(digest+'  original-flash-8MB.bin\n')
 (backup/'IDENTITY.json').write_text(json.dumps({'chip':'ESP32-S3','base_mac':mac,'flash_bytes':8388608,'original_sha256':digest},indent=2)+'\n')
 profile={'name':'XIAO Toy '+mac.replace(':','')[-4:].upper(),'base_mac':mac,'port':a.port,'peripherals':a.project=='oled-touch'}
 key=secrets.token_bytes(32);private=[(backup/'ota-key.hex',key.hex()+'\n'),(project/'firmware/ToyBLE/Secrets.h','#pragma once\nstatic const unsigned char OTA_KEY[32] = {'+','.join(str(v) for v in key)+'};\n'),(project/'device.json',json.dumps(profile,indent=2)+'\n')]
 for path,text in private:
  with path.open('x') as f:f.write(text)
  os.chmod(path,0o600)
 print('Original backup verified by size/SHA256. Local profile and unique key prepared. No flash was written.')
if __name__=='__main__':main()
