// CYD three-screen roll-fold holder, based on preferred hinge A.
// mm. Print assembly flat as supplied. No separate pins or hinge assembly.
part="assembly"; // assembly, left, middle, right, fit
left_fold=0; right_fold=0; // preview degrees 0..180; left first then right
$fn=64;
tray_centers=[-70,0,70];
board_w=50; board_l=86; pcb_t=1.5; pcb_seat=8;
tray_w=58; tray_l=92; tray_t=2; rail_h=14.2;
cone_radius=3.35; cone_depth=2.85; normal_gap=.4; axial_gap=.5;
station_y=[-62,62];
module box(x0,x1,y0,y1,z0,z1){translate([x0,y0,z0])cube([x1-x0,y1-y0,z1-z0]);}
module outline(x0,x1,y0,y1,r){translate([x0+r,y0+r])offset(r=r)square([x1-x0-2*r,y1-y0-2*r]);}
// Flat sole, vertical side then inward top-only roll. No unsupported underside round.
module softbox(x0,x1,y0,y1,h,r=1.2,edge=.6){
 hull(){linear_extrude(.01)outline(x0,x1,y0,y1,r);
 for(a=[0:15:90]){d=edge*(1-cos(a));translate([0,0,h-edge+edge*sin(a)-.01])linear_extrude(.01)outline(x0+d,x1-d,y0+d,y1-d,r-d);}}
}
module cone(y,d,h,clearance=0){translate([0,y,h])rotate([-90*d,0,0])cylinder(h=cone_depth+clearance*sqrt(2),r1=cone_radius+clearance*sqrt(2),r2=cone_radius-cone_depth);}
module male_unit(h){
 for(d=[-1,1]){softbox(-12,4.4,d<0?-13:8.5,d<0?-8.5:13,h+cone_radius+1,1.2);cone(d*8.5,-d,h);}
 // Broad sole attaches cheeks to their tray connector.
 softbox(-26,-7,-13,13,3,1.2);
}
module socket_unit(h){
 difference(){union(){
 softbox(0,26,-8,8,3,1.2);
 softbox(-4.3,4.3,-8,8,h,1.2);
 translate([0,-8,h])rotate([-90,0,0])cylinder(h=16,r=4.3);
 }
 for(d=[-1,1])cone(d*8.5,-d,h,normal_gap);
 }
}
module clip(side,yy){
 // Vertical spring beam; recessed from protective rail. Retains exposed PCB edge.
 // 0.90 mm beam, 8.8 mm free height; physical snap force is untested.
 mirror([side<0?1:0,0,0]) translate([0,yy,0]){
 box(25.45,26.35,-2.3,2.3,1.7,10.5);
 // 45-degree underside lead-in + insertion bevel, profile [x,z].
 translate([0,2.3,0])rotate([90,0,0])linear_extrude(4.6)
 polygon([[25.45,9.35],[24.65,10.15],[24.65,10.4],[25.8,11.55],[26.35,11.55],[26.35,9.35]]);
 }
}
module tray(){
 difference(){
 union(){
 softbox(-29,29,-46,46,2,4,.6);
 // Long side rails keep glass below the handling edges.
 for(s=[-1,1])mirror([s<0?1:0,0,0]){
 softbox(27.2,29,-44,44,rail_h,.8,.4);
 for(yy=[-39,39]){
 translate([0,yy,0])softbox(20.5,25.25,-4,4,pcb_seat,1,.4);
 clip(1,yy);
 }
 }
 // End stops only at bare corner PCB edges. Middle 40 mm remains open for USB.
 for(s=[-1,1],end=[-1,1])mirror([s<0?1:0,0,0])mirror([0,end<0?1:0,0])
 softbox(21,26.2,43.45,46,9.2,1,.4);
 }
 // Open cable approach behind connector at either short edge.
 for(s=[-1,1])mirror([0,s<0?1:0,0])box(-20,20,36.5,47,-.1,2.1);
 }
}
module left_raw(){
 translate([-70,0,0])tray();
 for(yy=station_y)translate([-35,yy,0]){
 male_unit(15);
 // Sole joining short units to tray end, beyond board envelope.
 softbox(-28,-20,yy<0?0:-30,yy<0?30:0,2,1.2);
 }
}
module middle_raw(){
 tray();
 for(yy=station_y){
 translate([-35,yy,0]){socket_unit(15);softbox(20,28,yy<0?0:-30,yy<0?30:0,2,1.2);}
 translate([35,yy,0]){male_unit(23);softbox(-28,-20,yy<0?0:-30,yy<0?30:0,2,1.2);}
 }
}
module right_raw(){
 translate([70,0,0])tray();
 for(yy=station_y)translate([35,yy,0]){socket_unit(23);softbox(20,28,yy<0?0:-30,yy<0?30:0,2,1.2);}
}
module spin(p,a){translate(p)rotate([0,a,0])translate(-p)children();}
if(part=="assembly"||part=="left")spin([-35,0,15],left_fold)left_raw();
if(part=="assembly"||part=="middle")middle_raw();
if(part=="assembly"||part=="right")spin([35,0,23],-right_fold)right_raw();
if(part=="fit")tray();
