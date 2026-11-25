// Notebook C — Two outward-cone hinges with through-vented cheeks. Dimensions in mm.
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
cheek_outer_y = 14;
station_centers = [-17,17];
panel_half_width = 39;
panel_half_length = 33;
panel_outer_corner = 5.5;
panel_inner_corner = 3;
panel_top_round = 0.8;
cheek_outer_corner = 2;
cheek_inner_corner = 1;
cheek_top_round = 0.8;
rotor_profile_corner = 0.65;
fixed_panel_inner_x = 8.2;
moving_panel_inner_x = 11.8;
rotor_foot_thickness = 3;
flap_thickness = 2.8;
label_depth = 0.64;
label_center_x = -(panel_half_width+fixed_panel_inner_x)/2;
label_font = "Arial:style=Bold";
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
assert(station_centers[1] - station_centers[0] > 2*cheek_outer_y);
assert(max([for (y = station_centers) abs(y)])+cheek_outer_y < panel_half_length);
assert(moving_panel_inner_x > sqrt(10*10 + axis_z*axis_z),
       "The shared moving panel must clear the backs of the cheeks");

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
    outer_radius = 4.50;
    inner_radius = 2.95;
    lip_angle = 39;
    steps = 72;
    angles = [for (i = [0:steps]) lip_angle + i * (360-2*lip_angle)/steps];
    translate([label_center_x,11,flap_thickness-label_depth])
        linear_extrude(height = label_depth+epsilon)
            polygon(concat(
                [for (a = angles) [outer_radius*cos(a),outer_radius*sin(a)]],
                [for (i = [steps:-1:0]) [inner_radius*cos(angles[i]),inner_radius*sin(angles[i])]]
            ));
}

module engraved_line(content, y, size) {
    translate([label_center_x,y,flap_thickness-label_depth])
        linear_extrude(height = label_depth+epsilon)
            offset(delta = 0.06)
                text(content, size = size, font = label_font,
                     halign = "center", valign = "center");
}

// Corners are ordered bottom-right, top-right, top-left, bottom-left.
// The first two rings have the full outline, giving a flat sole and vertical skirt.
function rounded_ring(x0, x1, y0, y1, radii, z, inset) =
    let(centers = [[x1-radii[0], y0+radii[0]],
                   [x1-radii[1], y1-radii[1]],
                   [x0+radii[2], y1-radii[2]],
                   [x0+radii[3], y0+radii[3]]])
    [for (corner = [0:3], step = [0:12])
        let(a = -90 + 90*corner + 90*step/12, r = radii[corner]-inset)
        [centers[corner][0]+r*cos(a),centers[corner][1]+r*sin(a),z]];

module bed_flat_rounded_box(x0, x1, y0, y1, h, radii, top_radius) {
    assert(min(radii) > top_radius && h > top_radius);
    rings = concat(
        [rounded_ring(x0,x1,y0,y1,radii,0,0)],
        [for (a = [0:15:90])
            rounded_ring(x0,x1,y0,y1,radii,h-top_radius+top_radius*sin(a),
                         top_radius*(1-cos(a)))]);
    ring_size = len(rings[0]);
    cap_start = (len(rings)-1)*ring_size;
    sides = [for (k = [0:len(rings)-2], i = [0:ring_size-1])
        let(j = (i+1)%ring_size, lower = k*ring_size, upper = (k+1)*ring_size)
        each [[lower+i,upper+i,upper+j],[lower+i,upper+j,lower+j]]];
    polyhedron([for (ring = rings) each ring],
               concat([[for (i = [0:ring_size-1]) i]], sides,
                      [[for (i = [ring_size-1:-1:0]) cap_start+i]]),
               convexity = 4);
}

// Round a convex corner in the XZ profile without extending past the original sides.
function corner_arc(previous, point, next, radius) =
    let(incoming = (point-previous)/norm(point-previous),
        outgoing = (next-point)/norm(next-point),
        turn = atan2(incoming.x*outgoing.y-incoming.y*outgoing.x, incoming*outgoing),
        tangent = point-incoming*radius*tan(turn/2),
        center = tangent+radius*[-incoming.y,incoming.x],
        start = atan2(-incoming.x,incoming.y))
    [for (step = [0:8])
        let(a = start+turn*step/8) center+radius*[cos(a),sin(a)]];

module cheek() {
    difference() {
        bed_flat_rounded_box(-10,5.05,cheek_inner_y,cheek_outer_y,10.2,
            [cheek_inner_corner,cheek_outer_corner,cheek_outer_corner,cheek_inner_corner],
            cheek_top_round);
        // Keep the original first-layer seam relief.
        yz_prism(-11,6,[
            [cheek_inner_y-epsilon,-epsilon],
            [cheek_inner_y+first_layer_relief+epsilon,-epsilon],
            [cheek_inner_y-epsilon,first_layer_relief+epsilon]
        ]);
        // The female cone is a parallel offset of the male at 45 degrees.
        axial_frustum(cheek_inner_y-epsilon, socket_cone_depth+epsilon,
                      socket_mouth_radius+epsilon, 0);
        // An open rear vent terminates the cone in a short 1.30 mm bore.
        axial_frustum(cheek_inner_y, cheek_outer_y-cheek_inner_y+epsilon,
                      socket_vent_radius, socket_vent_radius);
    }
}

// Positive-X leaf blank. A broader plan radius softens the free outer edge.
module panel(inner_x) {
    bed_flat_rounded_box(inner_x,panel_half_width,-panel_half_length,
        panel_half_length,flap_thickness,
        [panel_outer_corner,panel_outer_corner,panel_inner_corner,panel_inner_corner],
        panel_top_round);
}

module left() {
    difference() {
        union() {
            mirror([1,0,0]) panel(fixed_panel_inner_x);
            for (y = station_centers)
                translate([0,y,0]) {
                    cheek();
                    mirror([0,1,0]) cheek();
                }
        }
        engraved_c();
        engraved_line("Sol 6.2", 0, 5);
        engraved_line("2026-11-24", -8, 4);
    }
}

module positive_cone() {
    // Extend the root into the hub to avoid a coplanar joining face.
    axial_frustum(rotor_half_width-epsilon, cone_depth+epsilon,
                  cone_radius+epsilon, cone_tip_radius);
}

// Each rotor foot overlaps the shared right panel at bed level.
module rotor() {
    union() {
        difference() {
            foot_x = moving_panel_inner_x+1.8;
            outline = [[-3.75,0], [foot_x,0], [foot_x,rotor_foot_thickness],
                       [8,rotor_foot_thickness], [4,9.15], [-3.75,9.15]];
            xz_prism(-rotor_half_width, rotor_half_width, [
                for (i = [0:5])
                    if (i == 2 || i >= 4)
                        each corner_arc(outline[i-1],outline[i],outline[(i+1)%6],
                                        rotor_profile_corner)
                    else outline[i]
            ]);
            // Match the relief on the opposing cheek at both bed-level seams.
            for (side = [-1,1])
                scale([1,side,1])
                    yz_prism(-4,moving_panel_inner_x+2,[
                        [rotor_half_width-first_layer_relief-epsilon,-epsilon],
                        [rotor_half_width+epsilon,-epsilon],
                        [rotor_half_width+epsilon,first_layer_relief+epsilon]
                    ]);
        }
        positive_cone();
        mirror([0,1,0]) positive_cone();
    }
}

module right() {
    union() {
        panel(moving_panel_inner_x);
        for (y = station_centers)
            translate([0,y,0]) rotor();
    }
}

module lift_right() {
    translate([0,0,axis_z]) rotate([0,-fold,0])
        translate([0,0,-axis_z]) right();
}

assert(part == "assembly" || part == "left" || part == "right");
if (part == "assembly" || part == "left") left();
if (part == "assembly" || part == "right") lift_right();
