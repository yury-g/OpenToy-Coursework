/* PLANT v0.2 — bench-assembled, captive rotary modules. mm.
   Physical fit/release unqualified. Three identical joint prints.
   No printed splines. Fit the exact purchased horn before classroom use.
   Motor-side UP for joint printing; NEVER support the annular gap.
*/
$fn=72;
part="assembly";
gap=0.50; // radial clearance per side; sloped capture also permits axial play
mount_z=32.4; // provisional output-to-ear stack; calibrate on full joint coupon
turn=0; nod=0; roll=0;
eps=.02;
module slot(a,b,d,h){hull(){translate(a)cylinder(d=d,h=h);translate(b)cylinder(d=d,h=h);}}
module hexnut(h=2.8){cylinder(d=6.55,h=h,$fn=6);}
module rotor_profile(g=0){rotate_extrude()polygon([[0,0],[21+g,0],[21+g,.4],[19+g,2.8],[19+g,14],[21+g,16.4],[21+g,18+g],[0,18+g]]);}
module rotor(){difference(){
 rotor_profile();
 // Horn recess opens to motor side. No unsupported recess roof.
 translate([0,0,15.4])cylinder(r=17.3,h=4);
 translate([0,0,-1])cylinder(d=5,h=21);
 for(s=[-1,1]){slot([0,s*8,-1],[0,s*13.5,-1],2.3,21);translate([0,s*10,-1])cylinder(d=5.2,h=13.4);}
 for(x=[-8,8],y=[-9,9])translate([x,y,-1])cylinder(d=9.6,h=20);
 for(x=[-11,11]){translate([x,0,-1])cylinder(d=3.3,h=21);translate([x,0,12.6])hexnut(3.2);}
 // First-layer relief: prevents the bed squish closing the running gap.
 difference(){cylinder(r=22,h=.3);cylinder(r1=20.7,r2=21,h=.3+eps);}
}}
module stator(){difference(){union(){
 difference(){cylinder(r=24.5,h=18);rotor_profile(gap);}
 for(y=[-32,32])translate([-8,y-10,0])cube([16,20,18]);
 for(x=[-24,24])translate([x,0,0])cylinder(r=3,h=18);
// Buttresses grow inward above the moving disk.
 hull(){translate([-27,-8,14])cube([4,16,.4]);translate([-13,-8,mount_z-3])cube([8,16,3]);}
 hull(){translate([23,-8,14])cube([4,16,.4]);translate([18,-8,mount_z-3])cube([9,16,3]);}
 }
 rotor_profile(gap);
// Exact case envelope remains a fit assumption; verify the purchased batch.
 translate([-6.6,-6.6,25.3])cube([24.2,13.2,40]);
 for(x=[-10.35,21.15])translate([x,0,mount_z-5])cylinder(d=1.8,h=8);
 for(y=[-32,32])translate([0,y,-1])cylinder(d=3.3,h=20);
 // Bed relief on the cavity, independent of slicer compensation.
 translate([0,0,-eps])cylinder(r1=21+gap+.3,r2=21+gap,h=.32);
}}
module joint(){stator();rotor();}
module motor(){ // representative MG92B envelope / provisional ears, NOT a certified CAD model
 translate([0,0,mount_z-14])rotate([180,0,0]){
 translate([-6,-6,-31])cube([22.8,12,23.5]);
 translate([-12.1,-6,-16])cube([35,12,2]);
 translate([0,0,-7.5])cylinder(d=12,h=3.5);translate([0,0,-4])cylinder(d=5,h=4);
 }}
module horn(){translate([0,0,17.4])rotate([180,0,0])difference(){union(){cylinder(d=8,h=2);slot([0,-14,0],[0,14,0],5,2);}cylinder(d=2,h=4,center=true);}}
module base(){difference(){union(){
 translate([-42,-42,0])linear_extrude(4)offset(r=8)square([84,84]);
 for(y=[-32,32]){translate([-8,y-7,4])cube([16,14,36]);
 hull(){translate([-20,y-9,4])cube([40,18,.5]);translate([-8,y-7,30])cube([16,14,.5]);}}
 }for(y=[-32,32]){translate([0,y,27])cylinder(d=3.3,h=15);translate([0,y,34])hexnut(2.8);
 translate([-3.4,y-9,34])cube([6.8,9,2.8]);}
 for(x=[-40,40],y=[-40,40])translate([x,y,-1])cylinder(d=3.4,h=6);
}}
module stalk(){difference(){union(){
 translate([0,0,58])cylinder(r=17,h=4);
 hull(){translate([8,-6,61])cube([8,12,.6]);translate([18,-6,72])cube([6,12,.6]);}
 translate([18,-6,62])cube([6,12,46]);
 hull(){translate([18,-6,107])cube([6,12,1]);translate([18,-38,144])cube([6,76,1]);}
 translate([18,-38,144])cube([6,76,12]);
 // Rounded leaves join the stem; their root slope is supported.
 for(s=[-1,1])hull(){translate([18,s*4,83])cube([6,5,5]);translate([18,s*18-2.5,99])cube([6,5,5]);}
 }
 for(x=[-11,11])translate([x,0,57])cylinder(d=3.3,h=7);
 translate([0,0,57])cylinder(d=6,h=7);
 hull(){translate([17,-2,107])cube([9,4,1]);translate([17,-24,144])cube([9,48,1]);} translate([17,-24,144])cube([9,48,40]);
for(y=[-32,32]){translate([17,y,150])rotate([0,90,0])cylinder(d=3.3,h=9);translate([21.2,y,150])rotate([0,90,0])hexnut(3.2);}
 }}
module elbow(){difference(){union(){
 translate([-4,-17,133])cube([4,34,34]);
 // Rectangular rails avoid difficult diagonal support interfaces.
 for(z=[133,163]){
  translate([-35,-3,z])cube([35,6,4]);
  translate([-39,-3,z])cube([12,17,4]);
 }
 translate([-39,8,111])cube([8,6,78]);
 for(z=[118,182])translate([-43,8,z-7])cube([16,6,14]);
 }
 translate([-44,7,142])cube([18,8,16]);
 for(z=[139,161])translate([-5,-1.7,z-1.7])cube([7,3.4,3.4]);
 translate([-5,-3,147])cube([7,6,6]);
 for(z=[118,182]){translate([-35,7,z])rotate([-90,0,0])cylinder(d=3.3,h=9);translate([-35,7.9,z])rotate([-90,0,0])hexnut(2.9);}
 }}
module flower(){difference(){union(){
 linear_extrude(2.0)union(){offset(r=7)square([56,32],center=true);
 for(a=[0:60:300])rotate(a)hull(){translate([0,24])circle(r=8);translate([0,42])circle(r=8);}}
 translate([0,0,1.9])cylinder(r=17,h=14.1);
 // Decorative center rings are printed, with no screen/sensor implication.
 }for(x=[-8,8],y=[-9,9])translate([x,y,2.0])cylinder(d=9.6,h=15);
 for(x=[-11,11])translate([x,0,-1])cylinder(d=3.3,h=19);
 translate([0,0,-1])cylinder(d=6,h=19);
 }}
module washer(t=.4){difference(){cylinder(d=8,h=t);cylinder(d=2.4,h=t+1);}}
module gauges(){
 // Same capture profile, shorter plain housing: .40 / .50 / .60 per side.
 for(i=[0:2])translate([i*59,0,0]){
 difference(){cylinder(r=24.5,h=18);rotor_profile([.4,.5,.6][i]);}
 rotor();translate([-10,-34,0])cube([20,12,2]);
 translate([-8,-32,2])linear_extrude(.4)text(str([40,50,60][i]),size=4);
 }
}
module ear_shims(){for(i=[0:2])translate([i*18,0,0])difference(){cube([12,12,[.4,.8,1.2][i]]);translate([6,6,-1])cylinder(d=2.6,h=4);}}
module j0(){translate([0,0,58])rotate([180,0,0])children();}
module j1(){translate([0,0,150])rotate([0,90,0])children();}
module j2(){translate([-35,32,150])rotate([90,0,0])children();}
module a1(){translate([0,0,150])rotate([nod,0,0])translate([0,0,-150])children();}
module a2(){translate([-35,32,150])rotate([0,roll,0])translate([35,-32,-150])children();}
module assembly(){color("Sienna")base();
 j0(){color("DarkGreen")stator();color("DimGray")motor();}
 rotate([0,0,turn]){
 j0(){color("OliveDrab")rotor();color("Wheat")horn();}
 color("ForestGreen")stalk();
 j1(){color("DarkGreen")stator();color("DimGray")motor();}
 a1(){j1(){color("OliveDrab")rotor();color("Wheat")horn();}color("SeaGreen")elbow();
 j2(){color("DarkGreen")stator();color("DimGray")motor();}
 a2(){j2(){color("OliveDrab")rotor();color("Wheat")horn();color("Gold")translate([0,0,-16])flower();}}}}
}
// Assembly members are exposed separately for independent mesh/physics review.
if(part=="joint")joint();
else if(part=="stator")stator();else if(part=="rotor")rotor();
else if(part=="motor")motor();else if(part=="horn")horn();
else if(part=="base")base();else if(part=="stalk")translate([0,0,24])rotate([0,90,0])translate([0,0,-58])stalk();
else if(part=="stalk_raw")translate([0,0,-58])stalk();
else if(part=="elbow")translate([0,0,17])rotate([-90,0,0])translate([0,0,-111])elbow();
else if(part=="elbow_raw")translate([0,0,-111])elbow();
else if(part=="flower")flower();else if(part=="gauges")gauges();else if(part=="shims")ear_shims();
else assembly();
