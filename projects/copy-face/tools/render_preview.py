#!/usr/bin/env python3
"""Actual C++ face renderer, scripted input; annotated simulated UI, not device video."""
import hashlib,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];out=root/'images';out.mkdir(exist_ok=True)
eyes=root.parent/'animatronic-eyes'
env=dict(os.environ)
if Path('/Library/Developer/CommandLineTools').exists():env.setdefault('DEVELOPER_DIR','/Library/Developer/CommandLineTools')
includes=[eyes/'firmware/AnimatronicEyes',eyes/'firmware/NessoEyes',eyes/'shared/opentoy',root/'firmware/reference']
metadata=json.loads((root/'firmware/metadata.json').read_text())
fonts=[Path('/System/Library/Fonts/Supplemental/Arial.ttf'),Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')]
font=next((ImageFont.truetype(str(p),24) for p in fonts if p.exists()),ImageFont.load_default())
with tempfile.TemporaryDirectory(prefix='copy-face-preview-') as tmp:
 exe=Path(tmp)/'preview'
 subprocess.run(shlex.split(env.get('CXX','c++'))+['-O2','-std=c++14']+['-I'+str(p) for p in includes]+[str(root/'tools/preview.cpp'),'-o',str(exe)],env=env,check=True)
 status_path=Path(tmp)/'status.txt'
 process=subprocess.Popen([str(exe),str(status_path)],stdout=subprocess.PIPE);frames=[]
 for i in range(120):
  data=process.stdout.read(1024*600*3)
  if len(data)!=1024*600*3:raise RuntimeError('Incomplete renderer output')
  im=Image.frombytes('RGB',(1024,600),data);d=ImageDraw.Draw(im)
  d.rectangle((0,0,1023,47),fill='#151d22');d.rectangle((0,544,1023,599),fill='#151d22')
  d.text((16,10),f"COPY-FACE v{metadata['firmware_version']}  {metadata['version_created']}",font=font,fill='white')
  d.text((900,10),'--',font=font,fill='white');d.rectangle((950,14,993,33),outline='white',width=2);d.rectangle((994,20,998,27),fill='white')
  d.rectangle((720,0,863,47),fill='#254d40');d.text((754,10),'Menu',font=font,fill='white')
  d.text((16,558),'SIMULATION / NO CAMERA  |  Mouth follows selected lip contours',font=font,fill='#ffd577')
  frames.append(im)
 if process.wait()!=0:raise RuntimeError('Renderer failed')
 small=next((ImageFont.truetype(str(p),20) for p in fonts if p.exists()),font)
 statuses=status_path.read_text().splitlines()
 assert len(statuses)==len(frames)
 for im,state in zip(frames,statuses):
  name,left,right,tracked=state.split('|');d=ImageDraw.Draw(im)
  d.text((16,60),'Camera preview (simulated)',font=small,fill='white')
  d.text((16,292),'Shape guess: '+name,font=small,fill='white')
  d.text((16,326),f'Eyes L / R: {left} / {right}',font=small,fill='white')
  d.text((16,368),'Primary face: tracked' if tracked=='1' else 'No current tracked face',font=small,fill='white')
  d.multiline_text((16,404),'Green edges: eye / lip outlines\nGreen dots: estimated contact\n\nLandmarks are scripted.\nNot a live camera feed.',font=small,fill='#a7c2ae',spacing=4)
 frames[10].save(out/'copy-face.png')
 frames[30].save(out/'copy-face-frown.png')
 frames[50].save(out/'copy-face-neutral.png')
 frames[70].save(out/'copy-face-open.png')
 frames[0].save(out/'copy-face.gif',save_all=True,append_images=frames[1:],duration=100,loop=0,optimize=True)
 sources=[root/'core/Tracking.h',root/'core/Geometry.h',root/'core/DeveloperView.h',root/'core/Strokes.h',root/'core/Scene.h',root/'firmware/reference/Smooth.h',root/'tools/preview.cpp']
 record={'kind':'C++ native face renderer with synthetic observations; header/footer annotated in Python; not camera or device video, no speed evidence',
         'frames':120,'step_ms':100,'metadata':metadata,'source_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
 (out/'preview-manifest.json').write_text(json.dumps(record,indent=2)+'\n')
 print(out/'copy-face.png')
