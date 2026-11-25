/* Swan: turn, two neck bends and opening beak. Grip pads are purchased stick-on rubber.
   Exploratory print-in-place CAD; sock handling is a test target, not demonstrated. */
include <servo-toy-library.scad>
part="preview";gap=.45;turn=0;lower=0;upper=0;beak=0;P=[0,0,62];U=[0,0,110];J=[0,0,145];
module base(){turn_base(gap,"strong",false,40);}
module carrier(){difference(){union(){turn_ring(gap,"strong",false,40);fixed_fork(62,28,gap);translate([-70,-17,38.5])cube([101,34,5.5]);translate([-39,0,62])rotate([0,90,0])servo_mount("standard");for(y=[-9,9])translate([-58,y-2,35.6])cube([33.1,4,5]);}translate([-33,22,21])cube([66,27,25]);translate([0,0,38])radial_slots(8);translate([0,0,38])cylinder(r=24,h=3);translate([-39,0,62])rotate([0,90,0])servo_clearance("standard");translate([-12.8,-6.8,38])cube([36.4,13.6,8]);for(x=[-28,28])translate([x,0,62])bore_x(g=gap);}}
module tube(z0,L,curve,r=12){difference(){for(i=[0:11])hull(){for(t=[i/12,(i+1)/12])translate([0,curve*sin(180*t),z0+L*t])cylinder(r=r,h=1);}for(i=[0:11])hull(){for(t=[i/12,(i+1)/12])translate([0,curve*sin(180*t),z0-.1+L*t])cylinder(r=r-3,h=1.3);}}}
module lowerneck(){difference(){union(){axles(62,28,-36);translate([-23,-10,59])cube([46,20,6]);tube(62,30,-10);
 for(s=[-1,1]){hull(){translate([s*8-3,-10,83])cube([6,20,4]);translate([s*26-3,-10,99])cube([6,20,4]);}translate([s*26-3,-10,99])cube([6,20,11]);translate([s*26,0,110])socket_x(g=gap);}
 translate([-37,0,110])rotate([0,90,0])servo_mount("strong");for(y=[-9,9])translate([-56,y-2,85.6])cube([40,4,4]);for(y=[-9,9])translate([-18,y-2,85.6])cube([12,4,6]);
 }for(s=[-1,1])translate([s*26,0,110])bore_x(g=gap);translate([-44,0,62])rotate([0,90,0])cylinder(d=4,h=18);translate([-40,-13,118.5])cube([19,26,5]);}}
module upperneck(){translate([0,0,3])difference(){union(){axles(107,26,-34);translate([-21,-10,104])cube([42,20,6]);tube(107,18,9,10);
 for(s=[-1,1]){hull(){translate([s*7-3,-8,120])cube([6,16,4]);translate([s*20-3,-8,131])cube([6,16,4]);}translate([s*20-3,-8,131])cube([6,16,11]);translate([s*20,0,142])socket_x(g=gap);}
 translate([-31,0,142])rotate([0,90,0])servo_mount("strong");for(y=[-9,9])translate([-50,y-2,121.5])cube([44,4,4]);
 // Hollow upper head and fixed upper beak, raised above the moving lower jaw.
 difference(){translate([0,0,154])scale([1,1.5,.8])sphere(r=15);translate([0,0,153])scale([1,1.5,.8])sphere(r=12);translate([-25,-25,125])cube([50,50,22]);}
 for(x=[-20,17])translate([x,-5,141])cube([3,10,13]);
 for(x=[-20,9])translate([x,-5,148])cube([11,10,4]);translate([0,0,147])linear_extrude(3)polygon([[-10,4],[10,4],[7,26],[-7,26]]);
 }for(s=[-1,1])translate([s*20,0,142])bore_x(g=gap);translate([-42,0,107])rotate([0,90,0])cylinder(d=4,h=18);}}
module lowerbeak(){translate([0,0,3])difference(){union(){axles(142,20,-28);translate([-15,-6,138])cube([30,12,4]);translate([0,0,138])linear_extrude(3)polygon([[-9,5],[9,5],[6,26],[-6,26]]);}translate([-37,0,142])rotate([0,90,0])cylinder(d=4,h=20);}}
module starters(){turn_starters(40);for(y=[-12,12])starter_post(27,y,38.5);starter_post(-56.8,0,73.05);for(y=[-12,12])starter_post(-56.8,y,20.15);starter_wall(0,0,59,46,90);pivot_ties(62,28);pivot_ties(110,26);pivot_ties(145,20);
 for(y=[-9,9]){starter_post(-56.5,y,35.6);starter_post(-54.5,y,85.6);starter_post(-48.5,y,120.6);}
 starter_wall(0,0,107,42,90);starter_wall(0,0,141,30,90);starter_wall(0,16,150,18,90);starter_wall(0,16,141,16,90);
 starter_post(-34.5,0,47);starter_post(-32.5,0,95);starter_post(-26.5,0,130);
}
module sy0(){translate([0,0,38-gap])servo_mesh("strong");}module sy1(){translate([-39,0,62])rotate([0,90,0])servo_mesh("standard");}module sy2(){translate([-37,0,110])rotate([0,90,0])servo_mesh("strong");}module sy3(){translate([-31,0,145])rotate([0,90,0])servo_mesh("strong");}
module h0(){translate([0,0,39])stock_horn();}module h1(){translate([-38,0,62])rotate([0,90,0])stock_horn();}module h2(){translate([-36,0,110])rotate([0,90,0])stock_horn();}module h3(){translate([-30,0,145])rotate([0,90,0])stock_horn();}
module assembly(pre=false){base();if(pre)sy0();translate([0,0,pre?-gap:0])rotate([0,0,turn]){carrier();if(pre){sy1();h0();}about(P,lower){lowerneck();if(pre){sy2();h1();}about(U,upper){upperneck();if(pre){sy3();h2();}about(J,beak){lowerbeak();if(pre)h3();}}}}}
if(part=="preview")assembly(true);else if(part=="mechanism"){assembly();starters();}
else if(part=="base")base();else if(part=="carrier")carrier();else if(part=="lower")lowerneck();else if(part=="upper")upperneck();else if(part=="beak")lowerbeak();else if(part=="starters")starters();
else if(part=="servo0")sy0();else if(part=="servo1")sy1();else if(part=="servo2")sy2();else if(part=="servo3")sy3();else if(part=="horn0")h0();else if(part=="horn1")h1();else if(part=="horn2")h2();else if(part=="horn3")h3();
