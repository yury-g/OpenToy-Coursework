#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate the cross-platform README demo from the canonical host solver.
Needs Pillow and C++14. CXX, FONT_REGULAR and FONT_BOLD can override local defaults.
The two panels illustrate matching behaviors, not connected multiplayer.
"""
from pathlib import Path
import os, shlex, subprocess, tempfile
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'images'
BG='#080e19';PANEL='#121e30';WHITE='#f1f5ff';MUTED='#9eaec4';BLUE='#66caff';GOLD='#ffcc5a';MINT='#98e6df'
REG=os.environ.get('FONT_REGULAR','/System/Library/Fonts/Supplemental/Arial.ttf')
BOLD=os.environ.get('FONT_BOLD','/System/Library/Fonts/Supplemental/Arial Bold.ttf')
def font(n,bold=False):return ImageFont.truetype(BOLD if bold else REG,n)
def text(d,xy,value,n=18,color=WHITE,bold=False):d.text(xy,value,font=font(n,bold),fill=color)
def pad(d,x,y,active=False):
 d.rounded_rectangle((x,y,x+185,y+54),radius=14,fill='#24364b',outline='#456078',width=2)
 d.ellipse((x+13,y+9,x+49,y+45),fill='#080e19',outline=MINT,width=2)
 dx=7 if active else 0
 d.ellipse((x+24+dx,y+20,x+38+dx,y+34),fill=MINT)
 for label,bx,by in [('X',x+140,y+12),('Y',x+120,y+27),('A',x+160,y+27),('B',x+140,y+42)]:
  d.ellipse((bx-9,by-9,bx+9,by+9),fill=GOLD if active and label=='A' else '#365069')
  text(d,(bx-4,by-6),label,9,BG if active and label=='A' else WHITE,True)
phases=[('REST','Particles settle when left still.','Leave it still.','Leave it still.'),
 ('BOIL','Holding the button adds heat.','Hold the yellow button.','Hold B or Feather D1.'),
 ('COOL','Releasing the button removes heat.','Release the button.','Release B / D1.'),
 ('PUSH','Touch or cursor pushes particles away.','Touch and drag a finger.','Move the stick. A holds touch.'),
 ('SCATTER','Shaking scatters the particles.','Shake the Nesso.','Shake the sensor or tap X / D2.'),
 ('TILT','Particles move with the tilt.','Tilt the Nesso.','Tilt the fixed Feather + IMU.'),
 ('SETTLE','Motion stops after settling.','Leave it still.','Leave it still.')]
frames=[];snapshots={}
with tempfile.TemporaryDirectory(prefix='gravity-family-art-') as tmp:
 exe=Path(tmp)/'showcase'
 subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14','-O2',str(ROOT/'tools/showcase.cpp'),'-o',str(exe)],check=True)
 proc=subprocess.Popen([str(exe)],stdout=subprocess.PIPE)
 while line:=proc.stdout.readline():
  tick,phase,tx,ty=map(int,line.split());data=proc.stdout.read(135*227*3)
  if len(data)!=135*227*3:raise RuntimeError('Incomplete model frame')
  scene=Image.frombytes('RGB',(135,227),data)
  name,title,nesso,feather=phases[phase]
  color=GOLD if phase in (1,4) else MINT if phase==3 else BLUE
  im=Image.new('RGB',(960,660),BG);d=ImageDraw.Draw(im)
  text(d,(30,22),'GRAVITY WATER  /  NESSO + FEATHER',12,BLUE,True)
  text(d,(28,49),'Gravity Water on Nesso and Feather',38,WHITE,True)
  text(d,(30,99),'400 particles controlled by tilt, buttons, touch or gamepad.',18,MUTED)
  for k,(x,label,subtitle,control) in enumerate([(28,'ARDUINO NESSO N1','All-in-one touch + motion + sound',nesso),(493,'ADAFRUIT FEATHER','Reverse TFT + IMU + optional gamepad',feather)]):
   d.rounded_rectangle((x,143,x+439,534),radius=21,fill=PANEL,outline='#2c415b',width=1)
   text(d,(x+21,161),label,19,WHITE,True)
   text(d,(x+21,188),subtitle,13,MUTED)
   # Abstract display frame, not a hardware product photograph.
   d.rounded_rectangle((x+20,222,x+167,461),radius=12,fill='#273c52',outline='#54758d',width=2)
   im.paste(scene,(x+26,228));d=ImageDraw.Draw(im)
   if phase==3:
    cx=x+26+tx;cy=228+ty
    d.ellipse((cx-8,cy-8,cx+8,cy+8),outline=WHITE if k==0 else GOLD,width=2)
    d.ellipse((cx-2,cy-2,cx+2,cy+2),fill=WHITE if k==0 else GOLD)
   text(d,(x+193,238),'CONTROLS',11,MUTED,True)
   if k==0:
    text(d,(x+193,266),'Touch the screen.',20,MINT,True)
    text(d,(x+193,300),'Hold to heat.',19,WHITE)
    d.rounded_rectangle((x+193,341,x+313,372),radius=9,fill=GOLD)
    text(d,(x+211,348),'YELLOW BUTTON',10,BG,True)
    text(d,(x+193,403),'Generated sound',15,MUTED)
    text(d,(x+193,425),'built into the toy.',15,MUTED)
   else:
    text(d,(x+193,266),'Move the cursor.',20,MINT,True)
    text(d,(x+193,300),'Stick moves. A pushes.',16,WHITE)
    pad(d,x+193,338,phase==3)
    text(d,(x+193,414),'B: boil   X: scatter',15,MUTED)
    text(d,(x+193,436),'Y: recenter',15,MUTED)
   d.line((x+21,476,x+418,476),fill='#2c415b')
   text(d,(x+21,492),control,17,color,True)
  d.rounded_rectangle((28,554,146,586),radius=16,fill='#20314a')
  text(d,(47,562),name,12,color,True);text(d,(166,556),title,24,WHITE,True)
  text(d,(30,608),'SCRIPTED MODEL DEMO  /  Independent devices, matching behavior. Not footage or multiplayer.',12,MUTED)
  text(d,(30,630),'Feather tilt direction depends on the fixed sensor mount; final mounting check is still pending.',12,MUTED)
  frames.append(im)
  if tick in (80,260,516,700):snapshots[phase]=im.copy()
 if proc.wait()!=0:raise RuntimeError('Model renderer failed')
OUT.mkdir(exist_ok=True)
sheet=Image.new('RGB',(960*2,660*2))
for i,phase in enumerate((0,1,3,4)):sheet.paste(snapshots[phase],((i%2)*960,(i//2)*660))
palette=sheet.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
quantized=[im.quantize(palette=palette,dither=Image.Dither.NONE) for im in frames]
quantized[0].save(OUT/'cross-platform-play.gif',save_all=True,append_images=quantized[1:],duration=[70,60,70]*105,loop=0,optimize=True,disposal=1)
snapshots[3].save(OUT/'cross-platform-poster.png')
# Static, readable control comparison for people skimming the README.
im=Image.new('RGB',(960,820),BG);d=ImageDraw.Draw(im)
text(d,(30,26),'Controls by device',38,WHITE,True)
text(d,(32,81),'Nesso touchscreen and Feather gamepad controls compared.',19,MUTED)
text(d,(286,137),'NESSO N1',18,BLUE,True);text(d,(594,137),'FEATHER + QT GAMEPAD',18,MINT,True)
rows=[('PUSH THE BEADS','Touch and drag','Move stick; hold A in place'),('HEAT','Hold yellow button','Hold B or onboard D1'),('COOL','Release the button','Release B / D1'),('SCATTER','Shake the device','Shake sensor; tap X or D2'),('TILT','Tilt the device','Tilt the fixed device + IMU'),('RECENTER CURSOR','Your fingertip is the cursor','Press Y'),('SOUND TEXTURE','Built-in generated audio','No speaker in this setup')]
for i,(action,a,b) in enumerate(rows):
 y=175+i*63
 d.rounded_rectangle((28,y,932,y+55),radius=10,fill=PANEL if i%2==0 else '#0d1725')
 text(d,(44,y+19),action,12,GOLD if i in (1,2) else MUTED,True)
 text(d,(286,y+17),a,16,WHITE);text(d,(594,y+17),b,16,WHITE)
d.rounded_rectangle((28,642,932,747),radius=16,fill='#17283a',outline='#32516a')
text(d,(48,660),'ONE SHARED PARTICLE ENGINE',13,MINT,True)
text(d,(48,688),'400 beads   /   energy colors   /   quick cooling   /   quiet rest',22,WHITE,True)
text(d,(48,723),'Nesso: built-in sensors. Feather: external LSM6DS3TR-C; gamepad is optional.',14,MUTED)
text(d,(30,773),'Both editions have been played on hardware. Feather IMU mounting direction is still to be checked.',13,MUTED)
text(d,(30,796),'Each device plays independently. The matching visuals do not indicate a shared or networked session.',12,MUTED)
im.save(OUT/'cross-platform-controls.png')
print('Created cross-platform animation, poster and control map.')
