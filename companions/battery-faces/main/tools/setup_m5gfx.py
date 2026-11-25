"""Copy installed M5GFX 0.2.25 locally and apply the Tab5 charger-preservation patch."""
from pathlib import Path
import subprocess,shutil,json
root=Path(__file__).resolve().parents[1]
sketchbook=Path(subprocess.check_output(['arduino-cli','config','get','directories.user'],text=True).strip())
source=sketchbook/'libraries/M5GFX'
assert 'version=0.2.25' in (source/'library.properties').read_text(),'Install M5GFX@0.2.25 first'
target=root/'vendor/M5GFX'
if target.exists():
 assert 'BatteryFaces: preserve both Tab5 charger' in (target/'src/M5GFX.cpp').read_text(),'Existing vendor differs; inspect before replacing'
 print('Already patched:',target)
else:
 shutil.copytree(source,target)
 subprocess.run(['patch','-p1','-i',str(root/'patches/M5GFX-0.2.25-preserve-tab5-charger.patch')],cwd=target,check=True)
 print('Prepared:',target)
