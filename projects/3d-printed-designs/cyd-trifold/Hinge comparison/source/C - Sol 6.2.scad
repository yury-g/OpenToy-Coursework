// C — Outward-cone hinge with through-vented cheeks. Dimensions in mm.
// OpenSCAD: use -D 'part="left"' or -D 'part="right"' for inspection exports.
// The default assembly pose has both flaps on Z=0. Positive fold lifts the right flap.
part = "assembly"; // [assembly,left,right]
fold = 0; // [0:1:180]

axis_z = 5.2;
rotor_half_width = 8.4;
axial_gap = 0.65;
cone_radius = 3.6;
cone_depth = 3.15;
cone_normal_gap = 0.50;
socket_vent_radius = 0.65;
first_layer_relief = 0.45;
cheek_outer_y = 16;
flap_thickness = 2.8;
label_depth = 0.64;
$fn = 96;

cheek_inner_y = rotor_half_width + axial_gap;
cone_tip_radius = cone_radius - cone_depth;
socket_offset = cone_normal_gap * sqrt(2);
socket_mouth_radius = cone_radius - axial_gap + socket_offset;
socket_cone_depth = socket_mouth_radius;
epsilon = 0.02;

assert(fold >= 0 && fold <= 180);
assert(cone_tip_radius > 0 && cone_normal_gap > 0 && axial_gap > 0);
assert(cone_depth > axial_gap + socket_offset, "Opposed bearings need residual engagement");
assert(first_layer_relief < axis_z - socket_mouth_radius);

// Extrude a profile expressed as [x,z] from y1 towards y0.
module xz_prism(y0, y1, outline) {
    translate([0,y1,0]) rotate([90,0,0])
        linear_extrude(height = y1-y0, convexity = 4) polygon(outline);
}

// Extrude a profile expressed as [y,z] along +X.
module yz_prism(x0, x1, outline) {
    multmatrix([[0,0,1,x0],[1,0,0,0],[0,1,0,0],[0,0,0,1]])
        linear_extrude(height = x1-x0) polygon(outline);
}

module axial_frustum(y, length, r0, r1) {
    translate([0,y,axis_z]) rotate([-90,0,0])
        cylinder(h = length, r1 = r0, r2 = r1);
}

module engraved_c() {
    outer_radius = 3.40;
    inner_radius = 2.15;
    lip_angle = 39;
    steps = 72;
    angles = [for (i = [0:steps]) lip_angle + i * (360-2*lip_angle)/steps];
    translate([-22,0,flap_thickness-label_depth])
        linear_extrude(height = label_depth+epsilon)
            polygon(concat(
                [for (a = angles) [outer_radius*cos(a),outer_radius*sin(a)]],
                [for (i = [steps:-1:0]) [inner_radius*cos(angles[i]),inner_radius*sin(angles[i])]]
            ));
}

module cheek() {
    difference() {
        // The lower edge of the inner face recedes by 0.45 mm at the bed.
        yz_prism(-10, 5.05, [
            [cheek_inner_y+first_layer_relief,0],
            [cheek_outer_y,0],
            [cheek_outer_y,10.2],
            [cheek_inner_y,10.2],
            [cheek_inner_y,first_layer_relief]
        ]);
        // The female cone is a parallel offset of the male at 45 degrees.
        axial_frustum(cheek_inner_y-epsilon, socket_cone_depth+epsilon,
                      socket_mouth_radius+epsilon, 0);
        // An open rear vent terminates the cone in a short 1.30 mm bore.
        axial_frustum(cheek_inner_y, cheek_outer_y-cheek_inner_y+epsilon,
                      socket_vent_radius, socket_vent_radius);
    }
}

module left() {
    difference() {
        union() {
            translate([-30,-cheek_outer_y,0])
                cube([21.8,2*cheek_outer_y,flap_thickness]);
            cheek();
            mirror([0,1,0]) cheek();
        }
        engraved_c();
    }
}

module positive_cone() {
    // Extend the root into the hub to avoid a coplanar joining face.
    axial_frustum(rotor_half_width-epsilon, cone_depth+epsilon,
                  cone_radius+epsilon, cone_tip_radius);
}

module right() {
    union() {
        difference() {
            xz_prism(-rotor_half_width, rotor_half_width, [
                [-3.75,0], [30,0], [30,flap_thickness],
                [8,flap_thickness], [4,9.15], [-3.75,9.15]
            ]);
            // Match the relief on the opposing cheek at both bed-level seams.
            for (side = [-1,1])
                scale([1,side,1])
                    yz_prism(-4,31,[
                        [rotor_half_width-first_layer_relief-epsilon,-epsilon],
                        [rotor_half_width+epsilon,-epsilon],
                        [rotor_half_width+epsilon,first_layer_relief+epsilon]
                    ]);
        }
        positive_cone();
        mirror([0,1,0]) positive_cone();
    }
}

module lift_right() {
    translate([0,0,axis_z]) rotate([0,-fold,0])
        translate([0,0,-axis_z]) right();
}

assert(part == "assembly" || part == "left" || part == "right");
if (part == "assembly" || part == "left") left();
if (part == "assembly" || part == "right") lift_right();
