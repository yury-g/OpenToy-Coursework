from PIL import Image,ImageDraw,ImageFont
import numpy as np
from pathlib import Path
import argparse
p=argparse.ArgumentParser();p.add_argument('raw_frames',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1]/'previews'
a=np.fromfile(a.raw_frames,dtype='<u2').reshape(-1,280,240)
frames=[]
for f in a:
 rgb=np.stack([((f>>11)&31)*255//31,((f>>5)&63)*255//63,(f&31)*255//31],axis=-1).astype('uint8')
 frames.append(Image.fromarray(rgb))
frames[0].save(r/'curious-touch.gif',save_all=True,append_images=frames[1:],duration=50,loop=0,optimize=True)
canvas=Image.new('RGB',(1120,440),'#10191e');d=ImageDraw.Draw(canvas)
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
for i,(idx,label) in enumerate([(50,'Curious'),(70,'Poked / annoyed'),(150,'Swipe down: hardware'),(190,'Swipe up: software')]):
 x=20+i*280;d.rounded_rectangle((x,57,x+256,353),18,fill='#253b46');canvas.paste(frames[idx],(x+8,65));d.text((x,20),label,font=font,fill='#ecf5f8')
d.text((20,384),'OpenToy / Waveshare 1.69   ·   Firmware simulation — not a device recording',font=small,fill='#b0c1c9')
canvas.save(r/'screens.png')
