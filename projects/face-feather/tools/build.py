#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Stage original face renderers and proven Feather telemetry; compile only."""
import argparse,shutil,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--version',type=int,choices=(1,2),required=True)
p.add_argument('--build-path',type=Path,required=True)
p.add_argument('--config-file',type=Path,required=True)
p.add_argument('--build-property',action='append',default=[])
a=p.parse_args();root=Path(__file__).resolve().parents[3];build=a.build_path.resolve()
if build.is_relative_to(root):p.error('Build outside the checkout')
build.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='face-source-',dir=build.parent) as tmp:
 sketch=Path(tmp)/'FaceFeather';shutil.copytree(root/'projects/face-feather/firmware/FaceFeather',sketch)
 for name in ('Protocol.h','FaceProtocol.h','EyePixels.h','DoodleProtocol.h','DoodlePixels.h'):
  shutil.copy2(root/'projects/xiao-class/firmware/ToyBLE'/name,sketch/name)
 shutil.copy2(root/'projects/gravity-water/firmware/GravityWaterFeather/Telemetry.h',sketch/'Telemetry.h')
 subprocess.run(['arduino-cli','--config-file',str(a.config_file),'compile','--fqbn','esp32:esp32:adafruit_feather_esp32s3_reversetft:PartitionScheme=tinyuf2','--build-path',str(build),'--build-property',f'compiler.cpp.extra_flags=-std=gnu++14 -DFACE_VERSION={a.version}',*[item for prop in a.build_property for item in ('--build-property',prop)],str(sketch)],check=True)
