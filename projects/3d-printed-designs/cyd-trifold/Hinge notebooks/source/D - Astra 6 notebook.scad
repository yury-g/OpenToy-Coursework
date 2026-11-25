// Notebook D: two outward-cone blind-socket units on two continuous panels.
// Units mm. PRINT AS SUPPLIED: both panel bottoms on Z=0, no assembly.
// Use CGAL export backend. Original coupon files are independent and retained.
part="assembly"; // assembly,left,right
$fn=96;
axis_z=5;
r=3.6;
normal_gap=0.45;
end_gap=0.60;
plate_t=2.8;
unit_centers=[-19,19]; // 38 mm separation along common Y axis
module box(a,b){translate(a) cube(b-a);}
// Rounded plan-view corners plus convex top-only rolled edges.
// No underside roundover: every upper profile stays inside the footprint.
module rounded_profile(a,b,rad,inset=0){
 rr=rad-inset;
 hull() for(x=[a[0]+rad,b[0]-rad]) for(y=[a[1]+rad,b[1]-rad])
 translate([x,y]) circle(r=rr,$fn=48);
}
module soft_box(a,b,corner=1,top_roll=0.6){
 hull(){
 translate([0,0,a[2]]) linear_extrude(0.001) rounded_profile(a,b,corner);
 for(t=[0:11.25:90])
 translate([0,0,b[2]-top_roll+top_roll*sin(t)-0.001])
 linear_extrude(0.001)
 rounded_profile(a,b,corner,top_roll*(1-cos(t)));
 }
}
module local_cones(clear=0){
 for(s=[-1,1]) translate([0,s*6,axis_z]) rotate([-90*s,0,0])
 cylinder(h=r+clear*sqrt(2),r1=r+clear*sqrt(2),r2=0);
}
module sockets(){
 difference(){
 union(){for(s=[-1,1]){
  y0=s<0?-12.6:6+end_gap;
  y1=s<0?-6-end_gap:12.6;
  soft_box([-10,y0,0],[0,y1,plate_t],1,0.5);
  soft_box([-5,y0,0],[5,y1,10.4],1.0,0.6);
 }}
 local_cones(normal_gap);
 }
}
module rotor(){
 union(){
 soft_box([0,-6,0],[12,6,plate_t],1,0.5);
 box([-r,-6,0],[r,6,axis_z]);
 translate([0,-6,axis_z]) rotate([-90,0,0]) cylinder(h=12,r=r);
 local_cones();
 }
}
module left_raw(){
 difference(){
 union(){
 soft_box([-36,-36,0],[-7,36,plate_t],4.5,0.7);
 for(y=unit_centers) translate([0,y,0]) sockets();
 }
 // Top-face engraving: 0.6 mm deep (three 0.2 mm layers).
 // Text runs along long panel direction so small attribution need not shrink.
 // Bold 5.5 mm lettering + 0.08 mm outward offset thickens recess strokes.
 translate([-16,14,plate_t-0.6]) rotate([0,0,90]) linear_extrude(0.8)
 text("D",size=10,font="Liberation Sans:style=Bold");
 translate([-28,-30,plate_t-0.6]) rotate([0,0,90]) linear_extrude(0.8)
 offset(delta=0.08) text("2026-11-24",size=5.5,font="Liberation Sans:style=Bold");
 translate([-16,-30,plate_t-0.6]) rotate([0,0,90]) linear_extrude(0.8)
 offset(delta=0.08) text("Astra 6",size=5.5,font="Liberation Sans:style=Bold");
 }
}
module right_raw(){
 union(){
 soft_box([10,-36,0],[36,36,plate_t],4.5,0.7);
 for(y=unit_centers) translate([0,y,0]) rotor();
 }
}
// Bottom 0.25 mm chamfer, separately applied to each connected leaf.
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
