"""Original editable SVG board illustrations with exact RGB565 framebuffer pixels.
Run after verify_family.py. Animation uses exported renderer bitmaps, illustrative only.
"""
from pathlib import Path
import sys,json,shutil,hashlib,datetime,html
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
A=ROOT/'assets'
COLORS={'blue':'#28a0ff','orange':'#ffaa28','yellow':'#ffdc28','rainbow':'#f881e3','green':'#68d09a','purple':'#ae72ff','pink':'#ff84c6','teal':'#24d2bd'}
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
def board(name,path):
 w,h,p=pixels(path);c=COLORS[name]
 if name=='blue':
  b='<rect x="60" y="20" width="228" height="508" rx="20" fill="#12272e" stroke="#4f737e" stroke-width="3"/>'
  for side in (70,278):
   for y in range(45,500,34):b+=f'<circle cx="{side}" cy="{y}" r="7" fill="#07161e" stroke="#c4ab6b" stroke-width="4"/>'
  for x in (112,174,236):b+=f'<rect x="{x-15}" y="450" width="30" height="32" rx="5" fill="#bbbfc4"/>'
  b+='<rect x="145" y="500" width="58" height="35" rx="5" fill="#b9c2ca"/>'
  sx,sy,sw,sh=90,95,168,168*h/w
 elif name=='orange':
  b='<rect x="44" y="15" width="260" height="520" rx="42" fill="#222a38" stroke="#74839a" stroke-width="3"/><rect x="33" y="165" width="12" height="70" rx="6" fill="#ed9b24"/><rect x="304" y="270" width="12" height="70" rx="6" fill="#ed9b24"/>'
  b+=text(174,492,'NESSO N1',18,'#8997ab','text-anchor="middle"')
  sx,sy,sw,sh=84,64,180,180*h/w
 elif name=='pink':
  b='<rect x="62" y="18" width="224" height="508" rx="27" fill="#f39d20" stroke="#ffd18a" stroke-width="3"/><rect x="286" y="205" width="12" height="58" rx="6" fill="#434b57"/><rect x="141" y="5" width="66" height="25" rx="4" fill="#abb8c3"/><circle cx="174" cy="460" r="20" fill="#232d3e"/>'
  sx,sy,sw,sh=90,74,168,168*h/w
 elif name=='purple':
  b='<rect x="23" y="32" width="302" height="468" rx="13" fill="#a88023" stroke="#e7c77c" stroke-width="3"/>'
  for x in (40,308):
   for y in (48,484):b+=f'<circle cx="{x}" cy="{y}" r="7" fill="#192127" stroke="#e7c77c" stroke-width="3"/>'
  b+='<rect x="136" y="492" width="76" height="28" rx="5" fill="#b6bfce"/>'
  # Physical240x320; logical135x240 uniformly scales to180x320 with sidebars.
  sx,sy,sw,sh=84,65,180,320
  b+='<rect x="54" y="65" width="240" height="320" fill="#000"/>'
 else:
  b='<rect x="21" y="36" width="306" height="438" rx="38" fill="#142b42" stroke="#5d829c" stroke-width="3"/>'
  for x in (40,308):
   for y in (58,450):b+=f'<circle cx="{x}" cy="{y}" r="8" fill="#0b111c" stroke="#ceae68" stroke-width="4"/>'
  b+='<rect x="140" y="464" width="68" height="33" rx="6" fill="#bcc9d3"/><circle cx="23" cy="260" r="8" fill="#7c8c9c"/>'
  sx,sy,sw,sh=54,87,240,280
 if name=='purple':b+='<rect x="49" y="60" width="250" height="330" rx="10" fill="#020408"/>'
 else:b+=f'<rect x="{sx-5}" y="{sy-5}" width="{sw+10}" height="{sh+10}" rx="10" fill="#020408"/>'
 b+=f'<svg x="{sx}" y="{sy}" width="{sw}" height="{sh}" viewBox="0 0 {w} {h}"><rect width="{w}" height="{h}" fill="black"/>{p}</svg>'
 b+=text(174,576,name.upper()+' FACE',25,c,'font-weight="bold" text-anchor="middle"')
 models={'blue':'Adafruit Feather Reverse TFT','orange':'Arduino Nesso N1','yellow':'Waveshare Touch LCD 1.69','rainbow':'Waveshare Touch LCD 1.69','purple':'CYD ESP32-2432S028','pink':'M5Stack StickS3'}
 b+=text(174,603,models[name],16,'#c3cfe0','text-anchor="middle"')
 b+=text(174,624,'Last verified v15 capture' if name in ('orange','yellow') else 'Actual v17 firmware screen capture',12,'#aebdd4','text-anchor="middle"')
 return b

def main():
 for d in ['renders','screenshots','archive','animation']:(A/d).mkdir(exist_ok=True,parents=True)
 for p in sorted((ROOT/'outputs').glob('*.png')):
  shutil.copy2(p,A/'archive'/p.name)
 members=['blue','orange','yellow','rainbow','purple','pink'];parts=[]
 for i,name in enumerate(members):
  src=ROOT/f'outputs/v17-screen-{name}-shows-{name}.png'
  if not src.exists():src=ROOT/f'outputs/v15-screen-{name}-shows-{name}.png'
  assert src.exists(),src
  shutil.copy2(src,A/f'screenshots/{name}.png');b=board(name,src)
  (A/f'renders/{name}-board.svg').write_text(svg(348,630,b,f'{name.title()} Face: illustrative board, real screen capture'))
  parts.append(f'<g transform="translate({25+i*315},150) scale(.85)">{b}</g>')
 src=ROOT/'outputs/v17-tab5-screen.png';shutil.copy2(src,A/'screenshots/tab5.png');w,h,p=pixels(src)
 tab='<rect x="0" y="0" width="1100" height="667" rx="34" fill="#1e2a3a" stroke="#7a899d" stroke-width="3"/>'
 tab+=f'<svg x="35" y="38" width="1030" height="579.375" viewBox="0 0 {w} {h}"><rect width="{w}" height="{h}" fill="black"/>{p}</svg>'+text(550,650,'M5STACK TAB5 · LIVE ADAPTIVE MONITOR',18,'#becbe0','text-anchor="middle"')
 (A/'renders/tab5-board.svg').write_text(svg(1100,667,tab,'Tab5 illustration with actual v17 live-node capture'))
 camera='<rect x="75" y="45" width="198" height="270" rx="15" fill="#145b50" stroke="#96ccab" stroke-width="4"/>'
 for cx in (84,264):
  for cy in range(65,300,35):camera+=f'<rect x="{cx-9}" y="{cy}" width="18" height="16" rx="3" fill="#d3b975"/>'
 camera+='<rect x="132" y="27" width="84" height="34" rx="6" fill="#aebdc7"/><rect x="110" y="98" width="128" height="145" rx="9" fill="#252f35"/><circle cx="174" cy="166" r="49" fill="#131d29" stroke="#8796a9" stroke-width="5"/><circle cx="174" cy="166" r="30" fill="#172b4a"/><circle cx="163" cy="154" r="11" fill="#6188a6" opacity=".7"/>'
 camera+=text(174,365,'TEAL CAMERA',25,COLORS['teal'],'font-weight="bold" text-anchor="middle"')+text(174,398,'Seeed XIAO ESP32-S3 Sense',16,'#c3cfe0','text-anchor="middle"')+text(174,425,'160×120 B&W / 80×60 color · no built-in screen',13,'#aebdd4','text-anchor="middle"')
 (A/'renders/teal-board.svg').write_text(svg(348,470,camera,'Teal XIAO camera: original illustrative board'))
 body='<rect width="1950" height="1500" fill="#09111e"/>'+text(50,70,'BATTERY FACES',48,'#f6f8ff','font-weight="bold"')+text(52,108,'Five battery moods + USB + camera. One living network.',24)
 body+=''.join(parts)+f'<g transform="translate(130,760)">{tab}</g><g transform="translate(1450,825) scale(1.15)">{camera}</g>'+text(975,1468,'Original illustrations + real captures · Orange/Yellow: last verified v15 · Green retired',19,'#aebdd4','text-anchor="middle"')
 (A/'renders/battery-faces-family.svg').write_text(svg(1950,1500,body,'Battery Faces: Blue, Orange, Yellow, Rainbow, Purple, Pink, Teal camera and Tab5'))
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
 print('Generated SVG family, six board illustrations, exact screen captures, animated SVG and image archive')
if __name__=='__main__':main()
