# SPDX-License-Identifier: CC-BY-SA-3.0
"""One printed faceplate; dimensions in mm, exterior face at print Z=0."""
import json, math, itertools
from pathlib import Path
import FreeCAD as App, Part, Import, MeshPart
import hookup
R=Path(__file__).resolve().parent
OUT=R.parent
V=App.Vector
T=3.2
PRELOAD=.15
L=16.0
TH=1.6
GAP=.8

def box(x,y,z,w,d,h): return Part.makeBox(w,d,h,V(x,y,z))
def cyl(x,y,z,r,h): return Part.makeCylinder(r,h,V(x,y,z))
def rb(x,y,z,w,d,h,r):
 s=box(x,y,z,w,d,h)
 return s.makeFillet(r,[e for e in s.Edges if e.BoundBox.ZLength>h-1e-5])
def fuse(items): return items[0].fuse(items[1:]).removeSplitter() if len(items)>1 else items[0]
def transform(s,origin,angle=0):
 s=s.copy(); s.rotate(V(),V(0,0,1),angle); s.translate(V(*origin)); return s

def yzpoly(points,x,width):
 return Part.Face(Part.makePolygon([V(x,y,z) for y,z in points]+[V(x,*points[0])])).extrude(V(width,0,0))
def desc(name,group,material,s):
 v,f=s.tessellate(.06)
 return dict(name=name,group=group,material=material,vertices=[list(p) for p in v],triangles=f)
def feature(group,name,s):
 o=doc.addObject('Part::Feature','Part');o.Label=name;o.Shape=s;group.addObject(o);return o

# Load the verified vendor-derived board geometry, not its old mounting hardware.
old=App.openDocument(str(R/'Reference-Boards.FCStd'))
parts=[]
for key in ['feather','touch']:
 for obj, info in zip(old.getObject(key).Group, [i for i in json.loads((R/'reference-materials.json').read_text())['hardware'] if i['group']==key]):
  s=obj.Shape.copy(); s.rotate(V(),V(1,0,0),180);s.translate(V(0,176,21.2))
  mat=info['material']
  parts.append((key,info['name'],s,mat))
App.closeDocument(old.Name)
# Replace all five old switch blocks with Adafruit's actual STEP solids.
parts=[p for p in parts if not (p[0]=='feather' and p[3]=='button')]
switches=App.openDocument(str(R/'Feather-Switches.FCStd'))
for info in json.loads((R/'feather-switches.json').read_text())['parts']:
 s=switches.getObject(info['object']).Shape.copy();s.translate(V(5,31,3.7))
 parts.append(('feather',info['name'],s,info['material']))
App.closeDocument(switches.Name)
hookup.imu()
for name,s,mat in hookup.parts['imu']:
 parts.append(('imu',name,transform(s,(90.78,40,3.7),90),mat))
# Rebuild both leads in the new rear-mounted arrangement.
a,b=(23.35,42.43,6.85),(26.95,113,3.15)
hookup.cable('cable1',a,-1,b,-1,[
 (a,(19,42.43,6.85),(18.7,44.5,11.8),(18.7,53,10.8)),
 ((18.7,53,10.8),(18.7,76,8),(11,88,7),(13,101,7)),
 ((13,101,7),(14,113,7),(21,113,3.15),b)])
a,b=(83.05,113,3.15),(81.89,37.35,6.85)
hookup.cable('cable2',a,1,b,-1,[
 (a,(90,113,3.15),(97,112,6),(98,97,7)),
 ((98,97,7),(100,81,8),(100,48,9),(97,32,9)),
 ((97,32,9),(94,20,8),(81.89,21,6.85),b)])
# Rotate second plug into the IMU's -Y socket.
n,s,m=hookup.parts['cable2'][1]
s.translate(V(-1.4,1.4,0));s.rotate(V(*b),V(0,0,1),90);s.translate(V(1.4,1.4,0))
hookup.parts['cable2'][1]=(n,s,m)
for key in ['cable1','cable2']:
 for name,s,mat in hookup.parts[key]: parts.append((key,name,s,mat))

plate=rb(0,0,0,110,176,T,7)
# One integral open-back protective rim; all supports grow from the faceplate.
rim=rb(0,0,3.0,110,176,13,7).cut(rb(2.4,2.4,2.9,105.2,171.2,13.3,4.6))
# USB overmold opening through the rim, open at the back.
rim=rim.cut(box(-1,34.4,3.1,6,16,14))
plate=fuse([plate,rim])
cutters=[box(16.24,33.1,-.1,31.1,18.4,3.5),rb(29.30,100.0,-.1,51.4,26,3.5,2.84)]
# Shared open bay exposes D0/D1/D2 housings and permits a 7 mm access probe.
# It joins the screen aperture; no thin divider obscures the switch actuators.
cutters += [rb(9.0,31.85,-.1,7.5,21.16,3.5,1.0),
            rb(46.9,38.9,-.1,5.5,7.06,3.5,1.0)]
# First-layer relief for touch plug housings, leaving a 1.0 mm front skin.
for x in (18.2,77.9):cutters.append(rb(x,106.5,1.0,14,13,2.4,1.5))
plate=plate.cut(Part.makeCompound(cutters))
# Seat supports on unpopulated board corner regions. No metal fasteners.
supports=[]
for x,y in [(7.54,33.54),(7.54,51.32),(53.26,32.8415),(53.26,51.955)]:
 supports.append(cyl(x,y,3,1.65,.7).cut(cyl(x,y,2.9,1.30,.9)))
for x,y in [(75.54,42.54),(75.54,62.86),(88.6,42.5),(88.6,62.9)]:
 supports.append(cyl(x,y,3,1.8,.7).cut(cyl(x,y,2.9,1.30,.9)))
# Touch board front insertion: four 0.8 mm shelves fix its exposed plane at Z=0.
for x in (32.0,78.0):
 supports.append(box(x-1.6,99.8,1.6,3.2,1.3,1.6))
 supports.append(box(x-1.6,124.9,1.6,3.2,1.3,1.6))
# Clear raised plated rings at the touch shelf corners.
for k,n,s,m in parts:
 if k=='touch' and n=='Plated ring':
  b=s.BoundBox
  tool=cyl((b.XMin+b.XMax)/2,(b.YMin+b.YMax)/2,1.59,b.XLength/2+.10,.22)
  supports=[seat.cut(tool) for seat in supports]
# End stops leave nominal 0.2 mm clearance and keep PCB edges off the shell.
supports += [box(3.6,33.7,3,1.2,3,3.0),box(56,40,3,1.2,4,3.0),
             box(86.5,38.6,3,3,1.2,3.0),box(86.5,65.6,3,3,1.2,3.0)]
plate=fuse([plate]+supports)
# Geometrically isolated planar springs: 16 mm long, 1.6 mm thick, 0.8 mm relief.
# Cuts remain through-open, no concealed support material or extra printed pieces.
beams=[];jaws=[];preloads=[];spring_meta=[]
def spring(name,origin,angle,low,high,lip):
 global plate
 tr=lambda s:transform(s,origin,angle)
 pocket=tr(rb(0,-.15,-.1,17,2.85,3.5,.4))
 plate=plate.cut(pocket)
 beam=tr(box(-1,.3,0,17,TH,3.2))
 # Broader root at the anchored end reduces the notch severity.
 root=tr(cyl(-.25,.9,0,.9,3.2))
 beam=fuse([beam,root])
 beams.append(beam)
 if high>3.2:
  jaws.append(tr(box(13,.3,3.0,3,TH,high+1.0-3.0)))
 # Rounded entry ramps; small interference is deliberate and releases sideways.
 pad=tr(yzpoly([(.31,low+.15),(-PRELOAD,low+.45),(-PRELOAD,high-.20),(.31,high+.15)],13,3))
 preloads.append((name,pad))
 if lip:
  jaws.append(tr(yzpoly([(.31,high+.15),(-.4,high+.86),(.31,high+1.0)],13,3)))
 spring_meta.append(dict(name=name,length_mm=L,thickness_mm=TH,release_gap_mm=GAP,preload_mm=PRELOAD,origin=list(origin),angle=angle))
for side,edge in [('top',53.86),('bottom',31)]:
 for i,x in enumerate(([8,35] if side=='top' else [25,52])):
  spring('Feather '+side+str(i),(x,edge,0),0 if side=='top' else 180,3.7,5.3,True)
for side,edge in [('top',125.7),('bottom',100.3)]:
 for i,x in enumerate(([33,59] if side=='top' else [50,76])):
  spring('Touch '+side+str(i),(x,edge,0),0 if side=='top' else 180,0,1.6,False)
spring('IMU left',(73,44,0),90,3.7,5.3,True)
spring('IMU right',(90.78,61,0),270,3.7,5.3,True)
rigid=fuse([plate]+beams+jaws)
nominal=fuse([rigid]+[s for _,s in preloads])
assert nominal.isValid() and len(nominal.Solids)==1, ('solid',len(nominal.Solids))
# Rigid body must clear all hardware; only named grip pads may preload PCBs.
errors=[];contacts=[]
for key,name,s,mat in parts:
 vol=rigid.common(s).Volume
 if vol>1e-5: errors.append([key,name,round(vol,6)])
 for springname,pad in preloads:
  vol=pad.common(s).Volume
  if vol>1e-5:
   contacts.append([springname,key,name,round(vol,6)])
   if name!='PCB':errors.append(['preload touched component',springname,key,name,vol])
 b=s.BoundBox
 if b.XMin<2.39 or b.XMax>107.61 or b.YMin<2.39 or b.YMax>173.61 or b.ZMin<-.00001 or b.ZMax>16:errors.append(['bounds',key,name,str(b)])
print('UNINTENDED_COLLISIONS',errors,flush=True)
print('INTENDED_PRELOAD',contacts,flush=True)
# Hardware groups must not intersect one another.
for (ka,na,sa,_),(kb,nb,sb,_) in itertools.combinations(parts,2):
 if ka==kb or not sa.BoundBox.intersect(sb.BoundBox):continue
 vol=sa.common(sb).Volume
 if vol>1e-5:errors.append(['hardware',ka,na,kb,nb,round(vol,5)])
# Button identity, positions, visible actuator heights and straight access.
button_checks=[]
for name,x,y,radius in [('D0 BOOT',12.62,35.445,3.5),('D1',12.62,42.43,3.5),('D2',12.62,49.415,3.5),('Reset display side',49.45,42.43,2.5)]:
 actuator=next(s for k,n,s,m in parts if k=='feather' and n==name+' actuator')
 b=actuator.BoundBox
 assert abs((b.XMin+b.XMax)/2-x)<1e-6 and abs((b.YMin+b.YMax)/2-y)<1e-6
 assert abs(b.ZMin-1.8)<1e-6 and abs(b.XLength-2)<1e-6
 probe=cyl(x,y,-.1,radius,2.2)
 obstruction=nominal.common(probe).Volume
 assert obstruction<1e-6,(name,'blocked access',obstruction)
 button_checks.append(dict(name=name,center_mm=[x,y],actuator_recess_mm=b.ZMin,access_diameter_mm=radius*2,cover_obstruction_mm3=obstruction))
(R/'button-checks.json').write_text(json.dumps(button_checks,indent=2))
print('BUTTON_ACCESS_OK',button_checks,flush=True)
print('ALL_ERRORS',errors,flush=True)
(R/'checks.json').write_text(json.dumps(dict(errors=errors,preload=contacts,springs=spring_meta,solid_count=len(nominal.Solids),valid=nominal.isValid()),indent=2))
assert not errors,errors

doc=App.newDocument('OrangeFaceOnePiece')
sg=doc.addObject('App::DocumentObjectGroup','Faceplate')
pg=doc.addObject('App::DocumentObjectGroup','Hardware')
obj=feature(sg,'One-piece faceplate / nominal relaxed spring positions',nominal)
params=doc.addObject('Spreadsheet::Sheet','Dimensions')
for i,(k,v) in enumerate([('Envelope','110 x 176 x 16 mm'),('Face thickness','3.2 mm'),('Spring length','16 mm'),('Spring width','1.6 mm'),('Relief slot','0.8 mm nominal'),('Edge preload','0.15 mm per side'),('Touch face','Z = 0 mm, flush'),('PCB nominal thickness','1.60 mm'),('Revision','First-fit prototype / no physical validation')],1):params.set('A'+str(i),k);params.set('B'+str(i),v)
params.setColumnWidth('A',190);params.setColumnWidth('B',310)
for key,name,s,mat in parts:feature(pg,key+' / '+name,s)
doc.recompute();doc.saveAs(str(OUT/'cad/Orange-Face-One-Piece.FCStd'))
Import.export([obj],str(OUT/'cad/Orange-Face-One-Piece.step'))
mesh=MeshPart.meshFromShape(Shape=nominal,LinearDeflection=.04,AngularDeflection=.12,Relative=False)
assert mesh.isSolid();mesh.write(str(OUT/'print/Orange-Face-One-Piece.stl'))
# Rendered installed geometry excludes the infinitesimal nominal interference,
# representing the spring contacts displaced by the inserted PCBs.
pcbs=[s for k,n,s,m in parts if n=='PCB']
installed=nominal.cut(Part.makeCompound(pcbs))
(R/'assembly.json').write_text(json.dumps(dict(palette={**hookup.COLORS,'cover':'#c94921'},shell=[desc('One-piece faceplate','shell','cover',nominal)],installed_shell=[desc('Loaded faceplate','shell','cover',installed)],hardware=[desc(n,k,m,s) for k,n,s,m in parts],dimensions={'width':110,'height':176,'depth':16})))
print('CAD_OK',mesh.CountFacets,nominal.Volume,flush=True)
