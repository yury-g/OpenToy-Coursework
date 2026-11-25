// Exact preferred A bearing and feet, copied without geometric changes.
gap=0.4; // normal clearance, use 0.5 if the first print fuses
$fn=64;
axis_height=4.2;
radius=3.35;
cone_depth=2.85;
axial_gap=0.5;
// Hinge axis is Y. The cheeks and socket feet all touch the bed.
module cone(y,d,clearance=0){
    translate([0,y,axis_height]) rotate([-90*d,0,0])
        cylinder(h=cone_depth+clearance*sqrt(2),
        r1=radius+clearance*sqrt(2),r2=radius-cone_depth);
}
// Rounded footprints and a shrinking upper fillet keep the lower face flat.
module rounded_xy(x0,x1,y0,y1,r){
 translate([x0+r,y0+r])offset(r=r)square([x1-x0-2*r,y1-y0-2*r]);
}
module foot(x0,x1,y0,y1,h,r=1.2){
 edge=0.6;
 union(){
  linear_extrude(h-edge)rounded_xy(x0,x1,y0,y1,r);
  translate([0,0,h-edge-0.01])minkowski(){
   linear_extrude(.01)rounded_xy(x0+edge,x1-edge,y0+edge,y1-edge,r-edge);
   sphere(r=edge,$fn=24);
  }
 }
}
module left_unmarked(){
    union(){
        foot(-26,-7,-13,13,3);
        // Each cheek connects to its own broad foot.
        for(d=[-1,1]){
            foot(-12,4.4,d<0?-13:8.5,d<0?-8.5:13,axis_height+radius+1);
            cone(d*8.5,-d);
        }
    }
}
module right_untrimmed(){
    difference(){
        union(){
            foot(7,26,-8,8,3);
            foot(0,12,-8,8,3);
            // Round top, bed-supported lower half, like the earlier stand.
            foot(-4.3,4.3,-8,8,axis_height);
            translate([0,-8,axis_height]) rotate([-90,0,0])
                cylinder(h=16,r=4.3);
        }
        for(d=[-1,1]) cone(d*8.5,-d,gap);
    }
}
// Trim the barrel's 0.10 mm protrusion so both flat feet seat on Z=0.
module right(){
 intersection(){right_untrimmed();translate([-40,-20,0])cube([80,40,30]);}
}
module left(){
 difference(){
  left_unmarked();
  translate([-17,0,2.35])linear_extrude(0.8)
   text("A",size=7,font="Liberation Sans:style=Bold",halign="center",valign="center");
 }
}
