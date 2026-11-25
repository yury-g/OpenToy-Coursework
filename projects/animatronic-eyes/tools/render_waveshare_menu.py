#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Render the actual round header/menu without accessing hardware."""
import os,shlex,subprocess,tempfile
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='waveshare-menu-') as tmp:
 exe=Path(tmp)/'preview'
 subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++14','-O2',str(root/'tools/waveshare_menu_frames.cpp'),'-o',str(exe)],check=True)
 data=subprocess.check_output([str(exe)])
 n=360*360*3;assert len(data)==4*n
 out=Image.new('RGB',(800,840),'#182029');d=ImageDraw.Draw(out)
 mask=Image.new('L',(360,360));ImageDraw.Draw(mask).ellipse((0,0,359,359),fill=255)
 for i,label in enumerate(['Play - curved header','Menu - page 1 of 4','Sound - quiet choices','Performance - restore guide']):
  im=Image.frombytes('RGB',(360,360),data[i*n:(i+1)*n]);x=20+(i%2)*400;y=30+(i//2)*420
  out.paste(im,(x,y),mask);d.text((x,y-18),label,fill='white')
 d.text((20,820),'SOURCE PREVIEW - not a photograph. Battery unavailable shown as --.',fill='white')
 out.save(root/'docs/images/waveshare-menu.png')
