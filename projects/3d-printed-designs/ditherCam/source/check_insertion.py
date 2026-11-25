from pathlib import Path
import json,sys,hashlib
import trimesh,numpy as np,manifold3d as M
src=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
def solid(n):
 a=trimesh.load_mesh(src/n);return M.Manifold(M.Mesh(np.asarray(a.vertices,np.float32),np.asarray(a.faces,np.uint32)))
pod=solid('pod.stl');stack=solid('hw_xiao_stack.stl')
tips=M.Manifold.cube([6,3,5]).translate([22,-24,4])+M.Manifold.cube([6,3,5]).translate([-28,-24,4])
rigid=pod-tips
path=[{'offset_mm':float(z),'rigid_interference_mm3':float((rigid^stack.translate([0,0,float(z)])).volume())} for z in np.arange(0,50.01,.5)]
worst=max(path,key=lambda q:q['rigid_interference_mm3'])
r={'samples':path,'worst':worst,'seated_full_stack_overlap_mm3':float((pod^stack).volume()),'tolerance_mm3':.01,'scope':'Rigid installation path only. Excludes two bottom detent tip boxes at X +/-22..28, Y -24..-21, Z4..9. Physical spring deflection, cycle life and measured unit tolerances untested.'}
(out/'independent-insertion.json').write_text(json.dumps(r,indent=2))
(out/'model-sha256.json').write_text(json.dumps({f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in src.iterdir() if f.is_file()},indent=2))
assert worst['rigid_interference_mm3']<.01;assert r['seated_full_stack_overlap_mm3']<.01
print(worst)
