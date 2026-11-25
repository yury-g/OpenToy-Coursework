/* Giraffe: captive turn, bending hollow neck, opening mouth. Exploratory CAD.
   part preview / mechanism / base / carrier / neck / jaw / starters / servo0..2 / horn0..2.
   Horn and case envelopes approximate; use gauges before fitting motors. */
include <servo-toy-library.scad>
part="preview";gap=.45;turn=0;bend=0;mouth=0;
P=[0,0,62];J=[0,20,160];
module base(){turn_base(gap,"strong");}
module carrier(){difference(){union(){turn_ring(gap,"strong");fixed_fork(62,28,gap);translate([-58,-15,38.5])cube([89,30,5.5]);translate([-39,0,62])rotate([0,90,0])servo_mount("strong");for(y=[-9,9])translate([-58,y-2,44])cube([33.1,4,4]);}translate([0,0,38])radial_slots(8);translate([0,0,38])cylinder(r=24,h=3);translate([-39,0,62])rotate([0,90,0])servo_clearance("strong");translate([-12.8,-6.8,38])cube([36.4,13.6,8]);for(x=[-28,28])translate([x,0,62])bore_x(g=gap);}}
module neck_shell(){for(i=[0:7])hull(){for(t=[i/8,(i+1)/8])translate([0,10*t,62+85*t])cylinder(r=12-3*t,h=1);} }
module neck_hollow(){for(i=[0:7])hull(){for(t=[i/8,(i+1)/8])translate([0,10*t,61.9+85*t])cylinder(r=9-3*t,h=1.3);} }
module neck(){difference(){union(){
 axles(62,28,-36);translate([-23,-10,59])cube([46,20,6]);difference(){neck_shell();neck_hollow();}
 translate([-31,-2,153])cube([62,15,3]);for(x=[-31,28])translate([x,-2,153])cube([3,64,33]);
 translate([-31,-2,183])cube([62,64,3]);translate([-31,-2,153])cube([62,3,33]);
 translate([-31,30,169])cube([62,32,3]);translate([-31,59,169])cube([62,3,17]);
 translate([0,10,145])difference(){cylinder(r=9,h=10);cylinder(r=6,h=11);}
 for(x=[-28,28])translate([x,20,160])socket_x(g=gap);
 translate([-39,20,160])rotate([0,90,0])servo_mount();
 for(y=[11,29]){translate([-58,y-2,135.5])cube([30,4,4]);translate([-31,y-2,135.5])cube([3,4,20.5]);}
 for(x=[-11,11]){translate([x,8,185])cylinder(r=2,h=9);translate([x,8,194])sphere(r=3);}
 for(s=[-1,1])hull(){translate([s*30,3,181])sphere(r=3);translate([s*38,0,184])scale([1,.5,.55])sphere(r=5);}
 for(s=[-1,1])translate([s*31,34,179])scale([.45,1,1])sphere(r=4);
 }translate([-44,0,62])rotate([0,90,0])cylinder(d=4,h=18);
 for(x=[-28,28])translate([x,20,160])bore_x(g=gap);
 translate([-39,20,160])rotate([0,90,0])servo_clearance();
 }}
module jaw(){difference(){union(){
 for(x=[-28,28])translate([x,20,160])shaft_x();
 translate([-36,20,160])rotate([0,90,0])horn_plate();
 translate([-23,14,157])cube([46,12,6]);
 for(x=[-22,22])translate([x-1.5,18,155])cube([3,44,4]);
 translate([-23.5,30,155])cube([47,32,4]);
 }translate([-44,20,160])rotate([0,90,0])cylinder(d=4,h=18);}}
module starters(){turn_starters();starter_post(-34.5,0,47);for(x=[-28,28])starter_post(x,20,147.5);starter_wall(0,0,59,46,90);pivot_ties(62,28);
 for(y=[-9,9])starter_post(-56.5,y,37.6);
 starter_wall(0,60,169,60,90);starter_wall(0,10,153,60,90);
 starter_wall(0,45,155,44,90);starter_post(-34.5,20,145);starter_wall(0,20,157,44,90);translate([0,20,0])pivot_ties(160,28);for(y=[11,29])starter_post(-56.5,y,135.5);
 
 }
module sy0(){translate([0,0,38-gap])servo_mesh("strong");}module sy1(){translate([-39,0,62])rotate([0,90,0])servo_mesh("strong");}module sy2(){translate([-39,20,160])rotate([0,90,0])servo_mesh();}
module h0(){translate([0,0,39])stock_horn();}module h1(){translate([-38,0,62])rotate([0,90,0])stock_horn();}module h2(){translate([-38,20,160])rotate([0,90,0])stock_horn();}
module assembly(pre=false){color("#C2CDD2")base();color("#303A40")if(pre)sy0();translate([0,0,pre?-gap:0])rotate([0,0,turn]){color("#D6A449")carrier();color("#303A40")if(pre)sy1();color("#D6A449")if(pre)h0();about(P,bend){color("#E4BC66")neck();color("#303A40")if(pre)sy2();color("#D6A449")if(pre)h1();about(J,mouth){color("#A87541")jaw();color("#D6A449")if(pre)h2();}}}}
if(part=="preview")assembly(true);else if(part=="mechanism"){assembly();starters();}
else if(part=="base")base();else if(part=="carrier")carrier();else if(part=="neck")neck();else if(part=="jaw")jaw();else if(part=="starters")starters();
else if(part=="servo0")sy0();else if(part=="servo1")sy1();else if(part=="servo2")sy2();else if(part=="horn0")h0();else if(part=="horn1")h1();else if(part=="horn2")h2();
