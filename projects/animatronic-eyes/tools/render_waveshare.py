#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Source-rendered illustrative Waveshare tour; no device access or actual readout."""
import os,shlex,subprocess,tempfile,json,hashlib
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1];out=root/'docs/images';out.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='waveshare-render-') as tmp:
 exe=Path(tmp)/'frames';subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14','-O2',str(root/'tools/waveshare_frames.cpp'),'-o',str(exe)],check=True)
 data=subprocess.check_output([str(exe)]);size=360*360*3
 assert len(data)==size*270
 frames=[]
 for i in range(270):
  im=Image.new('RGB',(400,418),'#182029');screen=Image.frombytes('RGB',(360,360),data[i*size:(i+1)*size]);mask=Image.new('L',(360,360));ImageDraw.Draw(mask).ellipse((0,0,359,359),fill=255);im.paste(screen,(20,28),mask)
  d=ImageDraw.Draw(im);d.text((20,8),'WAVESHARE EYES 1 - SOURCE SIMULATION',fill='white')
  frame=i%90;label='LEFT POKE' if 22<=frame<25 else 'RIGHT POKE' if 47<=frame<50 else 'INDEPENDENT RECOVERY / IDLE'
  d.text((20,395),label+' | battery illustrative',fill='#b0c7d8')
  if 22<=frame<25 or 47<=frame<50:
   x=116 if frame<25 else 284;d.ellipse((x-9,190,x+9,208),outline='#ffb45c',width=2)
  frames.append(im)
 frames[0].save(out/'waveshare-eyes.gif',save_all=True,append_images=frames[1:],duration=100,loop=0,optimize=True)
 frames[30].save(out/'waveshare-eyes.png')
 sources=[root/'tools/waveshare_frames.cpp',root/'firmware/WaveshareEyes/Round.h',root/'firmware/WaveshareEyes/Status.h',root/'firmware/WaveshareEyes/Metadata.h',root/'firmware/AnimatronicEyes/Companion.h']
 (out/'waveshare-animation-manifest.json').write_text(json.dumps({'kind':'source simulation, not hardware footage','battery_mv':'illustrative 4100','frames':270,'duration_ms':27000,'sources':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}},indent=2)+'\n')
print('Wrote labeled Waveshare source animation and source hashes')
