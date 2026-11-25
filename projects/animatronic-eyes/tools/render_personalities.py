#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Render autonomous Feather personalities with the actual firmware classes.
Requires tools/requirements-animation.txt; does not connect to hardware.
"""
import hashlib,io,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageSequence
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/images'
def font(size):return ImageFont.load_default(size=size)
def rgb565(n):return ((n>>11)*255//31,((n>>5)&63)*255//63,(n&31)*255//31)
def main():
 with tempfile.TemporaryDirectory(prefix='eye-personalities-') as tmp:
  exe=Path(tmp)/'frames';subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14',str(ROOT/'tools/personality_frames.cpp'),'-o',str(exe)],check=True);data=io.BytesIO(subprocess.check_output([str(exe)]))
 assert data.readline()==b'PERSONALITIES1 480 50\n'
 frames=[];palette=None;hashes=[set(),set(),set()];stats=[]
 names=['CURIOUS','DREAMY','CHEEKY'];shapes=['Taller, narrower eyes','Softer, shorter eyes','Slightly uneven eyes'];habits=['Quick double blinks','Slow, lingering blinks','Alternating solo winks']
 for frame in range(480):
  t=int(data.readline());assert t==frame*50
  canvas=Image.new('RGB',(960,476),'#0b1018');d=ImageDraw.Draw(canvas)
  d.text((22,15),'Three Feathers / three personalities',font=font(30),fill='#eef5ff')
  d.text((22,55),'Same larger pupils · independent glances and blinks · gamepad optional',font=font(16),fill='#aab9cc')
  stats=[]
  for i in range(3):
   ident,ink,left,right,looks,active=map(int,data.readline().split());assert ident==i+1
   raw=data.read(32400);assert len(raw)==32400;hashes[i].add(hashlib.sha256(raw).hexdigest())
   mask=Image.frombytes('L',(240,135),raw).resize((268,151),Image.Resampling.NEAREST)
   color=rgb565(ink);screen=Image.new('RGB',(268,151),'black');screen.paste(color,(0,0,268,151),mask)
   x=20+i*312;d.rounded_rectangle((x,91,x+296,420),radius=14,fill='#161e2b')
   d.text((x+14,106),names[i],font=font(24),fill=color)
   d.text((x+14,140),'Feather Eyes 1',font=font(14),fill='#aab9cc')
   d.rounded_rectangle((x+8,166,x+288,331),radius=10,fill='black',outline=color,width=2);canvas.paste(screen,(x+14,173))
   d.text((x+14,346),shapes[i],font=font(17),fill='#eef5ff')
   d.text((x+14,378),habits[i],font=font(14),fill='#aab9cc')
   stats.append(dict(name=names[i],left_blinks=left,right_blinks=right,looks=looks))
  d.text((22,439),'Autonomous firmware simulation · no gamepad input · status strips omitted',font=font(14),fill='#aab9cc')
  if palette is None:palette=canvas.quantize(colors=64,method=Image.Quantize.MEDIANCUT)
  frames.append(canvas.quantize(palette=palette,dither=Image.Dither.NONE))
 assert not data.read();assert all(len(h)>30 for h in hashes);assert stats[0]['looks']>stats[2]['looks']>stats[1]['looks'];assert all(r['left_blinks']>0 and r['right_blinks']>0 for r in stats)
 OUT.mkdir(parents=True,exist_ok=True);gif=OUT/'feather-personalities.gif'
 frames[0].save(gif,save_all=True,append_images=frames[1:],duration=50,loop=0,disposal=2,optimize=True,comment=b'Actual firmware simulation. No gamepads. Status strips omitted.')
 with Image.open(gif) as decoded:
  total=0;count=0
  for f in ImageSequence.Iterator(decoded):
   assert f.convert('RGB').tobytes()==frames[total//50].convert('RGB').tobytes()
   total+=f.info['duration'];count+=1
  assert total==24000 and decoded.info.get('loop')==0
 frames[80].save(OUT/'feather-personalities.png')
 sources=['firmware/AnimatronicEyes/Personality.h','firmware/AnimatronicEyes/Companion.h','firmware/NessoEyes/Idle.h','firmware/NessoEyes/TouchReaction.h','firmware/NessoEyes/Presentation.h','shared/opentoy/EyePixels.h','tools/personality_frames.cpp','tools/render_personalities.py']
 manifest=dict(simulation=True,autonomous=True,duration_ms=total,frames=count,distinct_frames=[len(h) for h in hashes],observations=stats,sha256=hashlib.sha256(gif.read_bytes()).hexdigest(),source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources})
 (OUT/'personalities-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({k:v for k,v in manifest.items() if k!='source_sha256'},indent=2))
if __name__=='__main__':main()
