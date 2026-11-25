#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile RLCD Eyes, never upload. Stage shared behavior outside Git."""
import argparse, shutil, subprocess, tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-path',type=Path,required=True)
p.add_argument('--revision',default='local')
a=p.parse_args();root=Path(__file__).resolve().parents[1];checkout=root.parents[1]
build=a.build_path.resolve();eyes=root.parent/'animatronic-eyes'
if build.is_relative_to(checkout):p.error('Build outside the Git checkout')
if not a.revision.replace('-','').replace('.','').isalnum():p.error('Invalid revision')
build.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='rlcd-eyes-',dir=build.parent) as tmp:
 sketch=Path(tmp)/'RlcdEyes';shutil.copytree(root/'firmware/RlcdEyes',sketch)
 for folder,names in [('shared/opentoy',['Protocol.h','FaceProtocol.h','EyePixels.h']),('shared',['Telemetry.h']),('firmware/AnimatronicEyes',['Controls.h','Personality.h','Companion.h']),('firmware/NessoEyes',['Idle.h','Presentation.h','TouchReaction.h'])]:
  for name in names:shutil.copy2(eyes/folder/name,sketch/name)
 (sketch/'BuildRevision.h').write_text('#define OPENTOY_BUILD_REV "'+a.revision+'"\n')
 subprocess.run(['arduino-cli','compile','--fqbn','esp32:esp32:esp32s3:FlashSize=16M,FlashMode=dio,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc,PartitionScheme=app3M_fat9M_16MB','--build-path',str(build),str(sketch)],check=True)
