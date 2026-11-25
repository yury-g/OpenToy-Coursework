// Compact CYD S-fold prototype. Upright PIP chassis; separately flat printed bezels.
part="assembly"; // assembly,left,middle,right,left-bezel,middle-bezel,right-bezel,bezel-print
print_pose=false;
left_fold=0; right_fold=0;
$fn=64;
centers=[-54,0,54]; yh=-61; band=6; pitch=8.2; race_end=14.2;
front_axis=16; back_axis=-1.3;
stem=2.2; rise=1.2; gap=.6; outer=5.2;
module box(a,b){translate(a)cube(b-a);}
module rbox(a,b,r=.8){hull()for(x=[a.x+r,b.x-r],y=[a.y+r,b.y-r])translate([x,y,a.z])cylinder(h=b.z-a.z,r=r);}
module alongY(){rotate([-90,0,0])children();}
module axial_cyl(x,z,y0,y1,r){translate([x,y0,z])alongY()cylinder(h=y1-y0,r=r);}
module race(z0,socket=false,bed=false){
 rr=stem+(socket?gap:0); toe=bed?(socket?.25:-.25):0;
 translate([0,0,z0])rotate_extrude()polygon(concat([[0,0],[rr+toe,0],[rr,.4],[rr,1.2],[rr+rise,2.4],[rr+rise,3.3],[rr,4.5],[rr,band],[0,band]]));
}
// Native bearing axis Z, then converted to global Y. Both closed races start on bed.
module male_native(){
 race(0,false,true);race(pitch);
 translate([0,0,band-.01])cylinder(h=pitch-band+.02,r=stem);
 translate([0,0,race_end-.01])cylinder(h=2.61,r=stem);
}
module collar_native(){
 difference(){
 cylinder(h=race_end,r=outer);
 translate([0,0,-.1])cylinder(h=race_end+.2,r=stem+gap);
 translate([0,0,-.01])race(0,true,true);
 race(pitch,true);
 }
}
module male(x,z,side){
 translate([x,yh,z])alongY()male_native();
 // Fixed spoke exits above closed collars; short bridge between hub and case pillar.
 if(side<0){box([x-8,yh+15,z-.8],[x+1,yh+16.6,z+.8]);box([x-8,yh,min(0,z-.8)],[x-6,yh+16.6,z<0?1.8:max(13.2,z+.8)]);}
 else{box([x-1,yh+15,z-.8],[x+8,yh+16.6,z+.8]);box([x+6,yh,min(0,z-.8)],[x+8,yh+16.6,z<0?1.8:max(13.2,z+.8)]);}
}
module collar(x,z,side){
 translate([x,yh,z])alongY()collar_native();
 if(side<0)box([x-9,yh,z<0?z+1:min(0,z-3)],[x-4.3,yh+race_end,z<0?1.8:max(13.2,z-1)]);
 else box([x+4.3,yh,z<0?z+1:min(0,z-3)],[x+9,yh+race_end,z<0?1.8:max(13.2,z-1)]);
}
module seat(side,yy){
 mirror([side<0?1:0,0,0])hull(){
 box([20.5,yy-8,0],[25.2,yy-7.8,1.8]);
 box([20.5,yy,0],[25.2,yy+4,7.7]);
 }
}
module tray(x){
 translate([x,0,0])difference(){
 union(){
 rbox([-26.6,-61,0],[26.6,46,1.8],3);
 for(s=[-1,1])mirror([s<0?1:0,0,0]){
 box([25.3,-61,0],[26.6,46,13.2]);
 for(yy=[-42,39])seat(1,yy);
 // Corner end stops, central short edges remain open for USB.
 box([21,-44.5,0],[26.6,-43.35,9.5]);
 box([21,43.35,0],[26.6,46,9.5]);
 }
 }
 // USB back relief; lower opening ends in a45-degree peak for upright printing.
 translate([0,0,-.1])linear_extrude(2)polygon([[-20,-62],[20,-62],[20,-45],[0,-25],[-20,-45]]);
 box([-20,36.5,-.1],[20,47,1.9]);
 // Spring-clip sockets. Long45-degree ending ramp avoids an upright bridge.
 for(s=[-1,1],yy=[-25,25])mirror([s<0?1:0,0,0]){
 translate([25.25,yy-3,0])rotate([90,0,90])linear_extrude(1.5)
 polygon([[0,3.6],[6,3.6],[15.8,13.4],[0,13.4]]);
 translate([25.25,yy-3,0])rotate([90,0,90])linear_extrude(1.05)
 polygon([[0,2],[6,2],[17.4,13.4],[0,13.4]]);
 box([26.25,yy-2.25,2.3],[26.7,yy+2.25,3.5]);
 }
 }
}
module chassis(i){
 x=centers[i];
 difference(){tray(x);
 if(i==0||i==1)axial_cyl(-27,front_axis,yh-.1,yh+17.4,outer+.4);
 if(i==1||i==2)axial_cyl(27,back_axis,yh-.1,yh+17.4,outer+.4);
 }
 if(i==0)collar(-27,front_axis,-1);
 if(i==1){male(-27,front_axis,1);male(27,back_axis,-1);}
 if(i==2)collar(27,back_axis,1);
}
module bezel_clip(side,yy){
 mirror([side<0?1:0,0,0]){
 box([25.35,yy-2,2.5],[26.1,yy+2,13.5]);
 // Hook underside and tip slope. Assembled hook catches below socket ledge.
 translate([0,yy+2,0])rotate([90,0,0])linear_extrude(4)
 polygon([[26,2.5],[26.5,3.0],[26.5,3.3],[26,3.8]]);
 }
}
module bezel_plate(i){
 hull() for(s=[-1,1],t=[-1,1]) {
 rr=((i==0&&s<0)||(i==2&&s>0))?3:.8;
 translate([s*(26.6-rr),t<0?-61+rr:46-rr,13.3])cylinder(h=1.3,r=rr);
 }
}
module bezel(i){
 x=centers[i];
 difference(){
 translate([x,0,0])union(){
 difference(){bezel_plate(i);
 rbox([-22,-31.7,13],[22,26.7,15],.8);}
 for(s=[-1,1],yy=[-25,25])bezel_clip(s,yy);
 for(s=[-1,1],yy=[-39,39])mirror([s<0?1:0,0,0])rbox([20.8,yy-2,9.4],[23.8,yy+2,13.4],.5);
 }
 if(i==0||i==1)axial_cyl(-27,front_axis,yh-.1,yh+17.4,outer+.5);
 // Clear fixed pillar at front bearing in its own and reflected position.
 if(i==0||i==1)box([-36,-61,13.2],[-18,-43.4,15]);
 }
 if(i==0||i==1)intersection(){box([-36,-43.4,9.5],[-18,-42.4,13.4]);box([x-25.15,-44,0],[x+25.15,-42,14]);}
}
module spin(p,a){translate(p)rotate([0,a,0])translate(-p)children();}
module pose(i){if(i==0)spin([-27,0,front_axis],left_fold)children();else if(i==2)spin([27,0,back_axis],right_fold)children();else children();}
module result(){
 if(part=="assembly")for(i=[0:2])pose(i)chassis(i);
 if(part=="left")chassis(0);if(part=="middle")chassis(1);if(part=="right")chassis(2);
 if(part=="left-bezel")bezel(0);if(part=="middle-bezel")bezel(1);if(part=="right-bezel")bezel(2);
 if(part=="bezel-print")translate([0,0,14.6])rotate([180,0,0])translate([-centers[is_undef(which)?1:which],0,0])bezel(is_undef(which)?1:which);
}
if(print_pose)multmatrix([[1,0,0,135],[0,0,-1,140],[0,1,0,61],[0,0,0,1]])result();else result();
