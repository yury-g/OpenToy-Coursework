"""Create GitHub's GIF fallback and checksum every final retained image."""
from pathlib import Path
from PIL import Image
import hashlib,json,datetime,zipfile
assets=Path(__file__).resolve().parents[1]/'assets'
frames=[Image.open(assets/f'animation/mood-{i}.png').convert('RGB') for i in range(5)]
frames[0].save(assets/'animation/battery-mood.gif',save_all=True,append_images=frames[1:],duration=2000,loop=0,disposal=2,optimize=False)
with zipfile.ZipFile(assets/'battery-faces-images.zip','w',zipfile.ZIP_DEFLATED) as archive:
 for p in sorted(assets.rglob('*')):
  if p.is_file() and p.suffix in ('.png','.svg','.gif'):archive.write(p,str(p.relative_to(assets)))
manifest={'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'kind':'Real framebuffer snapshots; original illustrative board drawings; simulated charge/mood animation (not live)','files':{str(p.relative_to(assets)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(assets.rglob('*')) if p.is_file() and p.name!='manifest.json'}}
(assets/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Saved five-frame GIF and final asset checksums:',len(manifest['files']),'files')
