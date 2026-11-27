#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build Feather with staged copies of the canonical Nesso model/renderer. Never flashes USB."""
import argparse,shutil,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-path',type=Path,required=True)
p.add_argument('--config-file',type=Path)
p.add_argument('--build-property',action='append',default=[])
a=p.parse_args();root=Path(__file__).resolve().parents[1];build=a.build_path.resolve()
if build.is_relative_to(root.parents[1]):p.error('Keep build output outside the Git checkout.')
build.parent.mkdir(parents=True,exist_ok=True)
# Arduino library discovery does not consistently honor extra include paths.
# Stage the shared files at build time, keeping one authoritative source in Git.
with tempfile.TemporaryDirectory(prefix='water-feather-source-',dir=build.parent) as tmp:
 sketch=Path(tmp)/'GravityWaterFeather'
 shutil.copytree(root/'firmware/GravityWaterFeather',sketch)
 shutil.copytree(root/'firmware/GravityWaterNesso/src/fluid',sketch/'src/fluid')
 shutil.copy2(root/'firmware/GravityWaterNesso/NessoPixels.h',sketch/'NessoPixels.h')
 shutil.copy2(root/'firmware/GravityWaterNesso/WaterOverlay.h',sketch/'WaterOverlay.h')
 cmd=['arduino-cli']
 if a.config_file:cmd+=['--config-file',str(a.config_file)]
 cmd+=['compile','--fqbn','esp32:esp32:adafruit_feather_esp32s3_reversetft:PartitionScheme=tinyuf2','--build-path',str(build),'--build-property','compiler.cpp.extra_flags=-std=gnu++14']
 for prop in a.build_property:cmd+=['--build-property',prop]
 subprocess.run(cmd+[str(sketch)],check=True)
