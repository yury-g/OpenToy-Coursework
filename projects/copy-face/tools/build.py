#!/usr/bin/env python3
"""Stage and build the camera prototype. No flash or serial operations."""
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--work', type=Path, required=True)
a=p.parse_args(); root=Path(__file__).resolve().parents[1]; repo=root.parents[1]
work=a.work.resolve()
if work==repo or repo in work.parents:p.error('Use a private build directory outside Git')
stage=work/'source';build=work/'build'
shutil.copytree(root/'firmware',stage,dirs_exist_ok=True)
shutil.copytree(root/'core',stage/'main/core',dirs_exist_ok=True)
shutil.copy2(root/'locks/camera-idf5.4.2.lock',stage/'dependencies.lock')
imported=stage/'main/imported';imported.mkdir(exist_ok=True)
eyes=repo/'projects/animatronic-eyes'
for folder,names in [('firmware/AnimatronicEyes',['Controls.h','Personality.h','Companion.h']),
 ('firmware/NessoEyes',['TouchReaction.h','Idle.h','Presentation.h']),
 ('shared/opentoy',['Protocol.h','FaceProtocol.h','EyePixels.h'])]:
 for name in names:shutil.copy2(eyes/folder/name,imported/name)
metadata=json.loads((root/'firmware/metadata.json').read_text())
digest=hashlib.sha256()
sources=list((root/'firmware').rglob('*'))+list((root/'core').rglob('*'))+list(imported.glob('*.h'))
for src in sorted(x for x in sources if x.is_file()):
 digest.update(src.name.encode());digest.update(src.read_bytes())
metadata['source_sha256']=digest.hexdigest()
header='// Generated from canonical metadata and staged source.\n#pragma once\nnamespace copy_face {\n'
for name,key in [('appName','display_name_short'),('version','firmware_version'),('created','version_created'),('source','source_sha256')]:
 header+='constexpr const char* '+name+'='+json.dumps(metadata[key])+';\n'
(stage/'main/Metadata.h').write_text(header+'}\n');(stage/'version.txt').write_text(metadata['firmware_version'])
(work/'build-record.json').write_text(json.dumps(metadata,indent=2)+'\n')
subprocess.run(['idf.py','-C',str(stage),'-B',str(build),'build'],check=True)
config=dict(line.split('=',1) for line in (stage/'sdkconfig').read_text().splitlines() if line.startswith('CONFIG_') and '=' in line)
for key,value in {'CONFIG_IDF_TARGET':'"esp32p4"','CONFIG_SPIRAM_SPEED':'200','CONFIG_ESPTOOLPY_FLASHSIZE':'"16MB"','CONFIG_PARTITION_TABLE_OFFSET':'0x8000','CONFIG_LV_COLOR_DEPTH':'16'}.items():
 if config.get(key)!=value:raise RuntimeError(f'Unexpected {key}: {config.get(key)}')
image=build/'copy_face.bin'
metadata.update(image_bytes=image.stat().st_size,image_sha256=hashlib.sha256(image.read_bytes()).hexdigest())
if metadata['image_bytes']>0xb00000:raise RuntimeError('Image exceeds preserved 11 MiB app slot')
(work/'build-record.json').write_text(json.dumps(metadata,indent=2)+'\n')
print('Built only:',image,'bytes:',metadata['image_bytes'])
