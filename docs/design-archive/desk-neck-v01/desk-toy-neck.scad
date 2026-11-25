/* Desk toy neck v0.1 -- exploratory print-in-place CAD, 2026-10-01.
   Units: mm. X left/right; +Y is face direction; Z up.
   One print contains four captive members and accessible sacrificial starters.
   FREE ALL JOINTS BY HAND BEFORE INSERTING/POWERING SERVOS.
   Default servo envelope: DFRobot SER0039, published listing 22.3 x 11.8 x 26.3 mm; drawing 23 x 12 x 30 mm.
   Drawing differs from listing; stock horn dimensions remain provisional: print fit gauge.
   This is not a physically tested or slicer-qualified production model.
   part = mechanism / base / yaw / pitch / face / starters / coupon / fit-gauge
          / servo-yaw / servo-pitch / servo-roll / horn-yaw / horn-pitch / horn-roll
   preview includes representative (not printable) servos and stock horns.
*/
part = "preview";
gap = 0.45;                  // RADIAL and axial design gap, not diametral
face_w = 70;
face_h = 46;
servo_l = 23;                // vendor drawing; listing gives 22.3 mm
servo_w = 12;
servo_h = 30;                // includes output shaft per vendor drawing
shaft_offset = 6;             // vendor dimensional drawing
ear_pitch = 29;               // vendor drawing; elongated slots allow fit tuning
mount_clearance = 0.55;       // per side
yaw_servo_drop = gap;         // match turn sleeve after it settles onto its thrust face
yaw_angle = 0;
pitch_angle = 0;
roll_angle = 0;
include_starters = true;
$fn = 64;
eps=0.02;
P=[0,0,91];
body_center=servo_l/2-shaft_offset;

module rounded_box(size,r=3) {
    translate([r,r,0]) linear_extrude(size[2])
        offset(r=r) square([size[0]-2*r,size[1]-2*r]);
}
module slot(a,b,d,h) { hull() { translate(a)cylinder(d=d,h=h); translate(b)cylinder(d=d,h=h); } }
module radial_slots(h=6,diam=2.0) {
    for(a=[0,90,180,270]) rotate([0,0,a]) slot([8,0,-eps],[13.5,0,-eps],diam,h);
}
module servo_mount() {
    difference() {
        translate([body_center-19,-10,-19]) cube([38,20,3]);
        translate([-shaft_offset-mount_clearance,-servo_w/2-mount_clearance,-20])
            cube([servo_l+2*mount_clearance,servo_w+2*mount_clearance,6]);
        // Side-entry opening lets the motor be installed after the captive print.
        // Oversized ear holes allow alignment to the freed, gravity-seated joint.
        translate([-shaft_offset-mount_clearance,0,-20]) cube([servo_l+2*mount_clearance,12,6]);
        for(x=[body_center-ear_pitch/2,body_center+ear_pitch/2])
            translate([x,0,-20])cylinder(d=2.8,h=6);
    }
}
module servo_envelope() {
    translate([-shaft_offset,-servo_w/2,-servo_h]) cube([servo_l,servo_w,servo_h-7.5]);
    translate([-shaft_offset-5,-servo_w/2,-16]) cube([servo_l+10,servo_w,2]);
    translate([0,0,-7.5]) cylinder(d=12,h=3.6);
    translate([0,0,-4]) cylinder(d=5,h=4);
}
module stock_horn() {
    difference() {
        union() {
            cylinder(d=8,h=2);
            hull(){translate([0,-14,0])cylinder(d=5,h=2);translate([0,14,0])cylinder(d=5,h=2);}
        }
        cylinder(d=2.0,h=4,center=true);
        for(y=[-12,-8,8,12])translate([0,y,-eps])cylinder(d=1.7,h=3);
    }
}
module horn_plate() { difference(){cylinder(r=15,h=3); cylinder(d=4,h=8,center=true);radial_slots();} }

// Axial profile for a captive shaft: 45-degree shoulders retain the moving member.
module shaft_z(r=7,lip=8.8,len=10) {
    rotate_extrude() polygon([[0,-len/2],[r,-len/2],[r,-1.8],[lip,0],
                             [r,1.8],[r,len/2],[0,len/2]]);
}
module socket_z(r=7,lip=8.8,len=10,outer=12.5,g=gap) {
    difference(){
        translate([0,0,-len/2])cylinder(r=outer,h=len);
        rotate_extrude()polygon([[0,-len/2-1],[r+g,-len/2-1],[r+g,-1.8-g],
            [lip+g,0],[r+g,1.8+g],[r+g,len/2+1],[0,len/2+1]]);
    }
}
module shaft_x(){rotate([0,90,0])shaft_z(len=10.2);}
module socket_x(){rotate([0,90,0])socket_z(len=9);}
module yaw_profile(g=0) {
    rotate_extrude()polygon([[0,4.6-g],[27+g,4.6-g],[27+g,9-g],
        [29+g,11],[27+g,13+g],[27+g,39+g],[0,39+g]]);
}

module base() {
    difference(){
        union(){
            translate([-49,-46,0])rounded_box([98,92,4],8);
            translate([0,0,4])cylinder(r=33.5,h=16);
        }
        yaw_profile(gap);
        // Interior is open; fixed motor pedestals are added inside the rotating sleeve.
        translate([0,0,3.9])cylinder(r=24-gap,h=24);
        translate([-shaft_offset-0.5,-servo_w/2-0.5,-eps])cube([servo_l+1,servo_w+1,6]);
        for(a=[45,135,225,315])rotate([0,0,a])translate([24,-7,4])cube([14,14,4]);
        for(x=[-40,40],y=[-37,37])translate([x,y,-eps])cylinder(d=3.4,h=6);
    }
    intersection(){translate([0,0,38-yaw_servo_drop])servo_mount();translate([0,0,18-yaw_servo_drop])cylinder(r=23.5,h=5);}
    for(x=[body_center-ear_pitch/2,body_center+ear_pitch/2])
        difference(){translate([x,0,3.8])cylinder(r=3,h=15.3-yaw_servo_drop);translate([x,0,3])cylinder(d=1.8,h=25);}
}
module yaw() {
    difference(){
        union(){
            difference(){yaw_profile();translate([0,0,4])cylinder(r=24,h=38);}
            difference(){translate([0,0,38.5])cylinder(r=31,h=5.5);translate([0,0,38])cylinder(r=24,h=3);}
            translate([-50,-15,38.5])cube([24,30,5.5]);
            translate([26,-15,38.5])cube([24,30,5.5]);
            for(x=[-45,45]) {
                translate([x-3,-15,42])cube([6,30,49]);
                translate([x,0,91])socket_x();
            }
            // Left servo bracket is outside the head's rotating envelope.
            translate([-56,0,91])rotate([0,90,0])servo_mount();
            for(y=[-9,9])translate([-75,y-2,72])cube([33.1,4,4]);
        }
        // Remove the arm material inside both bearing cavities as well.
        for(x=[-45,45])translate([x,0,91])rotate([0,90,0])
            rotate_extrude()polygon([[0,-6],[7+gap,-6],[7+gap,-1.8-gap],[8.8+gap,0],
                                     [7+gap,1.8+gap],[7+gap,6],[0,6]]);
        // Opening accepts the entire motor/flange from above; horn attaches across Y.
        translate([body_center-17.2,-6.8,38])cube([34.4,13.6,8]);
        translate([0,0,38])radial_slots(8);
    }
}
module pitch() {
    difference(){
        union(){
            for(x=[-45,45])translate([x,0,91])shaft_x();
            translate([-53,0,91])rotate([0,90,0])horn_plate();
            // Neck carriage: back and side ribs leave a clear path for the face.
            for(x=[-37,37])translate([x-3,-58,88])cube([6,59,6]);
            translate([-40,-58,88])cube([80,6,6]);
            translate([0,-19,91])rotate([-90,0,0])servo_mount();
            for(x=[-22,25])translate([x-2,-56,81])cube([4,30,10]);
            for(x=[-22,25])hull(){
                translate([x-2,-28,81])cube([4,4,4]);
                translate([x<0?-10:6,-4,81])cube([4,12,4]);
            }
            translate([0,-2,91])rotate([-90,0,0])socket_z(r=8,lip=10,len=18,outer=13);
        }
        // Stock roll horn and driven plate need space between the rear support ribs.
        translate([0,-16.5,91])rotate([-90,0,0])cylinder(r=15.5,h=4);
        // Cut through all support ribs to keep the roll bearing cavity clear.
        translate([0,-2,91])rotate([-90,0,0])rotate_extrude()polygon(
          [[0,-10],[8+gap,-10],[8+gap,-1.8-gap],[10+gap,0],
           [8+gap,1.8+gap],[8+gap,10],[0,10]]);
        // Empty shaft core preserves access to stock horn center screws.
        translate([-58,0,91])rotate([0,90,0])cylinder(d=4,h=20);
    }
}
module face() {
    difference(){
        union(){
            translate([0,-2,91])rotate([-90,0,0])shaft_z(r=8,lip=10,len=18);
            translate([0,-16,91])rotate([-90,0,0])horn_plate();
            translate([0,-13.1,91])rotate([-90,0,0])cylinder(r=8,h=3);
            translate([0,6,91])rotate([-90,0,0])cylinder(r=8,h=20);
            translate([-face_w/2,24,91-face_h/2])rotate([90,0,0])
                rounded_box([face_w,face_h,3],7);
            // Shallow perimeter tray; electronics stay close to the roll axis.
            for(x=[-face_w/2,face_w/2-3])translate([x,15,91-face_h/2+7])cube([3,9,face_h-14]);
            for(z=[91-face_h/2,91+face_h/2-3])translate([-face_w/2+7,15,z])cube([face_w-14,9,3]);
            // Rear spokes join the face rim to the roll hub while preserving the display aperture.
            for(x=[-1,1],z=[-1,1])hull(){
                translate([x*5-2,18,91+z*5-2])cube([4,6,4]);
                translate([x*(face_w/2-8)-2,18,91+z*(face_h/2-2)-2])cube([4,6,4]);
            }
        }
        translate([-17,20,82])cube([34,8,18]);
        translate([0,-18,91])rotate([-90,0,0])cylinder(d=4,h=46);
        for(x=[-25,25])translate([x,20,91])rotate([-90,0,0])slot([0,-4,0],[0,4,0],2.4,8);
    }
}

module starter_post(x,y,z0,z1) {
    // Deliberately removable supports, not bearing contact surfaces.
    translate([x-2,y-2,z0])cube([4,4,z1-z0-0.6]);
    translate([x-0.35,y-0.35,z1-0.6])cube([0.7,0.7,0.65]);
}
module starter_wall(x,y,z0,z1,along=0,length=10) {
    // Thin externally removable wall; bottom below a preceding layer avoids islands.
    translate([x,y,z0])rotate([0,0,along])translate([-0.3,-length/2,0])
        cube([0.6,length,z1-z0+0.05]);
}
module starters() {
    // Segmented ring supports the initial layer of the captive turn sleeve.
    difference(){
      translate([0,0,3.8])difference(){cylinder(r=25.6,h=0.85);cylinder(r=25,h=1);}
      for(a=[0:15:345])rotate([0,0,a])translate([24.8,-0.25,3.7])cube([1,0.5,1.2]);
    }
    for(x=[-43,43],y=[-12,12])starter_post(x,y,0,38.5);
    for(x=[-37,37])starter_wall(x,-28.5,0,88,0,59);
    starter_wall(0,-55,0,88,90,80);
    starter_wall(0,19,0,68,90,56);
    for(x=[-22,25])starter_wall(x,-41,0,81,0,30);
    starter_wall(5.5,-36.5,0,81,90,38);
    for(y=[-8,8])starter_post(-73.5,y,0,66.5);
    starter_wall(0,-2,0,78,0,18);
    starter_post(-51.5,0,0,76);
    starter_post(0,-14.5,0,76);
    starter_post(0,10,0,83);
    // Tiny breakaway bearing ties support the first suspended axle layers.
    for(x=[-45,45])translate([x-0.35,-0.35,81.65])cube([0.7,0.7,0.70]);
    translate([-0.35,-2.35,80.45])cube([0.7,0.7,0.65]);
}
module yaw_transform(settle=false){translate([0,0,settle?-yaw_servo_drop:0])rotate([0,0,yaw_angle])children();}
module pitch_transform(){translate(P)rotate([pitch_angle,0,0])translate(-P)children();}
module roll_transform(){translate(P)rotate([0,roll_angle,0])translate(-P)children();}
module mechanism(){
    base();yaw_transform(){yaw();pitch_transform(){pitch();roll_transform()face();}}
    if(include_starters)starters();
}
module preview(){
    color("#D4DFE2")base();
    yaw_transform(true){
        color("#4F93A0")yaw();
        color("#F5A44C")translate([0,0,39])stock_horn();
        color("#303A40")translate([-56,0,91])rotate([0,90,0])servo_envelope();
        pitch_transform()color("#F5A44C")translate([-55,0,91])rotate([0,90,0])stock_horn();
        pitch_transform(){
            color("#DEA15D")pitch();
            color("#303A40")translate([0,-19,91])rotate([-90,0,0])servo_envelope();
            roll_transform()color("#F5A44C")translate([0,-18,91])rotate([-90,0,0])stock_horn();
            roll_transform()color("#ECE6D9")face();
        }
    }
    color("#303A40")translate([0,0,38-yaw_servo_drop])servo_envelope();
}
module coupon_unit(g) {
    // This coupon reproduces the shaft shoulder, axial clearance and breakaway tie.
    translate([0,0,13])rotate([0,90,0])socket_z(g=g);
    translate([0,0,13])rotate([0,90,0])shaft_z();
    for(x=[-5,5])translate([x-3,-13,0])cube([6,26,2]);
    translate([-0.35,-0.35,13-8.8-g-0.05])cube([0.7,0.7,g+0.1]);
    translate([4.9,0,13])rotate([0,90,0])cylinder(r=4,h=4.1);
    translate([6,-1,11])cube([3,26,4]);
    translate([6,23,9])cube([3,6,8]);
    starter_post(7.5,25,0,9);
}
module coupon(){for(i=[0:3])translate([0,i*49,0]){
    coupon_unit([0.30,0.40,0.45,0.50][i]);
    translate([-20,-13,0])cube([12.1,10,2]);
    translate([-19,-11,2])linear_extrude(0.4)
        text(["0.30","0.40","0.45","0.50"][i],size=3.2,font="Arial");
}}
module fit_gauge(){
    translate([13,10,19])servo_mount();
    translate([58,10,0])horn_plate();
    translate([0,30,0])difference(){cube([34,18,5]);translate([4,2,-1])cube([servo_l+0.6,servo_w+0.6,8]);}
}

if(part=="preview")preview();
else if(part=="mechanism")mechanism();
else if(part=="base")base();
else if(part=="yaw")yaw();
else if(part=="pitch")pitch();
else if(part=="face")face();
else if(part=="starters")starters();
else if(part=="coupon")coupon();
else if(part=="fit-gauge")fit_gauge();
else if(part=="servo-yaw")translate([0,0,38-yaw_servo_drop])servo_envelope();
else if(part=="servo-pitch")translate([-56,0,91])rotate([0,90,0])servo_envelope();
else if(part=="servo-roll")translate([0,-19,91])rotate([-90,0,0])servo_envelope();
else if(part=="horn-yaw")translate([0,0,39])stock_horn();
else if(part=="horn-pitch")translate([-55,0,91])rotate([0,90,0])stock_horn();
else if(part=="horn-roll")translate([0,-18,91])rotate([-90,0,0])stock_horn();
