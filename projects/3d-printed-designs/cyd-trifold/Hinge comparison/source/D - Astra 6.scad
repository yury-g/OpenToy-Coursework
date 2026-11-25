// D: Outward-cone blind-socket hinge coupon. Units mm.
// Print assembled at Z=0. Separate exports are inspection only, no assembly.
// 180 degree upward fold: rotate right about Y by -180 around Z=5.
part="assembly"; // assembly,left,right
$fn=96;
axis_z=5;
r=3.6;
normal_gap=0.45;
end_gap=0.60;
plate_t=2.8;
module box(a,b){translate(a) cube(b-a);}
// Two local pointed conical bearings, no through pin.
module cones(clear=0){
 for(s=[-1,1]) translate([0,s*6,axis_z]) rotate([-90*s,0,0])
 cylinder(h=r+clear*sqrt(2),r1=r+clear*sqrt(2),r2=0);
}
module left_raw(){
 difference(){
 union(){
 box([-28,-15,0],[-7,15,plate_t]);
 for(s=[-1,1]){
  y0=s<0?-12.6:6+end_gap;
  y1=s<0?-6-end_gap:12.6;
  box([-10,y0,0],[0,y1,plate_t]);
  box([-5,y0,0],[5,y1,10.4]);
 }
 }
 cones(normal_gap);
 // Wide engraved label, 0.6 mm deep, away from bearing.
 translate([-21,-4,plate_t-0.6]) linear_extrude(0.8)
 text("D",size=8,font="Liberation Sans:style=Bold");
 }
}
module right_raw(){
 union(){
 box([10,-15,0],[28,15,plate_t]);
 box([0,-6,0],[12,6,plate_t]);
 box([-r,-6,0],[r,6,axis_z]);
 translate([0,-6,axis_z]) rotate([-90,0,0]) cylinder(h=12,r=r);
 cones();
 }
}
// 0.25 mm bottom edge relief reduces elephant-foot sensitivity.
module bed_relief(){
 intersection(){
 children();
 union(){
 translate([0,0,0.25]) linear_extrude(20) projection() children();
 hull(){
 linear_extrude(0.001) offset(delta=-0.25) projection(cut=true) translate([0,0,-0.01]) children();
 translate([0,0,0.25]) linear_extrude(0.001) projection(cut=true) translate([0,0,-0.26]) children();
 }
 }
 }
}
module left(){bed_relief() left_raw();}
module right(){bed_relief() right_raw();}
if(part=="assembly"){left();right();}
if(part=="left") left();
if(part=="right") right();
