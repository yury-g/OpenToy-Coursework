/* Compact elbow, wrist and one finger. Anchor base before pressing/pushing. Exploratory draft. */
include <servo-toy-library.scad>
part="preview";gap=.45;elbow=0;wrist=0;finger=0;P=[0,0,42];W=[0,0,97];F=[0,0,132];
module base(){difference(){union(){translate([-85,-50,0])rounded_box([145,100,4],8);for(s=[-1,1]){translate([s*28-3,-15,4])cube([6,30,38]);translate([s*28,0,42])socket_x(g=gap);}translate([-39,0,42])rotate([0,90,0])servo_mount("standard");for(y=[-13,13])translate([-69,y-2,9])cube([44.1,4,5]);}for(s=[-1,1])translate([s*28,0,42])bore_x(g=gap);translate([-39,0,42])rotate([0,90,0])servo_clearance("standard");for(x=[-50,50],y=[-40,40])translate([x,y,-1])cylinder(d=4,h=7);}}
module forearm(){difference(){union(){axles(42,28,-36);translate([-23,-10,39])cube([46,20,6]);
 for(s=[-1,1]){translate([s*18-3,-10,42])cube([6,20,38]);hull(){translate([s*18-3,-10,76])cube([6,20,4]);translate([s*26-3,-10,87])cube([6,20,4]);}translate([s*26-3,-10,87])cube([6,20,10]);translate([s*26,0,97])socket_x(g=gap);}
 translate([-37,0,97])rotate([0,90,0])servo_mount("strong");for(y=[-9,9])translate([-56,y-2,72.5])cube([40,4,4]);
 }for(s=[-1,1])translate([s*26,0,97])bore_x(g=gap);translate([-44,0,42])rotate([0,90,0])cylinder(d=4,h=18);}}
module hand(){difference(){union(){axles(97,26,-34);translate([-21,-10,94])cube([42,20,6]);
 for(s=[-1,1]){translate([s*12-3,-10,97])cube([6,20,18]);hull(){translate([s*12-3,-10,111])cube([6,20,4]);translate([s*20-3,-10,121])cube([6,20,4]);}translate([s*20-3,-10,121])cube([6,20,11]);translate([s*20,0,132])socket_x(g=gap);}
 translate([-31,0,132])rotate([0,90,0])servo_mount();for(y=[-9,9])translate([-50,y-2,107.5])cube([39,4,4]);
 }for(s=[-1,1])translate([s*20,0,132])bore_x(g=gap);translate([-42,0,97])rotate([0,90,0])cylinder(d=4,h=18);}}
module fingertip(){difference(){union(){axles(132,20,-28);translate([-15,-8,129])cube([30,16,6]);translate([-6,-6,132])cube([12,12,21]);translate([0,0,153])sphere(r=6);}translate([-37,0,132])rotate([0,90,0])cylinder(d=4,h=20);}}
module starters(){starter_post(-56.8,0,53.05);starter_wall(0,0,39,46,90);pivot_ties(42,28);pivot_ties(97,26);pivot_ties(132,20);
 for(y=[-9,9]){starter_post(-54.5,y,72.6);starter_post(-48.5,y,107.5);}
 starter_wall(0,0,94,42,90);starter_wall(0,0,129,30,90);
 starter_post(-34.5,0,27);starter_post(-32.5,0,82);starter_post(-26.5,0,117);
}
module sy0(){translate([-39,0,42])rotate([0,90,0])servo_mesh("standard");}module sy1(){translate([-37,0,97])rotate([0,90,0])servo_mesh("strong");}module sy2(){translate([-31,0,132])rotate([0,90,0])servo_mesh();}
module h0(){translate([-38,0,42])rotate([0,90,0])stock_horn();}module h1(){translate([-36,0,97])rotate([0,90,0])stock_horn();}module h2(){translate([-30,0,132])rotate([0,90,0])stock_horn();}
module assembly(pre=false){base();if(pre)sy0();about(P,elbow){forearm();if(pre){sy1();h0();}about(W,wrist){hand();if(pre){sy2();h1();}about(F,finger){fingertip();if(pre)h2();}}}}
if(part=="preview")assembly(true);else if(part=="mechanism"){assembly();starters();}
else if(part=="base")base();else if(part=="forearm")forearm();else if(part=="hand")hand();else if(part=="finger")fingertip();else if(part=="starters")starters();
else if(part=="servo0")sy0();else if(part=="servo1")sy1();else if(part=="servo2")sy2();else if(part=="horn0")h0();else if(part=="horn1")h1();else if(part=="horn2")h2();
