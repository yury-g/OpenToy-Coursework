#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Stage and compile only, with an activated ESP-IDF 5.4.2 environment. Never flashes."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
from config_check import validate_config
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--work',type=Path,required=True,help='Private directory outside the checkout')
a=p.parse_args();root=Path(__file__).resolve().parents[1];repo=root.parents[1];work=a.work.resolve()
if work==repo or repo in work.parents:p.error('Build outside Git')
stage=work/'source';build=work/'build';stage.mkdir(parents=True,exist_ok=True)
shutil.copytree(root/'firmware',stage,dirs_exist_ok=True)
shutil.copy2(root/'locks/idf5.4.2.lock',stage/'dependencies.lock')
eyes=repo/'projects/animatronic-eyes';imported=stage/'main/imported';imported.mkdir(exist_ok=True)
sources=list((root/'firmware').rglob('*'))
for folder,names in [('firmware/AnimatronicEyes',['Controls.h','Personality.h','Companion.h']),('firmware/NessoEyes',['TouchReaction.h','Idle.h','Presentation.h']),('shared/opentoy',['Protocol.h','FaceProtocol.h','EyePixels.h'])]:
 for name in names:
  src=eyes/folder/name;sources.append(src);shutil.copy2(src,imported/name)
digest=hashlib.sha256()
for src in sorted(x for x in sources if x.is_file()):
 digest.update(str(src.relative_to(repo)).encode());digest.update(src.read_bytes())
metadata=json.loads((root/'firmware/metadata.json').read_text());metadata['source_sha256']=digest.hexdigest()
header='// Generated from metadata.json and staged source digests.\n#pragma once\nnamespace crowpanel {\n'
for name,key in [('appName','display_name_short'),('version','firmware_version'),('created','version_created'),('source','source_sha256')]:
 header+='constexpr const char* '+name+'='+json.dumps(metadata[key])+';\n'
(stage/'main/Metadata.h').write_text(header+'}\n');(stage/'version.txt').write_text(metadata['firmware_version'])
(work/'build-record.json').write_text(json.dumps(metadata,indent=2)+'\n')
subprocess.run(['idf.py','-C',str(stage),'-B',str(build),'build'],check=True)
validate_config((stage/'sdkconfig').read_text())
image=build/'crowpanel_eyes.bin';metadata['image_sha256']=hashlib.sha256(image.read_bytes()).hexdigest();metadata['image_bytes']=image.stat().st_size
(work/'build-record.json').write_text(json.dumps(metadata,indent=2)+'\n')
print('Built only:',image,'bytes:',metadata['image_bytes'],'source:',metadata['source_sha256'])
