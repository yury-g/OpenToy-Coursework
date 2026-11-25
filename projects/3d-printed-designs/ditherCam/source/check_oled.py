from pathlib import Path
import sys,json,math
import trimesh,numpy as np,manifold3d as M
src=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(exist_ok=True,parents=True)
def solid(mesh):return M.Manifold(M.Mesh(np.asarray(mesh.vertices,np.float32),np.asarray(mesh.faces,np.uint32)))
pod=solid(trimesh.load_mesh(src/'pod.stl'));body=solid(trimesh.load_mesh(src/'body.stl'))
a=trimesh.load_mesh(src/'hw_oled_active.stl');lo,hi=a.bounds
xs=np.linspace(lo[0]+.02,hi[0]-.02,15);zs=np.linspace(lo[2]+.02,hi[2]-.02,9)
results=[]
for horizontal,vertical in [(0,0),(-15,0),(15,0),(0,-15),(0,15)]:
 blocked=[]
 d=np.array([150*math.tan(math.radians(horizontal)),-150,150*math.tan(math.radians(vertical))])
 for x in xs:
  for z in zs:
   start=np.array([x,lo[1]-.01,z]);hits=(pod+body).ray_cast(start.tolist(),(start+d).tolist())
   if hits:blocked.append([float(x),float(z)])
 results.append({'horizontal_deg':horizontal,'vertical_deg':vertical,'sample_count':len(xs)*len(zs),'blocked_count':len(blocked),'blocked_points':blocked})
r={'active_surface_bounds_print_mm':a.bounds.tolist(),'shell_visibility':results,'scope':'Shell occlusion only, modeled OLED plane; no live video or physical optical measurement.'}
(out/'oled-visibility.json').write_text(json.dumps(r,indent=2));print(json.dumps({**r,'shell_visibility':[{k:v for k,v in t.items() if k!='blocked_points'} for t in results]},indent=2))
assert results[0]['blocked_count']==0, 'OLED active screen occluded on axis'
