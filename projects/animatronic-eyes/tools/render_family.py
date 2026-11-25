#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Rebuild family and Nesso GIFs from real firmware behavior with scripted inputs.
Requires the repository's tools/requirements-animation.txt. No hardware access.
"""
import hashlib,io,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageSequence
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/images'
BG='#0b1018';CARD='#161e2b';TEXT='#eef5ff';MUTED='#aab9cc';PINK='#ff81aa';CYAN='#54dbea';ORANGE='#ffba71'
font=lambda size:ImageFont.load_default(size=size)
def text(d,xy,label,size=16,fill=TEXT):d.text(xy,label,font=font(size),fill=fill)
def decode(mono):return Image.frombytes('L',(128,64),bytes(255 if mono[y//8*128+x]&(1<<(y%8)) else 0 for y in range(64) for x in range(128)))
def face_image(img,size):
 result=Image.new('RGB',size,'black');scale=min(size[0]/img.width,size[1]/img.height);img=img.convert('RGB').resize((round(img.width*scale),round(img.height*scale)),Image.Resampling.NEAREST);result.paste(img,((size[0]-img.width)//2,(size[1]-img.height)//2));return result
controls=['Stick / pupils or eye curves','A / happy','B / angry','X / surprised','Start + stick / lids and jaw','Select + stick / eyes or mouth','Select + Start / blink','Y / neutral or sorry']
def phase(t,poke):
 if poke:return ('Left' if poke==1 else 'Right')+' poke: closes, gets angrier'
 if 9000<=t<13500:return 'Smooth turn through every angle'
 if 6450<=t<9000:return 'Left recovers slowly; right relaxes'
 if 14950<=t<22500:return 'Right recovers on its own timing'
 return 'Idle: looking around + separate blinks'
def save(frames,path):
 frames[0].save(path,save_all=True,append_images=frames[1:],duration=50,loop=0,disposal=2,optimize=True,comment=b'Firmware-rendered simulation; scripted inputs; illustrative frames, not hardware video. Status strip omitted.')
 with Image.open(path) as gif:
  count=0;duration=0
  for f in ImageSequence.Iterator(gif):count+=1;duration+=f.info['duration'];assert f.size==frames[0].size
  assert duration==24000 and gif.info.get('loop')==0
 return dict(frames=count,duration_ms=duration,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory(prefix='eyes-family-') as tmp:
  exe=Path(tmp)/'frames';subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14',str(ROOT/'tools/family_frames.cpp'),'-o',str(exe)],check=True)
  data=io.BytesIO(subprocess.check_output([str(exe)]))
 assert data.readline()==b'FAMILY2 480 50\n'
 family=[];detail=[];hashes=set();blink=[False,False];asym=False
 # One shared palette avoids color flicker across GIF frames.
 palette=Image.new('RGB',(8,1));palette.putdata([tuple(bytes.fromhex(c[1:])) for c in [BG,CARD,TEXT,MUTED,PINK,CYAN,ORANGE,'#000000']]);palette=palette.quantize(colors=32)
 for _ in range(480):
  values=data.readline().decode().split();t,poke=int(values[0]),int(values[1]);angle=float(values[2]);ml,mr=int(values[3]),int(values[4]);l,r,part=map(int,values[5:8]);tx,ty=map(int,values[8:]);blink[0]|=l==100;blink[1]|=r==100;asym|=ml!=mr
  raw=data.read(32400);assert len(raw)==32400;hashes.add(hashlib.sha256(raw).digest());n=Image.frombytes('L',(240,135),raw)
  f1=Image.frombytes('L',(240,135),data.read(32400));f2=decode(data.read(1024))
  canvas=Image.new('RGB',(960,478),BG);d=ImageDraw.Draw(canvas)
  text(d,(22,14),'Animatronic Eyes / device family',30);text(d,(22,53),'OpenToy · three playable versions',17,MUTED)
  specs=[('Nesso N1','EYES 1 · touch + motion',PINK,n),('Reverse TFT Feather','EYES 1 · idle + QT gamepad',CYAN,f1),('Reverse TFT Feather','EYES 2 · QT gamepad',ORANGE,f2)]
  for i,(name,subtitle,color,img) in enumerate(specs):
   x=20+312*i;d.rounded_rectangle((x,88,x+296,419),radius=14,fill=CARD)
   text(d,(x+14,103),name,21);text(d,(x+14,133),subtitle,15,color)
   d.rounded_rectangle((x+8,163,x+288,328),radius=10,outline=color,width=2,fill='#000000');canvas.paste(face_image(img,(268,151)),(x+14,170))
   if i==0:
    text(d,(x+14,341),'Autonomous blink + gaze',17,color)
    text(d,(x+14,371),'Poke · anger · recovery · rotation',13,MUTED)
    if poke:
     cx=x+14+tx*268/240;cy=170+ty*151/135;d.ellipse((cx-10,cy-10,cx+10,cy+10),outline=PINK,width=3)
   elif i==1:
    text(d,(x+14,341),'Select + X/Y / poke' if poke else 'Stick / pupils' if 9000<=t<13500 else 'Autonomous blink + gaze',17,color)
    text(d,(x+14,371),'Gamepad takes over immediately',13,MUTED)
   else:
    text(d,(x+14,341),controls[part].split(' / ')[0],17,color)
    text(d,(x+14,371),'Scripted gamepad input',14,MUTED)
  text(d,(22,436),'Firmware simulations · status strips omitted · Eyes 1 idles on Nesso and Feather',14,MUTED)
  family.append(canvas.quantize(palette=palette,dither=Image.Dither.NONE))
  hero=Image.new('RGB',(740,468),BG);hd=ImageDraw.Draw(hero)
  text(hd,(22,14),'Nesso N1 / Animatronic Eyes 1',28);text(hd,(22,54),phase(t,poke),18,PINK)
  hd.rounded_rectangle((18,94,510,376),radius=14,outline=PINK,width=2,fill='#000000');hero.paste(n.convert('RGB').resize((480,270),Image.Resampling.NEAREST),(24,100))
  if poke:
   cx=24+tx*2;cy=100+ty*2;hd.ellipse((cx-15,cy-15,cx+15,cy+15),outline=PINK,width=4)
  for y,label in [(112,'Larger eyes'),(147,'Smooth turns'),(182,'Separate moods'),(217,'Unequal timing'),(252,'Idle glances'),(287,'Staggered blinks')]:text(hd,(532,y),label,16,TEXT)
  text(hd,(532,334),f'{angle*180/3.14159265:+.0f}° turn',17,PINK)
  text(hd,(22,395),'Touch pauses the idle animation; it resumes after release.',17)
  text(hd,(22,434),'Firmware-rendered simulation · scripted pokes / motion · status strip omitted',13,MUTED)
  detail.append(hero.quantize(palette=palette,dither=Image.Dither.NONE))
 assert not data.read() and all(blink) and asym and len(hashes)>200
 manifest=dict(simulation=True,scripted_inputs=True,status_strip_omitted=True,distinct_nesso_frames=len(hashes),both_full_blinks=blink,unequal_moods=asym)
 for name,frames in [('animatronic-family.gif',family),('nesso-eyes-behavior.gif',detail)]:manifest[name]=save(frames,OUT/name)
 sources=['firmware/NessoEyes/Idle.h','firmware/NessoEyes/TouchReaction.h','firmware/NessoEyes/Presentation.h','shared/opentoy/EyePixels.h','shared/opentoy/DoodlePixels.h','firmware/AnimatronicEyes/Controls.h','firmware/AnimatronicEyes/Companion.h','firmware/AnimatronicEyes/Personality.h','tools/family_frames.cpp','tools/render_family.py']
 manifest['source_sha256']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources}
 (OUT/'animation-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 family[80].save(OUT/'animatronic-family.png');detail[127].save(OUT/'nesso-eyes-poke.png')
 print(json.dumps({k:v for k,v in manifest.items() if k!='source_sha256'},indent=2))
if __name__=='__main__':main()
