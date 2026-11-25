/* Swan v02 — original five-servo exploratory CAD, 2026-10-01.
   PLA / 0.4 nozzle / 0.20 layers / 0.45 mm radial running gap.
   Captive mechanism prints upright; orange/red starter structures are removed.
   Motor service covers print separately, without hardware in the printer.
   Fit, friction, grip, speed and payload require physical qualification.
   +Y is the beak direction. All dimensions are millimetres. */
include <servo-toy-library.scad>
part="preview"; gap=.45;
turn=0; shoulder=0; elbow=0; wrist=0; jaw=0;
P=[0,34,83]; U=[0,34,137]; W=[0,34,191]; J=[0,68,221];
R=50;

module yawshape(g=0){rotate_extrude()polygon([[0,4.6-g],[R+g,4.6-g],[R+g,9-g],[R+2+g,11],[R+g,13+g],[R+g,53+g],[0,53+g]]);}
module base(){
 difference(){union(){translate([-80,-74,0])rounded_box([160,140,4],10);translate([0,0,4])cylinder(r=R+6.5,h=47.5);}
  translate([-7.1,46,45.3])cube([3.6,18,6.3]);yawshape(gap);translate([0,0,3.9])cylinder(r=R-3-gap,h=33);
  for(a=[45,135,225,315])rotate([0,0,a])translate([R-3,-7,4])cube([14,14,4]);
  for(x=[-70,70],y=[-64,56])translate([x,y,-eps])cylinder(d=3.4,h=6);
 }
 translate([0,0,52-gap])servo_mount("standard");
 for(x=[-13.95,33.65],y=[-5,5])difference(){translate([x,y,3.8])cylinder(r=3,h=28.45);translate([x,y,3])cylinder(d=2.5,h=34);}
}

// Embedded motor pockets: bearing loads go into captive PLA journals,
// while one purchased horn transmits torque. The service opening faces +Y.
module motor(Pt,k,sx,flip=false){translate(Pt)translate([sx,0,0])rotate([0,90,0])rotate([0,0,flip?180:0])servo_mesh(k);}
module motorcut(Pt,k,sx,flip=false){translate(Pt)translate([sx,0,0])rotate([0,90,0])rotate([0,0,flip?180:0])servo_clearance(k,.55);}
module drivesweep(Pt,sx){translate(Pt)translate([sx+.8,0,0])rotate([0,90,0])cylinder(r=17,h=5.5);}
module servicefront(Pt,xl,xr,zlo,zhi){translate(Pt)translate([xl-2.1,11.3,zlo+2.4])cube([xr-xl+4.2,55,zhi-zlo]);}
// Swept insertion corridors preserve the flange seating plane.
module entry(Pt,k,sx,flip=false,side=1){
 p=profile(k);c=.55;m=p[0]/2-p[3];fd=flange_drop(k);d=60;sy=flip?-side:side;
 translate(Pt)translate([sx,0,0])rotate([0,90,0])rotate([0,0,flip?180:0]){
  translate([-p[3]-c,-p[1]/2-c+(sy<0?-d:0),-p[2]-c])cube([p[0]+2*c,p[1]+2*c+d,p[2]-7.5+c]);
  translate([m-p[5]/2-c,-p[1]/2-c+(sy<0?-d:0),-fd])cube([p[5]+2*c,p[1]+2*c+d,2+c]);
  hull(){for(y=[0,sy*d])translate([0,y,-7.5])cylinder(r=6+c,h=3.6+c);}
  hull(){for(y=[0,sy*d])translate([0,y,-4])cylinder(r=2.5+c,h=4.1);}
 }
 hull(){for(y=[0,side*d])translate(Pt)translate([sx+.8,y,0])rotate([0,90,0])cylinder(r=17,h=2.4);}
}
module mount(Pt,k,sx,flip=false){translate(Pt)translate([sx,0,0])rotate([0,90,0])rotate([0,0,flip?180:0])servo_mount(k);}
module horn(Pt,sx,flip=false){translate(Pt)translate([sx+1,0,0])rotate([0,90,0])rotate([0,0,flip?180:0])stock_horn();}
module pod(Pt,k,sx,xl,xr,flip=false){
 zlo=k=="standard"?-44:flip?-17:-28; zhi=k=="standard"?21:flip?28:17;
 yy=k=="standard"?18.5:14;
 translate(Pt)difference(){
  union(){difference(){translate([xl-4.5,-yy,zlo])rounded_box([xr-xl+9,2*yy,zhi-zlo],3);
    translate([xl-2.1,-yy+2.4,zlo+2.4])cube([xr-xl+4.2,2*yy+5,zhi-zlo+2]);}
   for(v=[[xl-.25,zlo+(k=="standard"?16:7)],[xr+.25,zhi-6]])translate([v[0],yy-2.8,v[1]])rotate([90,0,0])cylinder(r=2.8,h=4.8);
   for(x=[xl,xr]){translate([x,0,0])socket_x(g=gap);translate([x-3,-yy,zlo])cube([6,yy+1,-zlo]);}
  }
  for(v=[[xl-.25,zlo+(k=="standard"?16:7)],[xr+.25,zhi-6]])translate([v[0],yy,v[1]])rotate([90,0,0])cylinder(d=1.6,h=8);
  for(x=[xl,xr])translate([x,0,0])bore_x(g=gap);
  for(y=[-12,12])translate([sx+2,y,0])rotate([0,90,0])cylinder(d=3.2,h=xr-sx+9);
  translate([sx+.8,0,0])rotate([0,90,0])cylinder(r=17,h=5.5);
 }
 mount(Pt,k,sx,flip);
}
module root(Pt,xl,xr,sx,h=29){translate(Pt)difference(){union(){
 for(x=[xl,xr])translate([x,0,0])shaft_x();
 translate([sx+3,0,0])rotate([0,90,0])horn_plate();
 translate([sx+5.7,0,0])rotate([0,90,0])cylinder(r=7,h=xr-sx-5.7);
 for(x=[xl-8,xr+4.9]){translate([x,-4,0])cube([3.1,8,h]);translate([x,0,0])rotate([0,90,0])cylinder(r=7,h=3.1);}
 translate([xl-8,-5,h-3])rounded_box([xr-xl+16,10,3],2);
}translate([sx-1,0,0])rotate([0,90,0])cylinder(d=4.6,h=xr-sx+11);}}
module tube(z0,L,curve,r=11){difference(){for(i=[0:11])hull(){for(t=[i/12,(i+1)/12])translate([0,34+curve*sin(180*t),z0+L*t])cylinder(r=r,h=1);}
 for(i=[0:11])hull(){for(t=[i/12,(i+1)/12])translate([0,34+curve*sin(180*t),z0-.1+L*t])cylinder(r=r-2.5,h=1.3);}}}

module carrier(){difference(){union(){
 difference(){yawshape();translate([0,0,4])cylinder(r=R-3,h=52);}
 difference(){translate([0,0,52.5])cylinder(r=R+4,h=5.5);translate([0,0,52])cylinder(r=R-3,h=3);}
 pod(P,"standard",10,-39,22);
 for(x=[-39,22])translate([x-3,19,50])cube([6,16,33]);
 }
 for(a=[0:30:330])rotate([0,0,a])translate([R-5,-10,23])cube([12,20,25]);
 for(a=[0:60:300])rotate([0,0,a])translate([20,-15,55])cube([25,30,4]);
 translate([-7.1,46,45.3])cube([3.6,18,7.3]);
 // Narrow flange-entry slot aligns with the base only at the assembly pose.
 // Installation access for the yaw servo and folded-neck clearance.
 translate([-18.3,-10.6,52])cube([56,21.2,9]);translate([0,0,52])radial_slots(8);
 translate([-37,49,39])cube([74,27,23]);translate([23.8,49,26])cube([3.2,5,13]);translate([23,52,25.9])cube([1,1,3]);
 entry(P,"standard",10);drivesweep(P,10);motorcut(P,"standard",10);translate([0,0,20])cylinder(r=32.8,h=25.1);difference(){translate([-80,-80,20])cube([160,160,32.6]);translate([0,0,19])cylinder(r=R,h=35);}translate([0,0,52-gap])servo_clearance("standard",.6);translate([0,0,52.3])cylinder(r=17,h=3);
 for(x=[-39,22])translate(P)translate([x,0,0])bore_x(g=gap);
}}
module lower(){difference(){union(){root(P,-39,22,10,33);tube(113,17,-6);pod(U,"premium",7.4,-30,18);}
 entry(U,"premium",7.4);motorcut(U,"premium",7.4);drivesweep(U,7.4);servicefront(U,-30,18,-28,17);}}
module middle(){difference(){union(){root(U,-30,18,7.4,26);tube(160,22,6,10.5);pod(W,"strong",7,-30,18);}
 entry(W,"strong",7);motorcut(W,"strong",7);drivesweep(W,7);servicefront(W,-30,18,-28,17);}}
module head(){difference(){union(){root(W,-30,18,7,26);
 for(x=[-33,18]){hull(){translate([x,32,214])cube([4,4,4]);translate([x,48,215])cube([4,4,4]);}hull(){translate([x,48,215])cube([4,4,4]);translate([x,54,204])cube([4,4,4]);}}
 // Thin hollow head surrounds the jaw motor, with an open service face.
 pod(J,"micro",7,-30,18,true);
 hull(){translate([-13,30,214])cube([26,8,4]);translate([-13,54,220])cube([26,8,4]);}
 translate(J)translate([-11,20,5])linear_extrude(3)polygon([[0,0],[22,0],[18,20],[4,20]]);
 translate(J)translate([-11,8,5])cube([22,15,3]);
 translate(J){translate([-29,-12,18])cube([5,35,3]);translate([-29,18,5])cube([5,5,16]);translate([-29,18,5])cube([40,5,3]);}
 }
 entry(J,"micro",7,true,-1);motorcut(J,"micro",7,true);drivesweep(J,7);
}}
module beak(){difference(){union(){
 translate(J){for(x=[-30,18])translate([x,0,0])shaft_x();translate([10,0,0])rotate([0,90,0])horn_plate();translate([12.7,0,0])rotate([0,90,0])cylinder(r=7,h=5.3);
 for(x=[-38,22.9]){translate([x,-4,-6])cube([3.1,28,4]);translate([x,0,0])rotate([0,90,0])cylinder(r=7,h=3.1);}
 translate([-38,19,-6])rounded_box([64,6,3],2);
 translate([-10,20,-6])linear_extrude(3)polygon([[0,0],[20,0],[16,20],[4,20]]);
 }
 }translate(J)translate([5,0,0])rotate([0,90,0])cylinder(d=4.6,h=25);}}

// Covers print flat, with 0.3 mm edge clearance. Two M2 x 6 screws secure
// each cover to side-wall pilot bosses. Covers are optional for first tests.
module cover_raw(Pt,k,xl,xr,flip=false){
 zlo=k=="standard"?-44:flip?-17:-28;zhi=k=="standard"?21:flip?28:17;yy=k=="standard"?18.5:14;
 translate(Pt){difference(){translate([xl-1.8,yy-.8,zlo+2.7])rotate([90,0,0])rounded_box([xr-xl+3.6,zhi-zlo-5.4,1.8],3);
  translate([xl+4,yy-3,zlo+7])cube([xr-xl-8,6,4]);
  for(v=[[xl-.25,zlo+(k=="standard"?16:7)],[xr+.25,zhi-6]])translate([v[0],yy+1,v[1]])rotate([90,0,0])cylinder(d=2.2,h=8);
  for(x=[xl,xr])translate([x-5,0,0])rotate([0,90,0])cylinder(r=13,h=10);
  translate([k=="standard"?10.8:7.8,0,0])rotate([0,90,0])cylinder(r=17.2,h=5.7);
  if(flip){translate([-12,8,4.7])cube([24,16,3.6]);translate([-29.5,8,17.5])cube([6,16,4]);}}
  
 }
}
module cover1(){difference(){cover_raw(P,"standard",-39,22);difference(){translate([-80,-80,20])cube([160,160,32.6]);translate([0,0,19])cylinder(r=R,h=35);}translate([-80,-80,0])cube([160,160,49.95]);}}
module cover2(){cover_raw(U,"strong",-30,18);}
module cover3(){cover_raw(W,"strong",-30,18);}
module cover4(){cover_raw(J,"micro",-30,18,true);}
module coverflat(i){
 Pt=i==1?P:i==2?U:i==3?W:J;k=i==1?"standard":i==4?"micro":"strong";
 xl=i==1?-39:-30;xr=i==1?22:18;yy=i==1?18.5:14;zlo=i==1?-44:i==4?-17:-28;
 translate([-xl+2,zlo*-1+1,yy-.8])rotate([-90,0,0])translate(-Pt){if(i==1)cover1();else if(i==2)cover2();else if(i==3)cover3();else cover4();}
}
module covers(){for(i=[1:4])translate([((i-1)%2)*80,floor((i-1)/2)*75,0])coverflat(i);}

module starters(){turn_starters(R);
 for(x=[-28,28],y=[-19,19])starter_post(x,y,52.5);
 // Front-access sacrificial walls support the pocket bridges and root bars.
 for(i=[0:3]){Pt=i==0?P:i==1?U:i==2?W:J;xl=i==0?-39:-30;xr=i==0?22:18;
  
  for(x=[xl,xr])translate([x-.35,Pt[1]-.35,Pt[2]-9.35])cube([.7,.7,.7]);
  zlo=i==0?-44:i==3?-17:-28;
  for(x=[xl+2,xr-2])starter_post(x,Pt[1]+7,Pt[2]+zlo);
  starter_wall((xl+xr)/2,Pt[1],Pt[2]+(i==3?-6:i==0?30:23),xr-xl+13,90);
  starter_post(i==0?14.5:11.5,Pt[1],Pt[2]-15);
 }
 starter_post(-39.25,47.3,52.65);starter_post(22.25,47.3,95.25,51.5);starter_post(-6.45,41.1,143.25);starter_post(-30.25,42.8,167.25);starter_post(-30.25,76.8,208.25);starter_post(18.25,76.8,240.25);starter_post(-7.8,45,94.05);starter_wall(0,97,226,20,90);starter_wall(0,97,215,18,90);
}
module servo0(){translate([0,0,52-gap])servo_mesh("standard");}
module servo1(){motor(P,"standard",10);}module servo2(){motor(U,"premium",7.4);}module servo3(){motor(W,"strong",7);}module servo4(){motor(J,"micro",7,true);}
module horn0(){translate([0,0,53])stock_horn();}module horn1(){horn(P,10);}module horn2(){horn(U,7.4);}module horn3(){horn(W,7);}module horn4(){horn(J,7,true);}
module assembly(pre=false){base();if(pre)servo0();translate([0,0,pre?-gap:0])rotate([0,0,turn]){carrier();if(pre){servo1();horn0();cover1();}
 about(P,shoulder){lower();if(pre){servo2();horn1();cover2();}about(U,elbow){middle();if(pre){servo3();horn2();cover3();}about(W,wrist){head();if(pre){servo4();horn3();cover4();}about(J,jaw){beak();if(pre)horn4();}}}}}}
if(part=="preview")assembly(true);else if(part=="mechanism"){assembly();starters();}
else if(part=="base")base();else if(part=="carrier")carrier();else if(part=="lower")lower();else if(part=="middle")middle();else if(part=="head")head();else if(part=="beak")beak();else if(part=="starters")starters();else if(part=="covers")covers();
else if(part=="servo0")servo0();else if(part=="servo1")servo1();else if(part=="servo2")servo2();else if(part=="servo3")servo3();else if(part=="servo4")servo4();
else if(part=="horn0")horn0();else if(part=="horn1")horn1();else if(part=="horn2")horn2();else if(part=="horn3")horn3();else if(part=="horn4")horn4();
else if(part=="cover1")cover1();else if(part=="cover2")cover2();else if(part=="cover3")cover3();else if(part=="cover4")cover4();

module middle_joint_coupon(){Q=[0,0,28];difference(){pod(Q,"premium",7.4,-30,18);entry(Q,"premium",7.4);motorcut(Q,"premium",7.4);drivesweep(Q,7.4);servicefront(Q,-30,18,-28,17);}root(Q,-30,18,7.4,26);for(x=[-30,18])translate([x-.35,-.35,18.65])cube([.7,.7,.7]);starter_post(11.9,0,13);starter_post(-36.5,0,21);starter_post(24.5,0,21);starter_wall(-6,0,51,64,90);starter_post(-6.45,7.1,34.25);starter_post(-30.25,8.8,4.25);starter_post(18.25,8.8,36.25);}
if(part=="middle-joint-coupon")middle_joint_coupon();
if(part=="cover2-flat")coverflat(2);
