#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build the enrolled Waveshare V2 target. Never uploads; keep output private."""
import argparse,shutil,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-path',type=Path,required=True);p.add_argument('--revision',default='local');a=p.parse_args()
root=Path(__file__).resolve().parents[1];build=a.build_path.resolve();checkout=root.parents[1]
if build.is_relative_to(checkout):p.error('Build outside the Git checkout')
if not a.revision.replace('-','').replace('.','').isalnum():p.error('revision must be alphanumeric with optional dash/dot')
build.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='waveshare-eyes-',dir=build.parent) as tmp:
 sketch=Path(tmp)/'WaveshareEyes';shutil.copytree(root/'firmware/WaveshareEyes',sketch)
 for folder,names in [('shared/opentoy',['Protocol.h','FaceProtocol.h','EyePixels.h']),('shared',['Telemetry.h']),('firmware/AnimatronicEyes',['Controls.h','Personality.h','Companion.h']),('firmware/NessoEyes',['Idle.h','Presentation.h','TouchReaction.h'])]:
  for name in names:shutil.copy2(root/folder/name,sketch/name)
 (sketch/'BuildRevision.h').write_text('#define OPENTOY_BUILD_REV "'+a.revision+'"\n')
 subprocess.run(['arduino-cli','compile','--fqbn','esp32:esp32:esp32s3:FlashSize=16M,FlashMode=dio,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc,PartitionScheme=app3M_fat9M_16MB','--build-path',str(build),str(sketch)],check=True)
