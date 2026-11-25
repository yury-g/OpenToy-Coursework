/* Premium profile: proxy derived from the A20 family STEP linked by AGFRC.
   Its file is named A20BHM; A20CLM fit and stock horn must be checked with a gauge.
   H includes shaft, unlike the published 27.5 mm body size.
   Shared original CAD for exploratory desk toys, 2026-10-01. Units mm.
   Ordinary 0.4 mm nozzle / PLA starting point, not physically qualified.
   Servo mounts and stock horn patterns require a printed fit gauge.
   Position servos inserted after removing all sacrificial scaffolding.
*/
$fn=64;
eps=0.02;
function profile(k="micro") = k=="premium" ? [23,12,31.754,5.57,28,32] : k=="standard" ? [40.7,19.7,42.9,10.5,47.6,54] : k=="strong" ? [22.8,12,31,6,31,35] : [23,12,30,6,29,33];
module rounded_box(s,r=3){translate([r,r,0])linear_extrude(s[2])offset(r=r)square([s[0]-2*r,s[1]-2*r]);}
module slot(a,b,d,h){hull(){translate(a)cylinder(d=d,h=h);translate(b)cylinder(d=d,h=h);}}
module radial_slots(h=6){for(a=[0,90,180,270])rotate([0,0,a])slot([8,0,-eps],[13.5,0,-eps],2,h);}
function flange_drop(k)=k=="standard"?16.3:k=="premium"?12.354:16;
module servo_mount(k="micro"){
 p=profile(k);c=p[0]/2-p[3];span=k=="standard"?64:38;fd=flange_drop(k);sy=k=="standard"?30:20;
 difference(){
  translate([k=="standard"?-18:c-span/2,-sy/2,-fd-3])cube([k=="standard"?c+span/2+18:span,sy,3]);
  translate([-p[3]-.55,-p[1]/2-.55,-fd-4])cube([p[0]+1.1,p[1]+1.1,6]);
  translate([-p[3]-.55,0,-fd-4])cube([p[0]+1.1,sy,6]);
  for(x=[c-p[4]/2,c+p[4]/2],y=k=="standard"?[-5,5]:k=="premium"?[-3.5,3.5]:[0])if(k=="standard")slot([x-1.5,y,-fd-4],[x+1.5,y,-fd-4],3.6,6);else if(k=="premium")slot([x-.7,y,-fd-4],[x+.7,y,-fd-4],2.6,6);else translate([x,y,-fd-4])cylinder(d=2.8,h=6);
 }
}
module servo_mesh(k="micro"){
 p=profile(k);c=p[0]/2-p[3];fd=flange_drop(k);
 translate([-p[3],-p[1]/2,-p[2]])cube([p[0],p[1],p[2]-7.5]);
 translate([c-p[5]/2,-p[1]/2,-fd])cube([p[5],p[1],2]);
 translate([0,0,-7.5])cylinder(d=12,h=3.6);translate([0,0,-4])cylinder(d=5,h=4);
}
module stock_horn(){difference(){union(){cylinder(d=8,h=2);hull(){translate([0,-14,0])cylinder(d=5,h=2);translate([0,14,0])cylinder(d=5,h=2);}}cylinder(d=2,h=4,center=true);for(y=[-12,-8,8,12])translate([0,y,-eps])cylinder(d=1.7,h=3);}}
module horn_plate(){difference(){cylinder(r=15,h=3);cylinder(d=4,h=8,center=true);radial_slots();}}
module shaft_z(r=7,lip=8.8,len=10.2){rotate_extrude()polygon([[0,-len/2],[r,-len/2],[r,-1.8],[lip,0],[r,1.8],[r,len/2],[0,len/2]]);}
module bore_z(r=7,lip=8.8,len=9,g=.45){rotate_extrude()polygon([[0,-len/2-1],[r+g,-len/2-1],[r+g,-1.8-g],[lip+g,0],[r+g,1.8+g],[r+g,len/2+1],[0,len/2+1]]);}
module socket_z(r=7,lip=8.8,len=9,outer=12.5,g=.45){difference(){translate([0,0,-len/2])cylinder(r=outer,h=len);bore_z(r,lip,len,g);}}
module shaft_x(r=7,lip=8.8,len=10.2){rotate([0,90,0])shaft_z(r,lip,len);}
module socket_x(r=7,lip=8.8,len=9,outer=12.5,g=.45){rotate([0,90,0])socket_z(r,lip,len,outer,g);}
module bore_x(r=7,lip=8.8,len=9,g=.45){rotate([0,90,0])bore_z(r,lip,len,g);}
module yaw_profile(g=0,r=27){rotate_extrude()polygon([[0,4.6-g],[r+g,4.6-g],[r+g,9-g],[r+2+g,11],[r+g,13+g],[r+g,39+g],[0,39+g]]);}
module turn_base(g=.45,k="micro",service=false,r=27){
 p=profile(k);c=p[0]/2-p[3];
 difference(){
  union(){translate([-49,-46,0])rounded_box([98,92,4],8);translate([0,0,4])cylinder(r=r+6.5,h=16);}
  yaw_profile(g,r);translate([0,0,3.9])cylinder(r=r-3-g,h=24);
  translate([-p[3]-.5,-p[1]/2-.5,-eps])cube([p[0]+1,p[1]+1,6]);
  for(a=[45,135,225,315])rotate([0,0,a])translate([r-3,-7,4])cube([14,14,4]);
  for(x=[-40,40],y=[-37,37])translate([x,y,-eps])cylinder(d=3.4,h=6);
  if(service)translate([c-18,7,7])cube([36,45,34]);
 }
 intersection(){translate([0,0,38-g])servo_mount(k);translate([0,0,18-g])cylinder(r=23.5,h=5);}
 for(x=[c-p[4]/2,c+p[4]/2])difference(){translate([x,0,3.8])cylinder(r=3,h=15.3-g);translate([x,0,3])cylinder(d=1.8,h=25);}
}
module turn_ring(g=.45,k="micro",service=false,r=27){
 p=profile(k);c=p[0]/2-p[3];
 difference(){
  union(){difference(){yaw_profile(0,r);translate([0,0,4])cylinder(r=r-3,h=38);}difference(){translate([0,0,38.5])cylinder(r=r+4,h=5.5);translate([0,0,38])cylinder(r=r-3,h=3);}}
  translate([c-(p[5]+1.4)/2,-6.8,38])cube([p[5]+1.4,13.6,8]);translate([0,0,38])radial_slots(8);
  if(service)translate([c-18,7,7])cube([36,45,34]);
 }
}
module fixed_fork(z=62,x=28,g=.45){difference(){union(){for(s=[-1,1]){translate([s*x-3,-15,42])cube([6,30,z-42]);translate([s*x,0,z])socket_x(g=g);}}for(s=[-1,1])translate([s*x,0,z])bore_x(g=g);}}
module axles(z=62,x=28,plate_x=-36){for(s=[-1,1])translate([s*x,0,z])shaft_x();translate([plate_x,0,z])rotate([0,90,0])horn_plate();}
module starter_post(x,y,z1,z0=0){translate([x-2,y-2,z0])cube([4,4,z1-z0-.6]);translate([x-.35,y-.35,z1-.6])cube([.7,.7,.65]);}
module starter_wall(x,y,z1,length=10,angle=0,z0=0){translate([x,y,z0])rotate([0,0,angle])translate([-.3,-length/2,0])cube([.6,length,z1-z0+.05]);}
module turn_starters(r=27){difference(){translate([0,0,3.8])difference(){cylinder(r=r-1.4,h=.85);cylinder(r=r-2,h=1);}for(a=[0:15:345])rotate([0,0,a])translate([r-2.2,-.25,3.7])cube([1,.5,1.2]);}}
module pivot_ties(z,x){for(s=[-1,1])translate([s*x-.35,-.35,z-9.35])cube([.7,.7,.70]);}
module fit_gauge(k="micro") {translate([24,16,flange_drop(k)+3])servo_mount(k);translate([108,15,0])horn_plate();p=profile(k);translate([0,40,0])difference(){cube([profile(k)[0]+13,profile(k)[1]+8,5]);translate([4,3,-1])cube([p[0]+1.1,p[1]+1.1,8]);}}
module about(P,a,axis=[1,0,0]){translate(P)rotate(a,axis)translate(-P)children();}

// Case clearances stop at the mounting face, so purchased ears can seat on PLA.
module servo_clearance(k="micro",c=.4){p=profile(k);m=p[0]/2-p[3];fd=flange_drop(k);
 translate([-p[3]-c,-p[1]/2-c,-p[2]-c])cube([p[0]+2*c,p[1]+2*c,p[2]-7.5+c]);
 translate([m-p[5]/2-c,-p[1]/2-c,-fd])cube([p[5]+2*c,p[1]+2*c,2+c]);
 translate([0,0,-7.5])cylinder(r=6+c,h=3.6+c);translate([0,0,-4])cylinder(r=2.5+c,h=4.1);
}
