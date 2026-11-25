#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Stage original face renderers and proven Feather telemetry; compile only."""
import argparse,shutil,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--version',type=int,choices=(1,2),required=True)
p.add_argument('--personality',choices=('classic','curious','dreamy','cheeky'),default='classic')
p.add_argument('--power',choices=('auto','normal','eco'),default='auto')
p.add_argument('--build-path',type=Path,required=True)
p.add_argument('--config-file',type=Path,required=True)
p.add_argument('--build-property',action='append',default=[])
a=p.parse_args()
if a.version==2 and a.personality!='classic':p.error('Personalities are Eyes 1 only')
personality=('classic','curious','dreamy','cheeky').index(a.personality)
eco=a.power=='eco' or (a.power=='auto' and personality>0)
if eco and (a.version!=1 or personality==0):p.error('Eco power requires an Eyes 1 personality')
root=Path(__file__).resolve().parents[1];build=a.build_path.resolve()
checkout=next((parent for parent in (root,*root.parents) if (parent/'.git').exists()),root)
if build.is_relative_to(checkout):p.error('Build outside the checkout')
build.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='face-source-',dir=build.parent) as tmp:
 sketch=Path(tmp)/'AnimatronicEyes';shutil.copytree(root/'firmware/AnimatronicEyes',sketch)
 for name in ('Protocol.h','FaceProtocol.h','EyePixels.h','DoodleProtocol.h','DoodlePixels.h'):
  shutil.copy2(root/'shared/opentoy'/name,sketch/name)
 shutil.copy2(root/'shared/Telemetry.h',sketch/'Telemetry.h')
 shutil.copy2(root/'shared/TinyFont3x5.h',sketch/'TinyFont3x5.h')
 shutil.copy2(root/'shared/SoundWake.h',sketch/'SoundWake.h')
 for name in ('Idle.h','TouchReaction.h','Presentation.h'):
  shutil.copy2(root/'firmware/NessoEyes'/name,sketch/name)
 subprocess.run(['arduino-cli','--config-file',str(a.config_file),'compile','--fqbn','esp32:esp32:adafruit_feather_esp32s3_reversetft:PartitionScheme=tinyuf2','--build-path',str(build),'--build-property',f'compiler.cpp.extra_flags=-std=gnu++14 -DFACE_VERSION={a.version} -DEYES_PERSONALITY={personality} -DEYES_ECO={int(eco)}',*[item for prop in a.build_property for item in ('--build-property',prop)],str(sketch)],check=True)
