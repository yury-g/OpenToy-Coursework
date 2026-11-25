#!/usr/bin/env python3
"""Create the README artwork from the actual host particle model. Needs Pillow + C++14.
Run from any directory. Build intermediates live in a temporary folder, outside Git.
CXX may contain compiler flags; FONT_REGULAR / FONT_BOLD may override local fonts.
"""
from pathlib import Path
import os, shlex, subprocess, tempfile
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'images'
REG=os.environ.get('FONT_REGULAR','/System/Library/Fonts/Supplemental/Arial.ttf')
BOLD=os.environ.get('FONT_BOLD','/System/Library/Fonts/Supplemental/Arial Bold.ttf')
def font(n,bold=False):return ImageFont.truetype(BOLD if bold else REG,n)
BG='#080e19';WHITE='#f1f5ff';MUTED='#9eaec4';BLUE='#66caff';GOLD='#ffcc5a'
labels=['A little world, at rest.','Hold to turn up the heat.','Let go. Watch it cool.','Push a path with your finger.','Give it a snow-globe shake.','Tilt it. Gravity does the rest.','Let the world settle.']
subs=['400 individual beads. Ready for your next move.','The yellow button brings the boil.','Warm colors fade back toward blue.','Nearby beads move away from your touch.','A quick burst scatters the particles.','The beads follow the lower physical edge.','Motion quiets down. Sound fades away.']
frames=[];examples={}
with tempfile.TemporaryDirectory(prefix='gravity-water-art-') as tmp:
 exe=Path(tmp)/'showcase'
 subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14','-O2',str(ROOT/'tools/showcase.cpp'),'-o',str(exe)],check=True)
 proc=subprocess.Popen([str(exe)],stdout=subprocess.PIPE)
 while line:=proc.stdout.readline():
  t,phase,tx,ty=map(int,line.split());data=proc.stdout.read(135*227*3)
  assert len(data)==135*227*3
  scene=Image.frombytes('RGB',(135,227),data)
  if phase==3:
   d=ImageDraw.Draw(scene);d.ellipse((tx-9,ty-9,tx+9,ty+9),outline='#ffffff',width=1);d.ellipse((tx-2,ty-2,tx+2,ty+2),fill='#ffffff')
  if t in (80,260,412,516,700,848):examples[phase]=scene.copy()
  im=Image.new('RGB',(960,540),BG);d=ImageDraw.Draw(im)
  d.rounded_rectangle((28,27,184,55),radius=14,fill='#172438');d.text((43,34),'OPEN TOY  /  0.3',font=font(12,True),fill=BLUE)
  d.text((40,96),'GRAVITY',font=font(63,True),fill=WHITE)
  d.text((40,163),'WATER',font=font(76,True),fill=WHITE)
  d.text((43,265),'Heat it. Shake it.',font=font(29),fill=WHITE)
  d.text((43,305),'Make a little weather.',font=font(29),fill=WHITE)
  color=GOLD if phase==1 else BLUE
  d.line((44,380,502,380),fill='#26354b',width=1)
  d.ellipse((44,408,52,416),fill=color)
  d.text((66,398),labels[phase],font=font(21,True),fill=color)
  d.text((44,435),subs[phase],font=font(15),fill=MUTED)
  d.text((44,495),'ARDUINO NESSO N1   /   A THERMAL PARTICLE TOY',font=font(11,True),fill=MUTED)
  d.rounded_rectangle((591,25,889,514),radius=28,fill='#142134',outline='#35516a',width=2)
  im.paste(scene.resize((270,454),Image.Resampling.NEAREST),(605,42))
  d=ImageDraw.Draw(im);d.text((656,521),'SCRIPTED SIMULATION',font=font(10,True),fill=MUTED)
  frames.append(im)
 assert proc.wait()==0
OUT.mkdir(exist_ok=True)
# Shared palette prevents colors shifting between GIF frames. Build from representative frames.
sheet=Image.new('RGB',(960*3,540*2))
for i,n in enumerate((20,65,103,129,175,212)):sheet.paste(frames[n],((i%3)*960,(i//3)*540))
palette=sheet.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
quantized=[im.quantize(palette=palette,dither=Image.Dither.NONE) for im in frames]
quantized[0].save(OUT/'gravity-water-hero.gif',save_all=True,append_images=quantized[1:],duration=[70,60,70]*105,loop=0,optimize=True,disposal=1)
frames[65].save(OUT/'gravity-water-poster.png')
# Feature explainer: six readable cards, each illustrated with an actual model frame.
canvas=Image.new('RGB',(960,1040),BG);d=ImageDraw.Draw(canvas)
d.text((34,27),'A handful of ways to play.',font=font(35,True),fill=WHITE)
d.text((36,74),'One screen. Your hands set everything in motion.',font=font(18),fill=MUTED)
cards=[('01','TILT','Follow the pull.', ['Turn the Nesso. The beads tumble','toward the lower edge.'],5,BLUE),('02','HOLD','Bring the boil.', ['Hold the yellow button to add heat.','Watch blue turn gold, then red.'],1,GOLD),('03','RELEASE','Cool it down.', ['Let go to remove the heat quickly.','The remaining motion settles.'],2,BLUE),('04','TOUCH','Make some room.', ['Drag a finger through the pool.','Nearby particles move aside.'],3,'#98e6df'),('05','SHAKE','Scatter a small world.', ['Shake it like a snow globe.','The beads burst into motion.'],4,GOLD),('06','LISTEN','Texture follows touch.', ['Generated noise and low tones','follow heat, motion and touch.'],0,'#baa5ff')]
for k,(num,name,title,copy,phase,color) in enumerate(cards):
 x=28+(k%2)*466;y=126+(k//2)*298
 d.rounded_rectangle((x,y,x+438,y+278),radius=20,fill='#121e30',outline='#26374e',width=1)
 d.text((x+22,y+20),num+' / '+name,font=font(12,True),fill=color)
 d.text((x+22,y+50),title,font=font(24,True),fill=WHITE)
 for j,line in enumerate(copy):d.text((x+22,y+95+j*23),line,font=font(15),fill=MUTED)
 scene=examples[phase].resize((70,118),Image.Resampling.NEAREST)
 canvas.paste(scene,(x+344,y+140))
 if name=='LISTEN':
  import math
  points=[(x+24+i,y+203+int(22*math.sin(i*.18)*(0.6+0.4*math.sin(i*.04)))) for i in range(280)]
  d.line(points,fill=color,width=2)
 else:
  d.line((x+24,y+207,x+295,y+207),fill='#26374e',width=2)
  d.ellipse((x+24,y+201,x+36,y+213),fill=color)
d.text((34,1020),'Screens: scripted host simulation using the shipping particle model. Sound is not included in GIFs.',font=font(12),fill=MUTED)
canvas.save(OUT/'feature-guide.png')
print('Created hero GIF, poster and feature guide in',OUT)
