// ONE flat-printed captive three-screen holder. Install boards only.
// A opposed cone/divot bearing surfaces retained; supporting cheeks are taller.
part="assembly"; // assembly,left,middle,right,fixed-left,fixed-middle,fixed-right,tips-left,tips-middle,tips-right
relief=true; // false exports the same design without material-saving windows
$fn=64;
pitch=64; centers=[-pitch,0,pitch]; stations=[-67,67]; left_axis=18.3; right_axis=28.3;
cone_radius=3.35; cone_depth=2.85; normal_gap=.4; axial_gap=.5;
base_t=3; pcb_seat=11; rail_h=17.3;
module box(a,b){translate(a)cube(b-a);}
module rounded_xy(x0,x1,y0,y1,r){translate([x0+r,y0+r])offset(r=r)square([x1-x0-2*r,y1-y0-2*r]);}
module softbox(x0,x1,y0,y1,h,r=1.2,edge=.6){
 hull(){linear_extrude(.01)rounded_xy(x0,x1,y0,y1,r);
 for(a=[0:15:90]){d=edge*(1-cos(a));translate([0,0,h-edge+edge*sin(a)-.01])linear_extrude(.01)rounded_xy(x0+d,x1-d,y0+d,y1-d,r-d);}}
}
module xzprism(y0,y1,p){translate([0,y1,0])rotate([90,0,0])linear_extrude(y1-y0)polygon(p);}
module yzprism(x0,x1,p){multmatrix([[0,0,1,x0],[1,0,0,0],[0,1,0,0],[0,0,0,1]])linear_extrude(x1-x0)polygon(p);}
module mx(s){if(s<0)mirror([1,0,0])children();else children();}
module cone(y,d,h,clearance=0){translate([0,y,h])rotate([-90*d,0,0])cylinder(h=cone_depth+clearance*sqrt(2),r1=cone_radius+clearance*sqrt(2),r2=cone_radius-cone_depth);}
module male(h){
 union(){
 softbox(-26,-7,-13,13,3,1.2);
 for(d=[-1,1]){
 difference(){softbox(-12,4.4,d<0?-13:8.5,d<0?-8.5:13,h+cone_radius+1,1.2);
 if(relief)xzprism(-14,14,[[-8,4],[0,4],[0,h-10.5],[-4,h-6.5],[-8,h-10.5]]);}
 cone(d*8.5,-d,h);
 }
 }
}
module socket(h){
 difference(){union(){
 softbox(0,26,-8,8,3,1.2);
 softbox(-4.3,4.3,-8,8,h,1.2);
 translate([0,-8,h])rotate([-90,0,0])cylinder(h=16,r=4.3);
 }
 for(d=[-1,1])cone(d*8.5,-d,h,normal_gap);
 if(relief)xzprism(-9,9,[[-1.3,4],[1.3,4],[1.3,h-7.8],[0,h-6.5],[-1.3,h-7.8]]);
 }
}
module base(){
 difference(){softbox(-29,29,-45,45,base_t,4,.5);
 if(relief)for(s=[-1,1])mx(s)for(yy=[[-38,-20],[-12,0],[8,36]])
 translate([0,0,-.1])linear_extrude(3.2)rounded_xy(3,25.5,yy[0],yy[1],3);
 // Solid USB-end load band retained. Raised plug envelope starts0.4mm above it.
 }
}
module tip(s){mx(s)translate([0,0,0])yzprism(20,25.2,[[-48.8,0],[-45.8,0],[-45.8,10],[-42.4,13.4],[-42.4,13.9],[-45.8,15.3],[-48.8,15.3]]);}
module flexure(s){mx(s){
 // 2 mm in-plane bending thickness, 3 mm tall, horizontal bed-rooted beam.
 box([1,-48.4,0],[23,-46.4,3]);
 // Rounded root blend to the central anchor; clear of the flexing free span.
 hull(){translate([2,-47.4,0])cylinder(h=3,r=1);translate([0,-45.4,0])cylinder(h=3,r=2);}
 }}
module tray_fixed(include_beams=true){
 base();
 // 3.5 mm protective long walls, with localized side-access slots.
 difference(){union(){for(s=[-1,1])mx(s)softbox(25.5,29,-44,44,rail_h,1.2,.6);}
 // Extended IO, DHT and SD on positive-X edge; speaker on opposite edge.
 for(yy=[[-36,-20],[-20,-4],[-4,16]])box([25.4,yy[0],3],[29.1,yy[1],18]);
 box([-29.1,8,3],[-25.4,24,18]);
 }
 // Solid bare-PCB corner seats, four per board.
 for(s=[-1,1],yy=[-41,41])mx(s)translate([0,yy,0])softbox(20.5,26.5,-2,2,pcb_seat,1,.5);
 // Fixed USB-end lips: 45-degree underside, no bearing load on glass.
 for(s=[-1,1])mx(s)yzprism(20.5,25.2,[[43.5,0],[46,0],[46,15.4],[41.7,15.4],[41.7,14.1],[43.5,12.3]]);
 // Central flexure root attached to the central base spine.
 softbox(-3,3,-49,-43.5,3,1,.5);
 for(s=[-1,1]){
 if(include_beams)flexure(s);
 mx(s){
 // Protected rear catch, 0.9 mm travel stop and 0.8 mm side clearance.
 softbox(19.3,29,-53,-49.7,16.5,1,.5);
 softbox(26,29,-52,-43,16.5,1,.5);
 }
 }
}
module tray(){tray_fixed();tip(-1);tip(1);}
module connector(dir,yy){
 mx(dir){
 if(yy<0){
 softbox(18,26,0,17,3,1.2);
 softbox(pitch/2-29,26,13.5,17,3,1.2);
 }else softbox(20,28,-30,0,3,1.2);
 }
}
module leaf(i,omit_tips=false,tips_only=false,omit_beams=false){
 x=centers[i];
 if(tips_only)translate([x,0,0]){tip(-1);tip(1);}
 else{
 translate([x,0,0]){if(omit_tips)tray_fixed(!omit_beams);else tray();}
 for(yy=stations){
 if(i==0)translate([-pitch/2,yy,0]){male(left_axis);connector(-1,yy);}
 if(i==1){
 translate([-pitch/2,yy,0]){socket(left_axis);connector(1,yy);}
 translate([pitch/2,yy,0]){male(right_axis);connector(-1,yy);}
 }
 if(i==2)translate([pitch/2,yy,0]){socket(right_axis);connector(1,yy);}
 }
 }
}
if(part=="assembly")for(i=[0:2])leaf(i);
if(part=="left")leaf(0);if(part=="middle")leaf(1);if(part=="right")leaf(2);
if(part=="fixed-left")leaf(0,true);if(part=="fixed-middle")leaf(1,true);if(part=="fixed-right")leaf(2,true);
if(part=="tips-left")leaf(0,false,true);if(part=="tips-middle")leaf(1,false,true);if(part=="tips-right")leaf(2,false,true);

if(part=="obstacles-middle")leaf(1,true,false,true);
