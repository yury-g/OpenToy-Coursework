#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export a GIF from firmware geometry/animation; never connect to the toy.
Checks all 19 hardware-tested target hashes, then decodes the saved GIF and
checks its final OLED pixels for every pose. Captions distinguish simulation.
"""
import argparse,hashlib,io,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageSequence
ROOT=Path(__file__).resolve().parents[1]
def fnv(buffer):
 h=2166136261
 for b in buffer:h=((h^b)*16777619)&0xffffffff
 return f'{h:08x}'
def pixels(buffer):
 return bytes(255 if buffer[y//8*128+x]&(1<<(y%8)) else 0 for y in range(64) for x in range(128))
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=ROOT/'docs/images/v2-face-tour.gif');args=parser.parse_args()
 fixtures=json.loads((ROOT/'projects/xiao-class/tests/doodle_frames.json').read_text());tour=fixtures+[dict(fixtures[0],label='Neutral / return to rest')]
 with tempfile.TemporaryDirectory(prefix='opentoy-animation-') as scratch:
  scratch=Path(scratch);commands=scratch/'poses.txt';commands.write_text('\n'.join(p['command'] for p in tour)+'\n');exe=scratch/'animation'
  subprocess.run(shlex.split(os.environ.get('CXX','c++'))+[str(ROOT/'tools/animation_dump.cpp'),'-o',str(exe)],check=True)
  data=io.BytesIO(subprocess.check_output([str(exe),str(commands)]))
 header=data.readline().decode().split();assert header[0]=='OPENTOYANIM1';count,frames,dt=map(int,header[1:]);assert count==len(tour)
 title=ImageFont.load_default(size=25);label=ImageFont.load_default(size=20);small=ImageFont.load_default(size=14)
 palette=[c for v in range(256) for c in (v,v,v)];images=[];records=[];hashes=set()
 for i in range(count):
  for j in range(frames):
   pose,t,settled,digest=data.readline().decode().split();buffer=data.read(1024);assert len(buffer)==1024 and int(pose)==i and fnv(buffer)==digest
   hashes.add(digest);oled=Image.frombytes('L',(128,64),pixels(buffer)).resize((512,256),Image.Resampling.NEAREST)
   canvas=Image.new('L',(640,384));canvas.paste(oled,(64,78));draw=ImageDraw.Draw(canvas)
   draw.text((20,12),'OpenToy / Face v2',font=title,fill=255);draw.text((20,46),tour[i]['label'],font=label,fill=235)
   draw.text((550,20),f'{i+1} / {count}',font=small,fill=200)
   draw.text((20,351),'Simulated OLED / firmware renderer / 128 x 64',font=small,fill=190)
   frame=canvas.convert('P');frame.putpalette(palette);images.append(frame)
   if j==frames-1:
    assert settled=='1' and digest==tour[i]['hash'],f'Final target mismatch: {tour[i]["command"]}'
    records.append(dict(tour[i],start_ms=i*frames*dt,end_ms=(i+1)*frames*dt,settled=True))
 assert not data.read();args.output.parent.mkdir(parents=True,exist_ok=True)
 images[0].save(args.output,save_all=True,append_images=images[1:],duration=dt,loop=0,disposal=2,optimize=False,comment=b'OpenToy Face v2. Firmware-rendered simulation, not physical OLED video.')
 # Verify pixels after GIF encoding, including disposal and merged static frames.
 with Image.open(args.output) as gif:
  decoded=[];end=0
  for frame in ImageSequence.Iterator(gif):
   duration=frame.info['duration'];end+=duration;decoded.append((end,frame.convert('L').copy()))
  assert end==count*frames*dt and gif.info.get('loop')==0
  for r in records:
   image=next(image for end,image in decoded if end>r['end_ms']-dt)
   image=image.crop((64,78,576,334)).resize((128,64),Image.Resampling.NEAREST)
   buffer=bytearray(1024)
   for y in range(64):
    for x in range(128):
     v=image.getpixel((x,y));assert v in (0,255)
     if v:buffer[y//8*128+x]|=1<<(y%8)
   assert fnv(buffer)==r['hash'],f'Encoded GIF target differs: {r["label"]}'
 manifest=dict(format='OpenToy Face v2 animation v1',simulation=True,width=640,height=384,oled_width=128,oled_height=64,generated_frames=len(images),gif_frames=len(decoded),duration_ms=end,distinct_oled_frames=len(hashes),frame_ms=dt,renderer_sha256=hashlib.sha256((ROOT/'projects/xiao-class/firmware/ToyBLE/DoodlePixels.h').read_bytes()).hexdigest(),gif_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),poses=records)
 args.output.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print(f'Saved {args.output}: {len(decoded)} encoded frames, {end/1000:.1f}s, {len(hashes)} distinct OLED frames. All {len(records)} final pose hashes match after decoding.')
if __name__=='__main__':main()
