#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile the Nesso touch version; does not flash any device."""
import argparse,shutil,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-path',type=Path,required=True)
p.add_argument('--config-file',type=Path)
a=p.parse_args();root=Path(__file__).resolve().parents[1];build=a.build_path.resolve()
checkout=next((parent for parent in (root,*root.parents) if (parent/'.git').exists()),root)
if build.is_relative_to(checkout):p.error('Build outside the Git checkout')
build.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='nesso-eyes-',dir=build.parent) as tmp:
 sketch=Path(tmp)/'NessoEyes';shutil.copytree(root/'firmware/NessoEyes',sketch)
 for name in ('Protocol.h','FaceProtocol.h','EyePixels.h'):shutil.copy2(root/'shared/opentoy'/name,sketch/name)
 shutil.copy2(root/'shared/Telemetry.h',sketch/'Telemetry.h')
 cmd=['arduino-cli']
 if a.config_file:cmd+=['--config-file',str(a.config_file)]
 subprocess.run(cmd+['compile','--fqbn','esp32:esp32:arduino_nesso_n1:PartitionScheme=app3M_fat9M_16MB','--build-path',str(build),str(sketch)],check=True)
