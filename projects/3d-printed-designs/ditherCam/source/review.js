import * as T from 'three';
import {OrbitControls} from 'three/examples/jsm/controls/OrbitControls.js';
import {STLLoader} from 'three/examples/jsm/loaders/STLLoader.js';
const $=id=>document.getElementById(id), payload=JSON.parse($('assets').textContent), assets=payload.meshes||payload, params=payload.parameters||{}, hinge=params.hinge||{};
const scene=new T.Scene();scene.background=new T.Color('#edece7');
const renderer=new T.WebGLRenderer({antialias:true});renderer.setPixelRatio(Math.min(devicePixelRatio,2));$('stage').appendChild(renderer.domElement);
const camera=new T.PerspectiveCamera(38,1,.1,2000);camera.up.set(0,0,1);
const controls=new OrbitControls(camera,renderer.domElement);controls.enableDamping=true;
scene.add(new T.HemisphereLight(0xffffff,0x777970,2.4));const light=new T.DirectionalLight(0xffffff,3);light.position.set(100,-180,270);scene.add(light);
const root=new T.Group(), pod=new T.Group();root.add(pod);scene.add(root);const hw=[];
const loader=new STLLoader();
for(const [key,encoded] of Object.entries(assets)){
 const bytes=Uint8Array.from(atob(encoded),c=>c.charCodeAt(0));const geom=loader.parse(bytes.buffer);geom.computeVertexNormals();
 const hardware=key.startsWith('hw_'), moving=key==='pod'||key==='hw_xiao_pod'||key==='hw_oled_active';
 const mat=new T.MeshStandardMaterial({color:key==='hw_oled_active'?0x122a2b:hardware?0x708d9c:moving?0xbf8257:0x385451,emissive:key==='hw_oled_active'?0x183b3b:0x000000,emissiveIntensity:key==='hw_oled_active'?.35:0,roughness:.75,metalness:0,side:T.DoubleSide});
 const mesh=new T.Mesh(geom,mat);mesh.name=key;(moving?pod:root).add(mesh);if(hardware)hw.push(mesh);
}
const arrow=new T.ArrowHelper(new T.Vector3(0,-1,0),new T.Vector3(-25.15,(params.pod?.y_print_pcb_bottom_mm||-22.2)-(params.pod?.camera_lens_min_above_base_bot_mm||18.9),23.85),23,0xb1482c,6,3);pod.add(arrow);
const grid=new T.GridHelper(320,16,0xb3b7b0,0xd4d6ce);grid.rotation.x=Math.PI/2;grid.position.set(0,25,-.2);scene.add(grid);
let angle=180, print=false, playing=false, phase=0;
const axis=new T.Vector3(0,0,hinge.axis_z_print_mm||21.5),qx=new T.Quaternion();
function update(){
 qx.setFromAxisAngle(new T.Vector3(1,0,0),(angle-90)*Math.PI/180);pod.quaternion.copy(qx);pod.position.copy(axis.clone().sub(axis.clone().applyQuaternion(qx)));
 root.rotation.x=print?0:-Math.PI/2;root.position.z=print?0:(hinge.y_sole_print_mm||98);
 $('angle').value=String(angle);$('degrees').textContent=Math.round(angle)+'°';$('mode').textContent=print?'Print pose':angle<1?'Outward facing':angle>179?'Selfie':'Flipping';
 window.cameraPreview={angle,orientation:print?'print':'tabletop',meshcounts:Object.keys(assets).length,movingtransform:{position:pod.position.toArray(),quaternion:pod.quaternion.toArray()}};
}
function view(){camera.position.set(200,-260,print?220:180);controls.target.set(0,print?25:25,print?25:77);controls.update();}
function setAngle(v){playing=false;$('play').textContent='Play flip';angle=v;update();}
$('angle').oninput=e=>setAngle(+e.target.value);$('forward').onclick=()=>{print=false;setAngle(0);view();};$('selfie').onclick=()=>{print=false;setAngle(180);view();};
$('print').onclick=()=>{print=!print;setAngle(print?90:0);view();};$('hardware').onchange=e=>hw.forEach(x=>x.visible=e.target.checked);$('reset').onclick=view;
$('play').onclick=()=>{playing=!playing;print=false;phase=Math.acos(1-angle/90);$('play').textContent=playing?'Pause':'Play flip';update();};
function resize(){const r=$('stage').getBoundingClientRect();renderer.setSize(r.width,r.height);camera.aspect=r.width/r.height;camera.updateProjectionMatrix();}new ResizeObserver(resize).observe($('stage'));
let last=performance.now();function tick(now){const dt=Math.min((now-last)/1000,.1);last=now;if(playing){phase+=dt*.7;angle=90*(1-Math.cos(phase));update();}controls.update();renderer.render(scene,camera);requestAnimationFrame(tick);}update();view();requestAnimationFrame(tick);
