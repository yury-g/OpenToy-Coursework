/* Print these before any large draft. Gap is per radial side, units mm. */
include <servo-toy-library.scad>
part="clearance";
module unit(g){
 translate([0,0,13])socket_x(g=g);translate([0,0,13])shaft_x();
 for(x=[-5,5])translate([x-3,-13,0])cube([6,26,2]);
 translate([-.35,-.35,13-8.8-g-.05])cube([.7,.7,g+.1]);
 translate([4.9,0,13])rotate([0,90,0])cylinder(r=4,h=4.1);translate([6,-1,11])cube([3,26,4]);translate([6,23,9])cube([3,6,8]);starter_post(7.5,25,9);
}
if(part=="clearance")for(i=[0:3])translate([0,i*49,0]){unit([.30,.40,.45,.50][i]);translate([-20,-13,0])cube([12.1,10,2]);translate([-19,-11,2])linear_extrude(.4)text(["0.30","0.40","0.45","0.50"][i],size=3.2,font="Arial");}
else if(part=="micro")fit_gauge("micro");else if(part=="strong")fit_gauge("strong");else if(part=="standard")fit_gauge("standard");
