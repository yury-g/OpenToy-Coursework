#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Reproduce the development overview and exact v1 gallery, without hardware."""
import hashlib,io,json,os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageOps,ImageSequence
ROOT=Path(__file__).resolve().parents[1]
IMAGES=ROOT/'docs/images'
def fnv(buffer):
 h=2166136261
 for b in buffer:h=((h^b)*16777619)&0xffffffff
 return f'{h:08x}'
def draw_frame(exe,fixture):
 # PBM defines 1 as black; this exporter emits 1 for a lit OLED pixel.
 im=ImageOps.invert(Image.open(io.BytesIO(subprocess.check_output([str(exe),fixture['command']]))).convert('L'))
 assert im.size==(128,64)
 b=bytearray(1024)
 for y in range(64):
  for x in range(128):
   if im.getpixel((x,y)):b[y//8*128+x]|=1<<(y%8)
 assert fnv(b)==fixture['hash'],fixture['command']
 return im.convert('RGB')
def main():
 title=ImageFont.load_default(size=40);head=ImageFont.load_default(size=24);body=ImageFont.load_default(size=18);small=ImageFont.load_default(size=15)
 bg='#f5f3ee';ink='#172a37';muted='#4e606b'
 with tempfile.TemporaryDirectory(prefix='opentoy-history-') as scratch:
  exe=Path(scratch)/'frame'
  subprocess.run(shlex.split(os.environ.get('CXX','c++'))+[str(ROOT/'tools/frame_dump.cpp'),'-o',str(exe)],check=True)
  eye=json.loads((ROOT/'projects/xiao-class/tests/eye_frames.json').read_text())
  indices=[0,1,4,7,10,13];labels=['Neutral / centered pupils','Left stick / whites up','D-pad / pupils right','Right stick / left wink','Right stick down / top lids','B / angry preset']
  frames=[draw_frame(exe,eye[i]) for i in indices]
  gallery=Image.new('RGB',(1160,1138),bg);d=ImageDraw.Draw(gallery)
  d.text((28,18),'OpenToy v1 / eye puppetry',font=title,fill=ink)
  d.text((28,68),'Exact firmware pixel simulations / not physical OLED photographs',font=body,fill=muted)
  for i,(frame,label,idx) in enumerate(zip(frames,labels,indices)):
   x=28+(i%2)*570;y=115+(i//2)*332
   d.text((x,y),label,font=head,fill=ink)
   gallery.paste(frame.resize((512,256),Image.Resampling.NEAREST),(x,y+34))
   d.text((x,y+297),f'128 x 64 / reference hash {eye[idx]["hash"]}',font=small,fill=muted)
  gallery.save(IMAGES/'v1-eyes.png',optimize=True)
  manifest=json.loads((IMAGES/'v2-face-tour.json').read_text())
  assert manifest['gif_sha256']==hashlib.sha256((IMAGES/'v2-face-tour.gif').read_bytes()).hexdigest()
  target=manifest['poses'][1]['end_ms']-25;elapsed=0;happy=None
  with Image.open(IMAGES/'v2-face-tour.gif') as gif:
   for f in ImageSequence.Iterator(gif):
    elapsed+=f.info['duration']
    if elapsed>target:happy=f.convert('RGB').crop((64,78,576,334));break
  assert happy is not None
  panel=Image.new('RGB',(1536,1400),bg);d=ImageDraw.Draw(panel)
  d.text((32,23),'OpenToy / visual development history',font=title,fill=ink)
  d.text((32,77),'October 1-2, 2026 / drawings, firmware simulations and actual app screenshots',font=body,fill=muted)
  cards=[
   ('01  USB + Bluetooth','electronic tests',None,'Identify / preserve / upload / verify','Two ESP32-S3 boards; serial and BLE checks'),
   ('02  Eye puppetry / v1','exact firmware simulation',frames[0],'Whites + pupils + independent eyelids','v1.0.0-eyes / owner: Works great'),
   ('03  Draw before coding','generated concept drawing',Image.open(IMAGES/'v2-face-concepts-01.png'),'A: soft curves / B: playful doodles','B selected / five expressions'),
   ('04  Try two approaches','browser design experiment',Image.open(IMAGES/'v2-local-comparison.jpg'),'Procedural curves / preset morph','Only procedural mode installed'),
   ('05  Face v2 / acceptance','exact firmware simulation',happy,'Curved eyes + brows + mouth + wiggle','v2.0.0-face / BLE + independent USB checked'),
   ('06  Save the stopping point','actual Mac app screenshot',Image.open(IMAGES/'v2-mac-app.jpg'),'Private source + 38-second GIF + recovery','checkpoint-2026-10-01-face-v2'),
  ]
  for i,(label,kind,image,caption,reference) in enumerate(cards):
   x=32+(i%2)*752;y=130+(i//2)*410
   d.rounded_rectangle((x,y,x+720,y+386),radius=14,fill='white',outline='#d9ddd9',width=2)
   d.text((x+18,y+15),label,font=head,fill=ink)
   box=(x+18,y+54,x+702,y+294)
   if image is not None:
    resample=Image.Resampling.NEAREST if 'firmware' in kind else Image.Resampling.LANCZOS
    thumb=ImageOps.contain(image.convert('RGB'),(684,240),method=resample)
    panel.paste(thumb,(box[0]+(684-thumb.width)//2,box[1]+(240-thumb.height)//2))
   else:
    d.rounded_rectangle(box,radius=8,fill='#172a37')
    for text,dy,font,color in [('USB -> XIAO -> serial',34,head,'white'),('Mac -> Bluetooth -> toy',86,head,'white'),('Original flash preserved / checked updates',147,body,'#d8e2db'),('Diagram of verified work / no hardware photo',188,small,'#b4cbbd')]:
     d.text((x+42,y+54+dy),text,font=font,fill=color)
   d.text((x+18,y+307),caption,font=body,fill=ink)
   d.text((x+18,y+334),reference,font=small,fill=muted)
   d.text((x+18,y+360),kind,font=small,fill=muted)
  d.text((32,1363),'Next requested: power-only controller -> toy / investigation, not yet implemented',font=body,fill=muted)
  panel.save(IMAGES/'history-overview.png',optimize=True)
  records=dict(format='OpenToy visual history export v1',simulation_notice='Exact firmware buffers and preserved concept/browser/app visuals; no OLED photographs',
   v1_reference_hashes=[eye[i]['hash'] for i in indices],
   source_images={name:hashlib.sha256((IMAGES/name).read_bytes()).hexdigest() for name in ['v2-face-concepts-01.png','v2-local-comparison.jpg','v2-mac-app.jpg','v2-face-tour.gif']},
   generated={name:hashlib.sha256((IMAGES/name).read_bytes()).hexdigest() for name in ['v1-eyes.png','history-overview.png']})
  (IMAGES/'history-overview.json').write_text(json.dumps(records,indent=2)+'\n')
  print('Saved history overview and 6 exact v1 frame/hash-checked PNG examples.')
if __name__=='__main__':main()
