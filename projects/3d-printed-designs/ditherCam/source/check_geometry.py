from pathlib import Path
import json, math, hashlib, sys
import numpy as np,trimesh,manifold3d as M
src=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
p=json.loads((src/'parameters.json').read_text());axis=p['hinge']['axis_z_print_mm'];sole=p['hinge']['y_sole_print_mm']
meshes={n:trimesh.load_mesh(src/(n+'.stl')) for n in ['body','pod','hw_rlcd','hw_xiao_pod','Camera-print']}
def solid(a):return M.Manifold(M.Mesh(np.asarray(a.vertices,np.float32),np.asarray(a.faces,np.uint32)))
s={n:solid(a) for n,a in meshes.items()}
def transform(angle,shift=0):
 r=math.radians(angle-90);c,t=math.cos(r),math.sin(r)
 return np.array([[1,0,0,shift],[0,c,-t,t*axis],[0,t,c,axis*(1-c)]])
report={'source_sha256':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in [src/'Camera.scad',src/'parameters.json',*[src/(n+'.stl') for n in meshes]]},'mesh':{},'sweep':{}}
for n,a in meshes.items():report['mesh'][n]={'watertight':bool(a.is_watertight),'positive_volume':bool(a.is_volume),'shells':len(a.split()),'bounds':a.bounds.tolist()}
for dx in [-.6,0,.6]:
 worst=0;worstangle=0;hwworst=0
 for a in range(181):
  tx=transform(a,dx);pod=s['pod'].transform(tx);hwp=s['hw_xiao_pod'].transform(tx)
  vol=(s['body']^pod).volume()
  hwvol=max((s['body']^hwp).volume(),(s['hw_rlcd']^pod).volume(),(s['hw_rlcd']^hwp).volume())
  if vol>worst:worst=vol;worstangle=a
  hwworst=max(hwworst,hwvol)
 report['sweep'][str(dx)]={'poses':181,'max_shell_intersection_mm3':worst,'worst_angle':worstangle,'max_moving_hardware_intersection_mm3':hwworst}
report['static_rlcd_intersections']=[{'volume':q.volume(),'bounds':q.bounding_box()} for q in (s['body']^s['hw_rlcd']).decompose()]
report['assembly_difference_mm3']=((s['body']+s['pod'])-s['Camera-print']).volume()+(s['Camera-print']-(s['body']+s['pod'])).volume()
(out/'independent-geometry.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k not in ['source_sha256','mesh']},indent=2))
