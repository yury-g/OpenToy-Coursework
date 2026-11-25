// Upright annular-race hinge coupon. Millimeters. Bed is XY.
// Inspect left and right in these same world coordinates.
part = "assembly"; // [assembly,left,right]
$fn = 120;
stem = 3.0;
ridge = 1.7;         // Radial rise; 45 degree flanks
radial_gap = 0.55;   // Horizontal separation at equal height
relief = 0.30;       // Additional radial gap on EACH body at the bed
outer_r = 6.9;
ring_h = 9.4;
bridge_z = 10.2;
leaf_depth = 24;
assert(ridge > radial_gap && outer_r - stem - ridge - radial_gap > 1.2);
module turned(profile) { rotate_extrude() polygon(profile); }
module race_shape(extra=0, bore=false) {
    base = stem + extra;
    toe = bore ? relief : -relief;
    // Overshoot open ends on subtraction for clean end faces.
    turned(concat(bore ? [[0,-0.1],[base+toe,-0.1]] : [[0,0]],
         [[base+toe,0],[base,.4],[base,2.6],
          [base+ridge,2.6+ridge],[base+ridge,5.2],
          [base,5.2+ridge],[base,12.4],[0,12.4]]));
}
module left_leaf() {
    difference() {
        union() {
            race_shape();
            translate([-leaf_depth,1,0]) cube([leaf_depth-8,3,12.4]);
            // Load path above moving collar. Horizontal bridge between
            // hub and wing uses the same two anchors on every part.
            translate([-9,1,bridge_z]) cube([10,2.0,12.4-bridge_z]);
        }
        // Upright B engraved in the fixed wing's outward face.
        translate([-17,4.01,6]) rotate([90,0,180])
            translate([0,0,-.75]) linear_extrude(.8)
                text("B", size=6, halign="center", valign="center",
                     font="Liberation Sans:style=Bold");
    }
}
module right_leaf() {
    difference() {
        union() {
            turned([[0,0],[outer_r-.3,0],[outer_r,.4],
                    [outer_r,ring_h],[0,ring_h]]);
            translate([4,1,0]) cube([leaf_depth-4,3,ring_h]);
        }
        race_shape(radial_gap,true);
    }
}
if (part=="assembly" || part=="left") left_leaf();
if (part=="assembly" || part=="right") right_leaf();
