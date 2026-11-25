#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate source-rendered 1-bit comparisons/animation; not device photographs."""
import hashlib,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1];out=root/'docs/images';out.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='rlcd-native-preview-') as tmp:
 tmp=Path(tmp);exe=tmp/'frames'
 subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14','-O2',str(root/'tools/preview_frames.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe),str(tmp)],check=True)
 sheet=Image.new('RGB',(824,716),'#24282c');draw=ImageDraw.Draw(sheet)
 draw.text((12,10),'BEFORE: enlarged pixels, light background',fill='white')
 draw.text((424,10),'AFTER: native contours, black background',fill='white')
 for row,mood in enumerate((100,0,-100)):
  y=42+row*220
  for col,mode in enumerate(('old','new')):
   im=Image.open(tmp/f'{mode}-{mood}.pgm').crop((0,40,400,240)).convert('RGB');sheet.paste(im,(12+412*col,y))
 draw.text((12,700),'Source simulations at panel resolution. Header omitted; no optical contrast simulation.',fill='white')
 sheet.save(out/'native-comparison.png')
 frames=[Image.open(tmp/f'frame-{i}.pgm').convert('P') for i in range(240)]
 frames[0].save(out/'native-eyes.gif',save_all=True,append_images=frames[1:],duration=50,loop=0,optimize=False)
 # Decode the output and compare every source frame, including GIF coalescing.
 gif=Image.open(out/'native-eyes.gif');index=0
 for i in range(gif.n_frames):
  gif.seek(i);repeat=gif.info['duration']//50
  for _ in range(repeat):
   assert gif.convert('L').tobytes()==frames[index].convert('L').tobytes();index+=1
 assert index==240
 sources=['firmware/RlcdEyes/NativeEyes.h','firmware/RlcdEyes/Display.h','tools/preview_frames.cpp']
 manifest={'kind':'source simulations, not hardware capture','version':'0.2.0','created':'2026-11-17','frames':240,'duration_ms':12000,'decoded_pixel_check':True,'sources':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in sources},'outputs':{n:hashlib.sha256((out/n).read_bytes()).hexdigest() for n in ('native-comparison.png','native-eyes.gif')}}
 (out/'native-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(out)
