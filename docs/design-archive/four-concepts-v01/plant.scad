/* Taller plant: 128 degree turn, nod and face tilt. Exploratory print-in-place draft. */
include <servo-toy-library.scad>
part="preview";gap=.45;turn=0;bend=0;tilt=0;face_w=70;face_h=46;P=[0,0,162];
module base(){difference(){union(){turn_base(gap,"micro",true);difference(){cylinder(r1=50,r2=43,h=39);translate([0,0,3.9])cylinder(r1=47,r2=40,h=36);}}translate([-12.5,7,7])cube([36,52,34]);}}
module carrier(){difference(){union(){turn_ring(gap,"micro",true);
 translate([0,0,43])difference(){cylinder(r=13,h=64);cylinder(r=10,h=65);}
 for(s=[-1,1])hull(){translate([s*9-(s<0?6:0),-3,103])cube([6,6,4]);translate([s*45-3,-3,144])cube([6,6,4]);}
 for(s=[-1,1]){translate([s*45-3,-15,144])cube([6,30,18]);translate([s*45,0,162])socket_x(g=gap);}
 translate([-56,0,162])rotate([0,90,0])servo_mount();for(y=[-9,9])translate([-75,y-2,143])cube([33.1,4,4]);
 for(s=[-1,1])hull(){translate([s*10,0,83])scale([1,.6,1])sphere(r=3);translate([s*31,0,106])scale([1,.28,.6])sphere(r=8);}
 }for(s=[-1,1])translate([s*45,0,162])bore_x(g=gap);}}
module nod() {
    difference(){
        union(){
            for(x=[-45,45])translate([x,0,162])shaft_x();
            translate([-53,0,162])rotate([0,90,0])horn_plate();
            // Neck carriage: back and side ribs leave a clear path for the face.
            for(x=[-37,37])translate([x-3,-58,159])cube([6,59,6]);
            translate([-40,-58,159])cube([80,6,6]);
            translate([0,-19,162])rotate([-90,0,0])servo_mount();
            for(x=[-22,25])translate([x-2,-56,152])cube([4,30,10]);
            for(x=[-22,25])hull(){
                translate([x-2,-28,152])cube([4,4,4]);
                translate([x<0?-10:6,-4,152])cube([4,12,4]);
            }
            translate([0,-2,162])rotate([-90,0,0])socket_z(r=8,lip=10,len=18,outer=13,g=gap);
        }
        // Stock roll horn and driven plate need space between the rear support ribs.
        translate([0,-16.5,162])rotate([-90,0,0])cylinder(r=15.5,h=4);
        // Cut through all support ribs to keep the roll bearing cavity clear.
        translate([0,-2,162])rotate([-90,0,0])rotate_extrude()polygon(
          [[0,-10],[8+gap,-10],[8+gap,-1.8-gap],[10+gap,0],
           [8+gap,1.8+gap],[8+gap,10],[0,10]]);
        // Empty shaft core preserves access to stock horn center screws.
        translate([-58,0,162])rotate([0,90,0])cylinder(d=4,h=20);
    }
}
module face() {
    difference(){
        union(){
            translate([0,-2,162])rotate([-90,0,0])shaft_z(r=8,lip=10,len=18);
            translate([0,-16,162])rotate([-90,0,0])horn_plate();
            translate([0,-13.1,162])rotate([-90,0,0])cylinder(r=8,h=3);
            translate([0,6,162])rotate([-90,0,0])cylinder(r=8,h=20);
            translate([-face_w/2,24,162-face_h/2])rotate([90,0,0])
                rounded_box([face_w,face_h,3],7);
            // Shallow perimeter tray; electronics stay close to the roll axis.
            for(x=[-face_w/2,face_w/2-3])translate([x,15,162-face_h/2+7])cube([3,9,face_h-14]);
            for(z=[162-face_h/2,162+face_h/2-3])translate([-face_w/2+7,15,z])cube([face_w-14,9,3]);
            // Rear spokes join the face rim to the roll hub while preserving the display aperture.
            for(x=[-1,1],z=[-1,1])hull(){
                translate([x*5-2,18,162+z*5-2])cube([4,6,4]);
                translate([x*(face_w/2-8)-2,18,162+z*(face_h/2-2)-2])cube([4,6,4]);
            }
        }
        translate([-17,20,153])cube([34,8,18]);
        translate([0,-18,162])rotate([-90,0,0])cylinder(d=4,h=46);
        for(x=[-25,25])translate([x,20,162])rotate([-90,0,0])slot([0,-4,0],[0,4,0],2.4,8);
    }
}


module flower(){face();for(a=[0:60:300])translate([0,0,162])rotate([0,a,0]){translate([0,40,0])rotate([90,0,0])linear_extrude(3)hull(){translate([0,26])circle(r=10);translate([0,43])circle(r=10);}translate([0,23,26])rotate([-90,0,0])cylinder(r=4,h=17);}}
module starters(){turn_starters();for(x=[-37,37])starter_wall(x,-28.5,159,59);starter_wall(0,-55,159,80,90);starter_wall(0,19,134,66,90);
 for(x=[-22,25])starter_wall(x,-41,152,30);starter_wall(5.5,-36.5,152,38,90);for(y=[-8,8])starter_post(-73.5,y,137.5);
 starter_wall(0,-2,149,18);starter_post(-51.5,0,147);starter_post(0,-14.5,147);starter_post(0,10,154);
 pivot_ties(162,45);translate([-.35,-2.35,151.45])cube([.7,.7,.65]);
 starter_wall(0,38,109,14,90);for(x=[-22.5,22.5])starter_wall(x,38,165,5,90);starter_wall(0,38,178,5,90);for(x=[-37,37])starter_wall(x,38,130.5,10,90);
}
module sy0(){translate([0,0,38-gap])servo_mesh();}module sy1(){translate([-56,0,162])rotate([0,90,0])servo_mesh();}module sy2(){translate([0,-19,162])rotate([-90,0,0])servo_mesh();}
module h0(){translate([0,0,39])stock_horn();}module h1(){translate([-55,0,162])rotate([0,90,0])stock_horn();}module h2(){translate([0,-18,162])rotate([-90,0,0])stock_horn();}
module assembly(pre=false){base();if(pre)sy0();translate([0,0,pre?-gap:0])rotate([0,0,turn]){carrier();if(pre){sy1();h0();}about(P,bend){nod();if(pre){sy2();h1();}about(P,tilt,[0,1,0]){flower();if(pre)h2();}}}}
if(part=="preview")assembly(true);else if(part=="mechanism"){assembly();starters();}
else if(part=="base")base();else if(part=="carrier")carrier();else if(part=="nod")nod();else if(part=="face")flower();else if(part=="starters")starters();
else if(part=="servo0")sy0();else if(part=="servo1")sy1();else if(part=="servo2")sy2();else if(part=="horn0")h0();else if(part=="horn1")h1();else if(part=="horn2")h2();
