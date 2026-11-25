from pathlib import Path
import itertools,json
import trimesh,numpy as np
p=Path(__file__).resolve().parent.parent/'model';par=json.loads((p/'parameters.json').read_text());axis=par['hinge']['axis_z_print_mm'];sole=par['hinge']['y_sole_print_mm']
b=trimesh.load_mesh(p/'body.stl');c=trimesh.load_mesh(p/'pod.stl')
rho=.00124; bm=b.volume*rho;cm=c.volume*rho
# Sensitivity bounds, not measured masses. Deliberately include a very low stationary
# shell mass and front-biased stationary electronics with maximum moving board mass.
worst=(1e9,None);best=(-1e9,None)
for frac_b,dy_b,frac_c,dy_c,m_r,y_r,m_cell,m_cam,z_cam,m_li in itertools.product([.2,1],[-5,5],[.2,1],[-3,3],[50,68],[6,15],[0,48],[22,32],[3,10],[0,24]):
 for angle in [0,180]:
  sign=1 if angle==0 else -1
  y_body=float(b.center_mass[2])+dy_b
  y_pod=axis-sign*(float(c.center_mass[1])+dy_c)
  y_cam=axis-sign*(par['pod']['y_print_pcb_bottom_mm']-z_cam)
  y_li=axis-sign*(par['pod']['y_print_pcb_bottom_mm']+7.6)
  masses=[bm*frac_b,cm*frac_c,m_r,m_cell,m_cam,m_li]
  ys=[y_body,y_pod,y_r,25,y_cam,y_li]
  cg=sum(m*y for m,y in zip(masses,ys))/sum(masses)
  case={'angle':angle,'mass_g':sum(masses),'cg_y_mm':cg,'body_fraction_of_solid':frac_b,'pod_fraction_of_solid':frac_c,'camera_mass_g':m_cam,'camera_cg_above_base_mm':z_cam,'lipo_mass_g':m_li,'cell_18650_g':m_cell}
  if cg<worst[0]:worst=(cg,case)
  if cg>best[0]:best=(cg,case)
result={'status':'estimated_sensitivity_only_not_measured_balance','cases':2048,'min_front_margin_mm':worst[0],'min_rear_margin_mm':58-best[0],'worst_front_case':worst[1],'assumptions':['Shell masses varied between 20% and 100% of solid PLA volume; this is not a slicer mass prediction.','Body centroid varied +/-5 mm; moving shell centroid +/-3 mm in its print-space Y direction.','Bare RLCD 50-68 g at 6-15 mm behind front sole edge; optional 18650 0-48 g at 25 mm.','XIAO plus base 22-32 g with center 3-10 mm above its base PCB; LiPo 0-24 g at assumed 58x42.5x4 mm bay center.','Only gravity on a level table; no cable pull, shove, slope, dynamic flipping, or actual measurements.']}
(p.parent/'validation/balance-sensitivity.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
