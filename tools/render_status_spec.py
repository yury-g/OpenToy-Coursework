#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Render illustrative status-bar layouts; requires Pillow 12.x."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
out=Path(__file__).resolve().parents[1]/'docs/images/app-status-bar-spec.png'
def font(n):return ImageFont.load_default(size=n)
def bar(im,y,h,small=False,roundscreen=False):
 d=ImageDraw.Draw(im);w=im.width;x=70 if roundscreen else 0;right=w-70 if roundscreen else w
 d.rectangle((x,y,right-1,y+h-1),fill='#233a4b')
 size=12 if roundscreen else 8;f=font(size)
 line='EYES1 v1.3.0' if small else 'EYES1 v1.3.0 2026-11-16'
 d.text((x+2,y),line,font=f,fill='white')
 if small:d.text((x+2,y+10),'2026-11-16',font=f,fill='#d2e1eb')
 d.text((right-43,y),'82%',font=f,fill='#c6f5ce')
 d.rectangle((right-21,y+2,right-4,y+8),outline='#c6f5ce');d.rectangle((right-3,y+4,right-2,y+6),fill='#c6f5ce');d.rectangle((right-19,y+4,right-8,y+6),fill='#7bd78c')
 assert d.textbbox((x+2,y),line,font=f)[2]<right-43
 return im
def face(im,top,roundscreen=False):
 d=ImageDraw.Draw(im);w,h=im.size;cy=(h+top)/2;ew=w*.22;eh=min((h-top)*.58,w*.26)
 for cx in (w*.28,w*.72):
  d.rounded_rectangle((cx-ew/2,cy-eh/2,cx+ew/2,cy+eh/2),radius=max(4,ew*.28),fill='#e4fbff');r=ew*.28;d.ellipse((cx-r,cy-r,cx+r,cy+r),fill='#070e13')
 return im
canvas=Image.new('RGB',(1140,430),'#0c131c');d=ImageDraw.Draw(canvas)
d.text((22,14),'OpenToy app status bar',font=font(27),fill='white')
d.text((22,48),'Design examples · default visible · hide for performance · not installed-device screenshots',font=font(15),fill='#b3c4d6')
a=face(Image.new('RGB',(240,135),'#070e13'),10);bar(a,0,10);canvas.paste(a.resize((480,270),Image.Resampling.NEAREST),(22,95));d.text((22,77),'240 x 135 · single 10 px row',font=font(14),fill='#c9d9e8')
b=face(Image.new('RGB',(128,64),'#070e13'),20);bar(b,0,20,small=True);canvas.paste(b.resize((256,128),Image.Resampling.NEAREST),(530,95));d.text((530,77),'128 x 64 · readable two-row fallback',font=font(13),fill='#c9d9e8')
c=face(Image.new('RGB',(128,64),'#070e13'),0);canvas.paste(c.resize((256,128),Image.Resampling.NEAREST),(530,253));d.text((530,233),'Performance mode · header hidden',font=font(13),fill='#c9d9e8')
r=Image.new('RGB',(360,360),'#070e13');face(r,54,True);bar(r,40,16,roundscreen=True);mask=Image.new('L',(360,360));ImageDraw.Draw(mask).ellipse((0,0,359,359),fill=255);r.thumbnail((288,288));mask=mask.resize((288,288),Image.Resampling.LANCZOS);canvas.paste(r,(818,95),mask);d.text((818,77),'Round screen · inset safe header',font=font(14),fill='#c9d9e8')
d.text((22,393),'App + installed version + version creation date                                        Local battery stays at the right',font=font(16),fill='#b3c4d6');canvas.save(out)
print(out)
