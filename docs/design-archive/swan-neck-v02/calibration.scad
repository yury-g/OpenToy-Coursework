/* Print fit gauges before the full swan. All dimensions mm. */
include <servo-toy-library.scad>
part="clearance";
module joint(g){translate([0,0,12.5])difference(){union(){socket_x(g=g);translate([-4.5,-12.5,-12.5])cube([9,25,12.5]);}bore_x(g=g);}translate([0,0,12.5])shaft_x();for(s=[-1,1])translate([s*5.1,0,12.5])rotate([0,s*90,0])cylinder(r=5,h=14);translate([-.35,-.35,3.15])cube([.7,.7,.7]);}
if(part=="clearance")for(i=[0:3])translate([i*48+24,17,0])joint([.3,.4,.45,.5][i]);
else if(part=="MG996R")fit_gauge("standard");else if(part=="MG92B")fit_gauge("strong");else if(part=="SER0039")fit_gauge("micro");else if(part=="A20-family")fit_gauge("premium");
