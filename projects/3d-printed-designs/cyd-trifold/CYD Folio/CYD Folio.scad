// CYD S-fold folio. Units mm. Build direction is along the 86 mm PCB edge.
// Export "body", "keeper" or "coupon". "open", "closed" and "display" are views.
part = "body";
$fn = 64;
pcb_w = 50;
pcb_l = 86;
pcb_t = 1.5;
side_fit = 0.45;
rear_seat = 7.7;          // From outside back to rear PCB surface
hinge_radial_gap = 0.40;
hinge_axial_gap = 0.40;
face = 14;
front_axis = face+1.2;
back_axis = -1.2;
half = pcb_w/2+5;
ax = half+5.1;
pitch = 2*ax;
bottom = 2.4;
height = bottom+pcb_l+2.6;
roof = rear_seat+pcb_t+0.35;
wall = 1.8;
pin = 1.85;
barrel = 4.15;
lid_outer = front_axis-2.8;
lid_inner = face;
pack_depth = 2*front_axis - (2*back_axis - (2*front_axis-lid_outer));
print_wing = -84;
print_lid = 150;

module block(x1,x2,y1,y2,z1,z2) {
    translate([x1,y1,z1]) cube([x2-x1,y2-y1,z2-z1]);
}
module hinge_at(x,y,angle) {
    translate([x,y,0]) rotate([0,0,angle]) translate([-x,-y,0]) children();
}
module side_pair() { children(); mirror([1,0,0]) children(); }

// Corner seats and hooks occupy the exposed PCB beyond the display glass.
module corner_hardware() {
    edge = pcb_w/2;
    inner = edge+side_fit;
    // Backing pads at the two short ends. The upper pad grows at 45 degrees.
    block(edge-4,half,0,rear_seat,0,bottom+5);
    hull() {
        block(half-wall,half,0,rear_seat,height-15,height-14.8);
        block(edge-4,half,0,rear_seat,height-5.8,height-5.6);
    }
    block(edge-4,half,0,rear_seat,height-5.8,height);
    // Lower edge socket. The first 2.4 mm supports the PCB end.
    block(edge-4,half,0,roof+1.4,0,bottom);
    block(inner,half,rear_seat-0.1,roof+1.4,0,bottom+5);
    block(edge-1.1,half,roof,roof+1.4,0,bottom+5);
    // Two independent upper cantilevers; the outer wall shields each release arm.
    root_z = height-22;
    block(inner,half,7.9,12.2,root_z,root_z+2.6);
    block(inner,inner+1.45,8.15,12.2,root_z,height-3.6);
    translate([0,0,height-6.4]) linear_extrude(2.68)
        polygon([[edge-0.85,roof],[inner+1.38,roof],
                 [inner+1.22,12.05],[inner+0.04,12.05],[edge-0.85,roof+0.55]]);
    // Top end stop, independent of cantilever.
    block(edge-3.5,half,0,roof,height-2.05,height);
}
// Round the four exposed corners in the plane of the display.
module silhouette(rad=3.2) {
    translate([0,face+5,0]) rotate([90,0,0]) linear_extrude(face+10)
        offset(r=rad) translate([-half+rad,rad])
            square([2*(half-rad),height-2*rad]);
}
module shell(keeper_groove=false) {
    difference() {
        union() {
            block(-half,half,0,wall,0,height);
            side_pair() {
                block(half-wall,half,0,face,0,height);
                corner_hardware();
            }
        }
        // Cable overmould can extend behind the PCB on either USB variant.
        // The 42 mm opening leaves the corner seats and hinge rails intact.
        block(-21,21,-1,rear_seat,height-9.5,height+1);
        if(keeper_groove) block(-8,8,-0.1,0.75,7.15,11.4);
    }
}
module cover() {
    difference() {
        block(-half,half,lid_outer,lid_inner,0,height);
        // Finger scallop at the free edge makes it easy to lift the closed lid.
        translate([half+9,lid_inner+1,height*0.52]) rotate([90,0,0])
            cylinder(r=12,h=lid_inner-lid_outer+2);
        block(-8,8,lid_outer-0.1,lid_outer+0.75,7.15,11.4);
    }
}
module knuckle(side,is_front,z0,z1,cover_side=false) {
    y = is_front ? front_axis : back_axis;
    near_y = is_front ? face-1.3 : 1.3;
    hull() {
        translate([side*ax,y,z0]) cylinder(h=z1-z0,r=barrel);
        translate([side*(half-1),near_y,z0]) cylinder(h=z1-z0,r=1.0);
    }
}
module joint_half(side,is_front,shaft=false,cover_side=false) {
    y = is_front ? front_axis : back_axis;
    h = is_undef($test_height) ? height : $test_height;
    bands = [0,h/7,h*0.435,h*0.565,h*6/7,h];
    if(shaft) {
        translate([side*ax,y,0]) cylinder(h=h,r=pin);
        for(i=[0,2,4]) knuckle(side,is_front,bands[i],bands[i+1],cover_side);
    } else {
        difference() {
            union() for(i=[1,3])
                knuckle(side,is_front,bands[i]+hinge_axial_gap,
                        bands[i+1]-hinge_axial_gap,cover_side);
            translate([side*ax,y,-1]) cylinder(h=height+2,r=pin+hinge_radial_gap);
        }
    }
}
module leaf(n) {
    intersection() {silhouette(); if(n==3) cover(); else shell(n==0);}
    if(n==0) joint_half(1,true,true);
    if(n==1) {joint_half(-1,true); joint_half(1,false,true);}
    if(n==2) {joint_half(-1,false); joint_half(1,true,true);}
    if(n==3) joint_half(-1,true,false,true);
}
module locate(n,left,right,lid) {
    if(n==0) hinge_at(-ax,front_axis,left) translate([-pitch,0,0]) children();
    if(n==1) children();
    if(n==2) hinge_at(ax,back_axis,right) translate([pitch,0,0]) children();
    if(n==3) hinge_at(ax,back_axis,right) hinge_at(3*ax,front_axis,lid)
        translate([2*pitch,0,0]) children();
}
module folio(left,right,lid) {
    for(n=[0:3]) locate(n,left,right,lid) leaf(n);
}
// Slides over the middle of the short end after folding. Detents engage grooves.
module keeper() {
    clearance=0.35;
    span=pack_depth+2*clearance;
    t=1.8;
    module arm() {
        polygon([[0,0],[13.8,0],[13.8,t],[11.3,t],[10,t+0.95],
                 [8.4,t+0.95],[7.7,t],[0,t]]);
    }
    linear_extrude(13) union() {
        arm();
        translate([0,span+2*t,0]) mirror([0,1,0]) arm();
        translate([-2,0]) square([2,span+2*t]);
    }
}
module coupon() {
    // Same shaft, bore, axial gaps and print angle, shortened for a small trial.
    intersection() {
        let($test_height=20) union() for(n=[0:1]) locate(n,print_wing,0,0) leaf(n);
        translate([-ax,front_axis,0]) cylinder(h=10.8,r=15);
    }
}
// Same pockets and spring arms as the final trays. Lay its open back frame
// on the bed for a fast check of an unknown CYD revision and USB plug.
module board_gauge() {
    rotate([90,0,0]) difference() {
        intersection() {shell(); silhouette();}
        block(-half+6,half-6,-1,face+1,5,height-13);
        block(-half-1,half+1,2.5,face+1,bottom+6,height-22.1);
    }
}
if(part=="fit") board_gauge();
if(part=="body") folio(print_wing,print_wing,print_lid);
if(part=="keeper") keeper();
if(part=="coupon") coupon();
if(part=="open") folio(0,0,0);
if(part=="closed") folio(-180,-180,180);
if(part=="display") folio(-12,-12,125);
// Individual transformed leaves are also available for assembly checks.
if(part=="leaf") leaf(is_undef(which) ? 0 : which);
