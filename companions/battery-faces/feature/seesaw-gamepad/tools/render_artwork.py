"""Common physical-scale SVG board illustrations with unchanged framebuffer pixels.
Source dimensions and assumptions: assets/physical-dimensions.json.
Run after verify_family.py. Animation uses exported renderer bitmaps, illustrative only.
"""
from pathlib import Path
import sys,json,shutil,hashlib,datetime,html
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
A=ROOT/'assets'
COLORS={'blue':'#28a0ff','orange':'#ffaa28','yellow':'#ffdc28','rainbow':'#f881e3','green':'#68d09a','purple':'#ae72ff','pink':'#ff84c6','teal':'#24d2bd','silver':'#c4cbd0'}
def pixels(path):
 im=Image.open(path).convert('RGB');w,h=im.size;groups={}
 for y in range(h):
  x=0
  while x<w:
   c=im.getpixel((x,y));end=x+1
   while end<w and im.getpixel((end,y))==c:end+=1
   if c!=(0,0,0):groups.setdefault(c,[]).append(f'M{x} {y}h{end-x}v1h-{end-x}z')
   x=end
 return w,h,''.join(f'<path fill="#{c[0]:02x}{c[1]:02x}{c[2]:02x}" d="{"".join(d)}"/>' for c,d in groups.items())
def svg(w,h,body,title):
 return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" role="img"><title>{html.escape(title)}</title><defs><linearGradient id="spectrum"><stop stop-color="#ff5555"/><stop offset=".2" stop-color="#ffad28"/><stop offset=".4" stop-color="#e6f83e"/><stop offset=".6" stop-color="#3eeb9a"/><stop offset=".8" stop-color="#42aaff"/><stop offset="1" stop-color="#df6aff"/></linearGradient></defs>{body}</svg>'
def text(x,y,s,size=18,color='#aebdd4',extra=''):
 return f'<text x="{x}" y="{y}" fill="{color}" font-family="Arial,sans-serif" font-size="{size}" {extra}>{html.escape(str(s))}</text>'
DIMENSIONS=json.loads((A/'physical-dimensions.json').read_text())
SCALE=DIMENSIONS['scale_px_per_mm']
MODELS={'blue':['Adafruit Feather','Reverse TFT'],'green':['NEW Adafruit Feather','Reverse TFT'],'orange':['M5Stack StickS3'],'pink':['Arduino Nesso N1'],'yellow':['Waveshare Touch LCD 1.69'],'rainbow':['Waveshare Touch LCD 1.69'],'purple':['CYD ESP32-2432S028'],'silver':['Waveshare RLCD 4.2'],'tab5':['M5Stack Tab5']}

def geometry(name):
 d=DIMENSIONS['devices'][name];bw,bh=d['outer_mm'];pw,ph=d['panel_pixels']
 if 'screen_active_mm' in d:sw,sh=d['screen_active_mm']
 else:
  diag=d['screen_diagonal_in']*25.4
  sw=diag*pw/(pw*pw+ph*ph)**.5;sh=sw*ph/pw
 assert abs(sw/sh-pw/ph)<.00001,(name,'panel aspect')
 assert sw<bw and sh<bh,(name,'screen fits body')
 return bw,bh,sw,sh

def board(name,path):
 """Draw in millimeters, then apply ONE scale for every device and screen."""
 bw,bh,sw,sh=geometry(name);w,h,p=pixels(path)
 # Preserve the physical panel and capture aspect independently; CYD needs sidebars.
 k=min(sw/w,sh/h);cw,ch=w*k,h*k
 sx,sy=(bw-sw)/2,(bh-sh)/2
 if name in ('blue','green'):sy=9
 if name in ('orange','pink'):sy=6
 if name=='purple':sy=8
 fill={'blue':'#12272e','green':'#12272e','orange':'#e9941e','pink':'#263345','purple':'#a88023','silver':'#79858b','tab5':'#1e2a3a'}.get(name,'#142b42')
 radius=2.54 if name in ('blue','green') else 1.5 if name in ('silver','purple') else 3
 b=f'<rect class="physical-outline" x="0" y="0" width="{bw}" height="{bh}" rx="{radius}" fill="{fill}" stroke="#96a8b9" stroke-width=".35"/>'
 if name in ('blue','green'):
  for x in (1.8,bw-1.8):
   for i in range(16):
    y=4+i*2.54
    b+=f'<circle cx="{x}" cy="{y}" r=".7" fill="#07161e" stroke="#c4ab6b" stroke-width=".3"/>'
  for x in (6,11.43,16.86):b+=f'<rect x="{x-1.3}" y="39" width="2.6" height="2.6" rx=".4" fill="#c2c7cb"/>'
 elif name=='purple':
  for x in (2.5,bw-2.5):
   for y in (2.5,bh-2.5):b+=f'<circle cx="{x}" cy="{y}" r="1.35" fill="#192127" stroke="#e7c77c" stroke-width=".35"/>'
 elif name in ('orange','pink'):
  b+=f'<circle cx="{bw/2}" cy="39" r="2.6" fill="#151e2a" stroke="#7a8b9c" stroke-width=".3"/>'
 elif name=='silver':
  b+=f'<rect x="{bw-1.2}" y="30" width=".8" height="4" rx=".3" fill="#dce3df"/>'
 # Bezel is decorative. The active screen rectangle has the documented physical ratio.
 b+=f'<rect x="{sx-.6}" y="{sy-.6}" width="{sw+1.2}" height="{sh+1.2}" rx=".8" fill="#020408"/>'
 b+=f'<rect class="physical-panel" x="{sx}" y="{sy}" width="{sw}" height="{sh}" fill="black"/>'
 cx,cy=sx+(sw-cw)/2,sy+(sh-ch)/2
 b+=f'<svg class="framebuffer" x="{cx}" y="{cy}" width="{cw}" height="{ch}" viewBox="0 0 {w} {h}" preserveAspectRatio="xMidYMid meet" shape-rendering="crispEdges"><rect width="{w}" height="{h}" fill="black"/>{p}</svg>'
 return f'<g class="device" data-device="{name}" data-width-mm="{bw}" data-height-mm="{bh}" transform="scale({SCALE})">{b}</g>'

def capture_label(name):
 if name in ('pink','yellow'):return 'Historical v15 capture'
 if name=='rainbow':return 'Historical v17 capture'
 if name=='tab5':return 'Actual v19 capture'
 return 'Actual v18 capture'

def caption(name,cx,y,size=18):
 d=DIMENSIONS['devices'][name];bw,bh=d['outer_mm']
 title='TAB5' if name=='tab5' else name.upper()+' FACE'
 b=text(cx,y,title,size+4,COLORS.get(name,'#f6f8ff'),'font-weight="bold" text-anchor="middle"')
 for i,line in enumerate(MODELS[name]):b+=text(cx,y+26+i*22,line,size,'#c3cfe0','text-anchor="middle"')
 offset=26+22*len(MODELS[name])
 b+=text(cx,y+offset,f'{bw:g} × {bh:g} mm',size,'#aebdd4','text-anchor="middle"')
 b+=text(cx,y+offset+23,capture_label(name),size-3,'#aebdd4','text-anchor="middle"')
 return b

def archive_renderings():
 # Keep every pre-change visual, including the old README preview, without overwriting.
 dest=A/'archive/before-physical-scale-2026-10-20'
 for p in sorted((A/'renders').glob('*')):
  if not p.is_file():continue
  target=dest/p.name
  if not target.exists():
   dest.mkdir(exist_ok=True,parents=True);shutil.copy2(p,target)

def main():
 for d in ['renders','screenshots','archive','animation']:(A/d).mkdir(exist_ok=True,parents=True)
 archive_renderings()
 for p in sorted((ROOT/'outputs').glob('*.png')):
  target=A/'archive'/p.name
  if not target.exists():shutil.copy2(p,target)
 members=['blue','green','orange','yellow','rainbow','purple','pink','silver','tab5'];drawings={}
 for name in members:
  src=ROOT/f'outputs/v18-screen-{name}-shows-{name}.png'
  if name=='silver':src=ROOT/'outputs/v18-screen-silver-native.png'
  if name=='tab5':
   src=ROOT/'outputs/v19-tab5-screen.png'
   if not src.exists():src=ROOT/'outputs/v18-tab5-screen.png'
  if not src.exists():src=ROOT/'outputs/v15-screen-orange-shows-orange.png' if name=='pink' else ROOT/f'outputs/v17-screen-{name}-shows-{name}.png'
  if not src.exists():
   old='orange' if name=='pink' else name
   src=ROOT/f'outputs/v15-screen-{old}-shows-{old}.png'
  assert src.exists(),src
  shutil.copy2(src,A/f'screenshots/{name}.png');b=board(name,src);drawings[name]=b
  bw,bh,_,_=geometry(name);width=max(360,round(bw*SCALE+80));height=round(bh*SCALE+190)
  content=f'<g transform="translate({(width-bw*SCALE)/2},25)">{b}</g>'+caption(name,width/2,bh*SCALE+62)
  content+=text(width/2,height-12,'6 px/mm · simplified physical outline',13,'#aebdd4','text-anchor="middle"')
  (A/f'renders/{name}-board.svg').write_text(svg(width,height,content,f'{name.title()}: common physical scale, unchanged {capture_label(name).lower()}'))
 # Shared row baselines make the relative heights directly comparable.
 body='<rect width="1680" height="1580" fill="#09111e"/>'+text(60,70,'BATTERY FACES',48,'#f6f8ff','font-weight="bold"')
 body+=text(62,110,'One physical scale for every board · 6 px/mm · unchanged screen captures',24)
 for name,x in [('tab5',85),('silver',1000)]:
  bw,bh,_,_=geometry(name);y=700-bh*SCALE
  body+=f'<g transform="translate({x},{y})">{drawings[name]}</g>'+caption(name,x+bw*SCALE/2,738,22)
 # Widths are caption allocations only; they never rescale the devices.
 cursor=60
 for name,space in [('purple',310),('blue',190),('green',190),('orange',190),('pink',190),('yellow',220),('rainbow',220)]:
  bw,bh,_,_=geometry(name);cx=cursor+space/2;y=1380-bh*SCALE
  body+=f'<g transform="translate({cx-bw*SCALE/2},{y})">{drawings[name]}</g>'+caption(name,cx,1412,16)
  cursor+=space
 # A scale bar remains valid after image resizing and avoids an actual-size claim.
 body+='<path d="M1030 890v-8h300v8M1030 882v-8M1090 882v-8M1150 882v-8M1210 882v-8M1270 882v-8M1330 882v-8" fill="none" stroke="#aebdd4" stroke-width="2"/>'
 body+=text(1180,913,'50 mm',17,'#aebdd4','text-anchor="middle"')
 body+=text(840,1553,'Simplified outer board / enclosure silhouettes · not precision CAD · historical captures retain their original on-screen labels',18,'#aebdd4','text-anchor="middle"')
 (A/'renders/battery-faces-family.svg').write_text(svg(1680,1580,body,'Battery Faces at one physical scale: Tab5 largest; Silver second largest; actual framebuffer captures'))
 camera='<rect x="75" y="45" width="198" height="270" rx="15" fill="#145b50" stroke="#96ccab" stroke-width="4"/>'
 for cx in (84,264):
  for cy in range(65,300,35):camera+=f'<rect x="{cx-9}" y="{cy}" width="18" height="16" rx="3" fill="#d3b975"/>'
 camera+='<rect x="132" y="27" width="84" height="34" rx="6" fill="#aebdc7"/><rect x="110" y="98" width="128" height="145" rx="9" fill="#252f35"/><circle cx="174" cy="166" r="49" fill="#131d29" stroke="#8796a9" stroke-width="5"/><circle cx="174" cy="166" r="30" fill="#172b4a"/><circle cx="163" cy="154" r="11" fill="#6188a6" opacity=".7"/>'
 camera+=text(174,365,'TEAL CAMERA',25,COLORS['teal'],'font-weight="bold" text-anchor="middle"')+text(174,398,'Seeed XIAO ESP32-S3 Sense',16,'#c3cfe0','text-anchor="middle"')+text(174,425,'160×120 B&W / 80×60 color · no built-in screen',13,'#aebdd4','text-anchor="middle"')
 (A/'renders/teal-board.svg').write_text(svg(348,470,camera,'Teal XIAO camera: original illustrative board'))
 # Self-contained SMIL: no JavaScript, external images or live network content.
 poses=json.loads((A/'animation/renderer-poses.json').read_text());groups=[]
 for i,pose in enumerate(poses):
  data=pose['pixels'];d=''
  for y in range(64):
   for x in range(128):
    if (data[(y//8)*128+x]>>(y%8))&1:d+=f'M{x} {y}h1v1h-1z'
  op=['0']*5;op[i]='1';op+=op[:1]
  content=f'<path d="{d}" fill="url(#spectrum)" transform="translate(72,145) scale(4)"/>'+text(570,105,str(pose['percent'])+'%',55,'#f6f8ff','text-anchor="end"')
  content+=f'<rect x="74" y="62" width="{pose["percent"]*3}" height="37" rx="5" fill="url(#spectrum)"/>'
  groups.append(f'<g opacity="{op[0]}"><animate attributeName="opacity" values="{";".join(op)}" keyTimes="0;.2;.4;.6;.8;1" dur="10s" calcMode="discrete" repeatCount="indefinite"/>{content}</g>')
 anim='<rect width="660" height="470" rx="28" fill="#09111e"/>'+text(42,36,'BATTERY MOOD',18,'#aebdd4')+'<rect x="70" y="58" width="308" height="45" rx="8" fill="none" stroke="#7990a9" stroke-width="3"/><rect x="380" y="70" width="8" height="20" fill="#7990a9"/>'
 anim+=''.join(groups)+text(330,435,'Illustrative charge animation · not live readings',19,'#aebdd4','text-anchor="middle"')
 (A/'animation/battery-mood.svg').write_text(svg(660,470,anim,'Illustration: charge rises and the original face becomes happier'))
 manifest={'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'kind':'Real framebuffer snapshots; original illustrative board drawings; simulated animation','files':{str(p.relative_to(A)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(A.rglob('*')) if p.is_file() and p.name!='manifest.json'}}
 (A/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('Generated common-scale SVG family and nine active board illustrations; retained real captures and historical image archive')
if __name__=='__main__':main()
