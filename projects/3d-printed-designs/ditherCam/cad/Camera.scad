// ============================================================================
// Camera.scad — Parametric Print-in-Place Freestanding Folding DIY Camera
// Revision: Integral Stepped Pod Faceplate (Camera Lens + Recessed OLED) &
//           RLCD SNAP / PWR / BROWSE Button Labels & Dual-Pose Access
//
// Target: Waveshare 4.2" RLCD (Landscape, stock stand removed) +
//         Seeed XIAO ESP32-S3 Sense + Expansion Base + 58x42.5x4 mm LiPo Pod
// Printer: Snapmaker U1, PLA, 0.4 mm nozzle, 0.2 mm layers, NO supports/raft/screws
//
// COORDINATE SYSTEMS:
//   1. PRINT POSE (pose = "print", Z_print = 0 is PEI build plate):
//      - Hinge axis along X at (Y = 0, Z = axis_z = 22.00 mm).
//      - Stationary `body` lies front-face-down on Z = 0, Y in [22.04, y_sole = 104.50].
//        Tabletop sole is the flat coplanar square-edged wall Y = y_sole, Z in [0, sole_depth].
//      - Captive `pod` prints at fold = 90 deg on Z = 0, X in [-33.0, +33.0],
//        captured between two rigid outboard vented female-cone cheeks on `body`
//        (|X| in [33.65, 40.50]).
//   2. TABLETOP USE POSE (pose = "use", Z_use = 0 is flat tabletop):
//      - Rigid right-handed transform from print pose:
//          X_use = X_print
//          Y_use = Z_print          (-Y_use = Viewer / RLCD Screen side; +Y_use = Scene / Rear)
//          Z_use = y_sole - Y_print (Z_use = 0 is coplanar foot sole; Hinge at Y_use=22.0, Z_use=104.5)
//      - fold = 0 deg   : FORWARD MODE (Camera lens points +Y_use, away from RLCD viewer)
//      - fold = 90 deg  : MID-FOLD / PRINT POSE (Camera lens points +Z_use, straight up)
//      - fold = 180 deg : SELFIE MODE (Camera lens points -Y_use, toward RLCD viewer above screen)
// ============================================================================

/* [Export & View Controls] */
part = "print_assembly"; // [print_assembly, body, pod, use_assembly, hw_rlcd, hw_xiao_pod, hw_xiao_stack, hw_oled_active, hw_lens_cone]
pose = "print";          // [print, use]
fold = 90;               // [-5:1:185]
show_hw = false;
axial_test_shift = 0.0;

/* [Hinge Parameters (Vented Opposing 45-deg Outboard Cone Pivot)] */
axis_z = 22.00;
y_sole = 104.50;
axial_gap = 0.65;
cone_normal_gap = 0.50;
cone_radius = 3.80;
cone_depth = 3.25;
socket_vent_radius = 0.70;
first_layer_relief = 0.45;
cheek_width = 6.85;
rotor_hub_r = 5.00;
cheek_top_r = 6.40;

/* [Main Body & Landscape 4.2" RLCD Parameters] */
body_width = 108.00;
sole_depth = 58.00;
body_corner_r = 3.20;
front_bezel_chamfer = 0.90;

// RLCD Landscape Geometry (from reference/rlcd-v2.stp & .pdf, stock stand removed)
rlcd_pcb_w = 92.52;         // Z_stp: [-44.66, +47.86]
rlcd_pcb_h = 69.12;         // X_stp: [-34.56, +34.56]
rlcd_base_h = 70.10;        // X_stp: [-35.55, +34.55]
rlcd_act_w = 84.80;         // Active LCD landscape width, centered on X = 0
rlcd_act_h = 63.60;         // Active LCD landscape height
rlcd_tft_w = 91.83;         // 042MN TFT glass width: [-43.90, +47.93]
rlcd_tft_h = 67.60;         // 042MN TFT glass height: [-33.80, +33.80]
rlcd_xy_clr = 0.80;         // Per-side XY pocket clearance
rlcd_center_z_use = 41.50;  // Active LCD center height above tabletop Z_use = 0 -> Y_print = 63.00
rlcd_tft_front_z = 5.15;    // Z_print (= Y_use) of TFT front glass (>= 0.80 mm clear of bezel everywhere!)
rlcd_bare_v2_t = 14.10;     // V2 front TFT to rear acrylic back
rlcd_bare_v1_t = 13.50;     // V1 front TFT to rear acrylic back

/* [Top Pod, Integral Stepped Faceplate & XIAO/LiPo Parameters] */
pcb_len = 58.00;
pcb_wid = 42.50;
pcb_corner_r = 3.00;
pcb_thick = 1.60;
board_xy_clearance = 0.30;
header_stack_h = 7.60;
camera_lens_min_above_base = 18.90;
camera_max_above_base = 23.16;
underside_clearance = 4.20; // Clears 3.20 mm CR1220 RTC holder + 1.00 mm air gap
divider_thick = 1.40;       // Integral continuous vertical insulating barrier
battery_len = 58.00;
battery_wid = 42.50;
battery_max_thick = 4.00;
battery_bay_depth = 6.00;   // Smooth 6.0 mm pocket (never squeezes 4.0 mm pouch)
battery_xy_clearance = 0.60;
spring_len = 20.00;         // Baseline in-layer compliant retainer length
spring_thick = 1.50;        // Baseline in-layer compliant retainer thickness

// Integral Stepped Faceplate & Recessed OLED Window Parameters (Eagle Z_e heights above base bottom)
face_cam_inner_ze = 23.40;  // Clears camera_max_above_base (23.16) for top-down PCB slide-in
face_cam_thick = 2.00;      // Camera faceplate thickness -> outer at z_e = 25.40
face_oled_inner_ze = 15.00; // Clears Sense PCB (14.63), J7 (10.10), Grove (7.80), OLED glass (3.45)
face_oled_thick = 2.20;     // OLED stepped faceplate thickness -> outer at z_e = 17.20 (8.2 mm recessed behind lens!)
oled_bezel_throat_ze = 10.00;// Stepped collar back plane (6.55 mm clear of OLED glass, 2.20 mm clear of Grove)
oled_throat_margin_x = 1.85;// Per-side margin around 21.74 mm active OLED -> 25.44 mm throat (clears +/-15 deg FOV!)
oled_throat_margin_y = 1.45;// Per-side margin around 10.86 mm active OLED -> 13.76 mm throat
oled_flare_xy = 3.90;       // Outward flare across 10.2 mm bezel depth (left outer -10.82, clean of -11.0 Camera step)
lens_window_extra_r = 0.55; // Extra radial margin outside 120 deg cone from 18.90 mm

$fn = 64;
eps = 0.02;

// ============================================================================
// DERIVED CONSTANTS & DESIGN ASSERTS
// ============================================================================
pod_half_x = pcb_len/2 + board_xy_clearance + 3.70; // 33.00 mm (66.0 mm wide pod)
cheek_inner_x = pod_half_x + axial_gap;             // 33.65 mm
cheek_outer_x = cheek_inner_x + cheek_width;        // 40.50 mm
cone_tip_radius = cone_radius - cone_depth;         // 0.55 mm
socket_offset = cone_normal_gap * sqrt(2);          // 0.7071 mm
socket_mouth_radius = cone_radius - axial_gap + socket_offset; // 3.8571 mm
socket_cone_depth = socket_mouth_radius;

assert(cone_tip_radius > 0.2, "Cone tip must stay positive");
assert(cone_depth > axial_gap + socket_offset + 1.0, "Opposed cones need positive residual capture");
assert(battery_bay_depth >= battery_max_thick + 1.5, "Battery bay must not clamp pouch");
assert(underside_clearance >= 3.8, "Underside clearance must clear 3.2 mm RTC holder");

// RLCD pocket bounds in Print Pose (X = X_use, Y = y_sole - Z_use, Z = Y_use)
y_lcd_center = y_sole - rlcd_center_z_use;     // 104.5 - 41.5 = 63.00
rlcd_x0 = -44.66 - rlcd_xy_clr;                // -45.46
rlcd_x1 =  47.93 + rlcd_xy_clr;                // +48.73
rlcd_y0 = y_lcd_center - 34.56 - rlcd_xy_clr;  // 63.00 - 35.36 = 27.64 (top edge in use)
rlcd_y1 = y_lcd_center + 35.55 + rlcd_xy_clr;  // 63.00 + 36.35 = 99.35 (bottom edge in use)

z_rlcd_pmma_front = rlcd_tft_front_z + 0.75;           // 5.90
z_rlcd_pcb_front  = rlcd_tft_front_z + 2.75;           // 7.90
z_rlcd_v1_back    = rlcd_tft_front_z + rlcd_bare_v1_t; // 18.65
z_rlcd_v2_back    = rlcd_tft_front_z + rlcd_bare_v2_t; // 19.25

// Selfie stop shelf: at fold=180, pod bottom face (Z_p=0, dz=-axis_z) rotates to Y_p = +axis_z = 22.00
y_selfie_shelf = axis_z + 0.04; // 22.04 (Z_use = 82.46, 5.6 mm crown margin above RLCD PCB top!)

// Pod bounds in Print Pose (fold = 90 deg)
z_e_rear_wall = -(underside_clearance + divider_thick + battery_bay_depth + 1.80); // -13.40
pod_inner_y = -8.80;
y_p_pcb0 = pod_inner_y + z_e_rear_wall; // -22.20
z_e_front_cam = face_cam_inner_ze + face_cam_thick; // 25.40
z_e_front_oled = face_oled_inner_ze + face_oled_thick; // 13.20
pod_front_y = y_p_pcb0 - z_e_front_cam; // -47.60
pod_oled_front_y = y_p_pcb0 - z_e_front_oled; // -35.40
z_p_pcb0 = 2.60;
pod_top_z = z_p_pcb0 + pcb_wid + 3.10; // 48.20

// Forward stop buttress Z_print (= Y_use)
z_forward_stop = axis_z - pod_inner_y - 0.04; // 30.76

// Eagle OLED & Lens Print-Pose Coordinates
oled_act_w = 21.74;
oled_act_h = 10.86;
oled_cx_p = 34.80 - pcb_len/2;         // +5.80
oled_cz_p = z_p_pcb0 + 21.35;          // 23.95
lens_cx_p = 3.85 - pcb_len/2;          // -25.15
lens_cz_p = z_p_pcb0 + 21.25;          // 23.85

// ============================================================================
// GEOMETRY HELPERS
// ============================================================================
module rrect2d(cx, cy, w, h, r) {
    rr = max(0.05, min(r, min(w, h)/2 - 0.05));
    translate([cx, cy])
        hull()
            for (sx = [-1, 1], sy = [-1, 1])
                translate([sx*(w/2 - rr), sy*(h/2 - rr)])
                    circle(r = rr);
}

module rbox(x0, x1, y0, y1, z0, z1, r=1.0) {
    if (x1 > x0 + 0.02 && y1 > y0 + 0.02 && z1 > z0 + 0.01)
        translate([0, 0, z0])
            linear_extrude(height = z1 - z0, convexity = 4)
                rrect2d((x0+x1)/2, (y0+y1)/2, x1-x0, y1-y0, r);
}

module yz_prism(x0, x1, pts) {
    if (x1 > x0 + 0.01)
        translate([x0, 0, 0])
            rotate([90, 0, 90])
                linear_extrude(height = x1 - x0, convexity = 6)
                    polygon(pts);
}

module xz_prism(y0, y1, pts) {
    if (y1 > y0 + 0.01)
        translate([0, y1, 0])
            rotate([90, 0, 0])
                linear_extrude(height = y1 - y0, convexity = 6)
                    polygon(pts);
}

module rfrustum(cx0, cy0, w0, h0, r0, z0, cx1, cy1, w1, h1, r1, z1) {
    hull() {
        translate([0, 0, z0])
            linear_extrude(height = 0.04)
                rrect2d(cx0, cy0, w0, h0, r0);
        translate([0, 0, z1 - 0.04])
            linear_extrude(height = 0.04)
                rrect2d(cx1, cy1, w1, h1, r1);
    }
}

// Extrude an Eagle [x_e, y_e] rounded rectangle along z_e (which maps to Y_print)
module pod_board_prism(x0, x1, y0, y1, z0, z1, r=0.8) {
    xp0 = x0 - pcb_len/2;
    xp1 = x1 - pcb_len/2;
    yp0 = y_p_pcb0 - z1;
    yp1 = y_p_pcb0 - z0;
    zp0 = z_p_pcb0 + y0;
    zp1 = z_p_pcb0 + y1;
    if (xp1 > xp0 + 0.02 && yp1 > yp0 + 0.01 && zp1 > zp0 + 0.02)
        translate([0, yp1, 0])
            rotate([90, 0, 0])
                linear_extrude(height = yp1 - yp0, convexity = 4)
                    rrect2d((xp0+xp1)/2, (zp0+zp1)/2, xp1-xp0, zp1-zp0, r);
}

// ============================================================================
// SUBASSEMBLY 1: STATIONARY MAIN BODY (`body`) IN PRINT POSE
// ============================================================================

module body_outboard_cheek(sx) {
    xa = (sx > 0) ? cheek_inner_x : -cheek_outer_x;
    xb = (sx > 0) ? cheek_outer_x : -cheek_inner_x;

    difference() {
        hull() {
            yz_prism(xa, xb, [
                [-cheek_top_r + 0.8, 0],
                [23.50, 0],
                [23.50, z_forward_stop],
                [6.00, z_forward_stop],
                [-cheek_top_r, axis_z - 2.0],
                [-cheek_top_r, 0.8]
            ]);
            translate([xa, 0, axis_z])
                rotate([0, 90, 0])
                    cylinder(h = xb - xa, r = cheek_top_r);
        }

        if (sx > 0)
            xz_prism(-cheek_top_r - 1, 25, [
                [xa - eps, -eps], [xa + first_layer_relief + eps, -eps], [xa - eps, first_layer_relief + eps]
            ]);
        else
            xz_prism(-cheek_top_r - 1, 25, [
                [xb + eps, -eps], [xb - first_layer_relief - eps, -eps], [xb + eps, first_layer_relief + eps]
            ]);
        yz_prism(xa - eps, xb + eps, [
            [-cheek_top_r - eps, -eps], [-cheek_top_r + first_layer_relief + 1.2, -eps], [-cheek_top_r - eps, first_layer_relief + 1.2]
        ]);

        if (sx > 0) {
            translate([xa - eps, 0, axis_z])
                rotate([0, 90, 0])
                    cylinder(h = socket_cone_depth + eps, r1 = socket_mouth_radius + eps, r2 = 0);
        } else {
            translate([xb + eps, 0, axis_z])
                rotate([0, -90, 0])
                    cylinder(h = socket_cone_depth + eps, r1 = socket_mouth_radius + eps, r2 = 0);
        }
        translate([xa - eps, 0, axis_z])
            rotate([0, 90, 0])
                cylinder(h = (xb - xa) + 2*eps, r = socket_vent_radius);
    }
}

module rlcd_retainer_arm(x_tip, dir_x, y_wall, dir_y) {
    x0 = min(x_tip, x_tip + dir_x * spring_len);
    x1 = max(x_tip, x_tip + dir_x * spring_len);
    ya = min(y_wall, y_wall + dir_y * spring_thick);
    yb = max(y_wall, y_wall + dir_y * spring_thick);
    z_arm_top = z_rlcd_v2_back + 3.20;

    union() {
        rbox(x0, x1, ya, yb, 0, z_arm_top, 0.45);
        x_root0 = (dir_x > 0) ? (x1 - 2.0) : (x0 - 3.5);
        x_root1 = (dir_x > 0) ? (x1 + 3.5) : (x0 + 2.0);
        ya_r = min(y_wall - dir_y * 0.5, y_wall + dir_y * 4.5);
        yb_r = max(y_wall - dir_y * 0.5, y_wall + dir_y * 4.5);
        rbox(x_root0, x_root1, ya_r, yb_r, 0, z_arm_top, 0.5);

        xh0 = (dir_x > 0) ? x0 : (x1 - 5.2);
        xh1 = (dir_x > 0) ? (x0 + 5.2) : x1;
        reach = 1.05;
        z_hook_seat = z_rlcd_v2_back + 0.40;
        if (dir_y > 0) {
            yz_prism(xh0, xh1, [
                [y_wall + 0.4, z_hook_seat - reach - 0.45],
                [y_wall - reach, z_hook_seat],
                [y_wall - reach, z_hook_seat + 0.85],
                [y_wall + 0.4, z_arm_top]
            ]);
        } else {
            yz_prism(xh0, xh1, [
                [y_wall - 0.4, z_hook_seat - reach - 0.45],
                [y_wall + reach, z_hook_seat],
                [y_wall + reach, z_hook_seat + 0.85],
                [y_wall - 0.4, z_arm_top]
            ]);
        }
    }
}

// Integrated button label for RLCD top crown (`Z_print = 22.5`, facing +Z_print = +Y_use rear):
// KEY4 (X=-8.40) = GPIO18 = SNAP; KEY3 (X=+1.60) = PWR; KEY1 (X=+11.60) = GPIO0 = BROWSE
module rlcd_crown_button_labels() {
    // Raised 0.60 mm text on upward-facing crown shelf (Z_print = 22.50..23.10) -> ZERO floating islands & 100% legible!
    // Left label: SNAP centered at X = -12.5, leader tick to KEY4 (X = -8.40)
    // Center label: PWR centered at X = +1.60 (KEY3)
    // Right label: BROWSE centered at X = +15.8, leader tick to KEY1 (X = +11.60)
    z_base = 22.45;
    h_txt = 0.65;
    // Slight backing bar on Y in [22.2, 27.4] is already solid from main frame; mirror X?
    // Wait: when looking from rear (+Z_print toward -Z_print), +X is to the RIGHT!
    // No mirror needed when reading from rear (+Y_use looking at the back): X increases left-to-right if rotated 180?
    // Let's check! In Use Pose: +X_use = right when looking at rear (+Y_use -> -Y_use): NO, +X_use is RIGHT when looking at the FRONT (-Y_use -> +Y_use)!
    // Let's check orientation:
    //   X_use = +X_print, Y_use = +Z_print (rear), Z_use = y_sole - Y_print (up).
    //   When a viewer stands in FRONT looking at the screen (-Y_use looking in +Y_use direction, Z_use up):
    //   Right-hand rule: +Y_use (forward) x +Z_use (up) = +X_use (RIGHT!).
    //   So looking from FRONT: -X is LEFT, +X is RIGHT!
    //   Looking from REAR (+Y_use looking in -Y_use direction, Z_use up): +X is LEFT, -X is RIGHT!
    //   On the rear crown shelf (Y_print in [22.1, 27.3], Z_print = 22.5 facing +Y_use):
    //   Viewing from rear/top looking at +Z_print with Z_use up (-Y_print up):
    //   X goes RIGHT-TO-LEFT, so rotate([0, 0, 180]) makes text read cleanly left-to-right from the rear!
    translate([0, 0, z_base])
        linear_extrude(height = h_txt, convexity = 4) {
            // Leader bars pointing straight from labels at Y=24.6 to button centers at Y=27.2!
            translate([-8.40, 26.2]) square([0.85, 2.2], center = true);
            translate([ 1.60, 26.2]) square([0.85, 2.2], center = true);
            translate([11.60, 26.2]) square([0.85, 2.2], center = true);
            // Labels read right-side-up when looking from rear/above (Z_use up = -Y_print up, so rotate 180 in XY_print!)
            rotate([0, 0, 180]) {
                translate([ 12.2, -23.8]) text("SNAP",   size = 2.6, font = "Arial:style=Bold", halign = "center", valign = "center");
                translate([ -1.6, -23.8]) text("PWR",    size = 2.6, font = "Arial:style=Bold", halign = "center", valign = "center");
                translate([-15.8, -23.8]) text("BROWSE", size = 2.6, font = "Arial:style=Bold", halign = "center", valign = "center");
            }
        }
}

// Front-face stencil-bridged button labels on Z_print = 0 (facing viewer -Y_use above RLCD screen!)
// Uses 0.55 mm horizontal stencil bridges across engraved glyphs so zero floating islands exist on Z = 0!
module rlcd_front_button_engravings() {
    difference() {
        union() {
            translate([-8.40, 24.4, -eps]) cube([0.90, 2.8, 0.48], center = true);
            translate([ 1.60, 24.4, -eps]) cube([0.90, 2.8, 0.48], center = true);
            translate([11.60, 24.4, -eps]) cube([0.90, 2.8, 0.48], center = true);
            // Viewed from front (-Z_print looking in +Z_print, Z_use up = -Y_print up):
            // mirror([0, 1, 0]) keeps X left-to-right and flips Y so -Y_print is UP!
            translate([0, 0, -eps])
                linear_extrude(height = 0.48, convexity = 4)
                    mirror([0, 1, 0]) {
                        translate([-12.5, -26.8]) text("SNAP",   size = 2.7, font = "Arial:style=Bold", halign = "center", valign = "center");
                        translate([  1.6, -26.8]) text("PWR",    size = 2.7, font = "Arial:style=Bold", halign = "center", valign = "center");
                        translate([ 15.8, -26.8]) text("BROWSE", size = 2.7, font = "Arial:style=Bold", halign = "center", valign = "center");
                    }
        }
        // Stencil bridges keeping closed loops (A, P, R, B, O) connected to Z = 0!
        translate([0, 26.8, 0.20]) cube([48.0, 0.52, 0.50], center = true);
    }
}

module main_body_print() {
    bx = body_width / 2; // 54.00
    z_frame = 22.50;

    union() {
        difference() {
            union() {
                // 1. Main RLCD front frame & cradle block
                hull() {
                    rbox(-bx + front_bezel_chamfer, bx - front_bezel_chamfer,
                         y_selfie_shelf + front_bezel_chamfer, y_sole, 0, 0.05, body_corner_r);
                    rbox(-bx, bx, y_selfie_shelf, y_sole, front_bezel_chamfer, z_frame, body_corner_r);
                }

                // 2. Rigid Forward Hard-Stop Piers at |X| in [28.8, 40.5], Y in [y_selfie_shelf, 29.0], Z in [0, z_forward_stop]
                for (sx = [-1, 1]) {
                    xa = (sx > 0) ? 28.80 : -40.50;
                    xb = (sx > 0) ? 40.50 : -28.80;
                    hull() {
                        rbox(xa + front_bezel_chamfer, xb - front_bezel_chamfer,
                             y_selfie_shelf + front_bezel_chamfer, 29.0, 0, 0.05, 1.2);
                        rbox(xa, xb, y_selfie_shelf, 29.0, front_bezel_chamfer, z_forward_stop, 1.2);
                    }
                }

                // 3. Broad Tabletop Foot Sole Wall (Y in [y_sole - 2.4, y_sole], Z in [0, sole_depth = 58.0])
                hull() {
                    rbox(-bx + front_bezel_chamfer, bx - front_bezel_chamfer, y_sole - 2.3, y_sole, 0, 0.05, 0.8);
                    rbox(-bx, bx, y_sole - 2.4, y_sole, front_bezel_chamfer, sole_depth, 0.8);
                }

                // 4. Left & Right Structural Side Buttresses (<=45 deg slope)
                for (sx = [-1, 1]) {
                    x_b0 = (sx < 0) ? -bx : (rlcd_x1 + 0.40);
                    x_b1 = (sx < 0) ? (rlcd_x0 - 0.40) : bx;
                    yz_prism(x_b0, x_b1, [
                        [y_sole, front_bezel_chamfer],
                        [y_sole, sole_depth],
                        [y_sole - 10.0, sole_depth],
                        [y_sole - 10.0 - (sole_depth - z_frame), z_frame],
                        [y_selfie_shelf + 1.0, z_frame],
                        [y_selfie_shelf + 1.0, front_bezel_chamfer]
                    ]);
                }

                // 5. Three Central Sole Stiffening Gusset Ribs
                for (rx = [-16.0, 0.0, 16.0])
                    yz_prism(rx - 2.2, rx + 2.2, [
                        [y_sole, z_frame - 1.0],
                        [y_sole, sole_depth],
                        [y_sole - 2.4, sole_depth],
                        [y_sole - 2.4 - (sole_depth - z_frame) * 0.12, z_frame - 1.0]
                    ]);
            }

            // --- SUBTRACTIONS ON MAIN BODY ---

            // A. Active RLCD Viewing Window
            rfrustum(0, y_lcd_center, rlcd_act_w + 2.6, rlcd_act_h + 2.6, 1.8, -eps,
                     0, y_lcd_center, rlcd_act_w + 0.6, rlcd_act_h + 0.6, 1.0, 1.15);
            rbox(-(rlcd_act_w/2 + 0.30), rlcd_act_w/2 + 0.30,
                 y_lcd_center - (rlcd_act_h/2 + 0.30), y_lcd_center + (rlcd_act_h/2 + 0.30),
                 1.05, 1.65, 1.0);
            rfrustum(0, y_lcd_center, rlcd_act_w + 0.60, rlcd_act_h + 0.60, 1.0, 1.60,
                     (rlcd_x0 + 0.15 + rlcd_x1 - 0.15)/2, y_lcd_center,
                     (rlcd_x1 - rlcd_x0) - 0.30, rlcd_tft_h + 0.50, 0.6, rlcd_tft_front_z);
            rbox(rlcd_x0 + 0.15, rlcd_x1 - 0.15,
                 y_lcd_center - (rlcd_tft_h/2 + 0.25), y_lcd_center + (rlcd_tft_h/2 + 0.25),
                 rlcd_tft_front_z - 0.05, z_rlcd_pmma_front + 0.10, 0.6);

            // B. RLCD Main Assembly Pocket
            rbox(rlcd_x0, rlcd_x1, rlcd_y0, rlcd_y1, z_rlcd_pmma_front, sole_depth + 2.0, 0.8);

            // C. Open-Top Flex Cavities for 4 Bed-Rooted RLCD Compliant Retainer Arms
            rbox(-41.5, -20.5, rlcd_y1 - 0.4, 102.00, -eps, sole_depth + 2.0, 0.5);
            rbox( 20.5,  41.5, rlcd_y1 - 0.4, 102.00, -eps, sole_depth + 2.0, 0.5);
            rbox(-48.55, rlcd_x0 + 0.4, 76.00, 97.00, -eps, sole_depth + 2.0, 0.5);
            rbox(rlcd_x1 - 0.4, 51.80,  76.00, 97.00, -eps, sole_depth + 2.0, 0.5);

            // D. Broad Open +X Side Port Aperture (RLCD USB-C H1 + MicroSD SD1)
            rbox(rlcd_x1 - 1.0, bx + 2.0, 32.5, 74.0, 6.90, sole_depth + 2.0, 1.2);

            // E. Matching Open -X Side RF / Antenna & Ventilation Aperture
            rbox(-bx - 2.0, rlcd_x0 + 1.0, 32.5, 74.0, 7.50, sole_depth + 2.0, 1.2);

            // F. Shared Top-Edge RLCD Button Finger Access Opening for KEY4 (SNAP), KEY3 (PWR), KEY1 (BROWSE)
            //    Starts at Z_print = 6.90 (45-deg front bevel at Y=22..24) and opens to +Z across X in [-13.5, +16.5],
            //    giving direct vertical press in Forward (0 deg) and clear rear-top reach behind pod in Selfie (180 deg)!
            yz_prism(-13.5, 16.5, [
                [y_selfie_shelf - 0.5, 6.90],
                [rlcd_y0 + 1.5, 6.90],
                [rlcd_y0 + 1.5, sole_depth + 2.0],
                [y_selfie_shelf - 0.5, 22.50]
            ]);
            // Open pocket directly behind the buttons (Y in [26.8, 33.2], Z >= 6.90) with 45-deg front chamfer
            rbox(-13.5, 16.5, 25.0, rlcd_y0 + 2.0, 6.90, sole_depth + 2.0, 1.2);

            // G. Front-face stencil-bridged SNAP / PWR / BROWSE engravings (Z = 0..0.45)
            rlcd_front_button_engravings();
        }

        // 6. Add the 2 Outboard Vented Female-Cone Hinge Cheeks
        body_outboard_cheek(-1);
        body_outboard_cheek(+1);

        // 7. Two 45-deg self-supporting passive top rear keeper tabs (outside button slot at X = +/-21.5)
        for (tx = [-21.5, 21.5])
            yz_prism(tx - 3.5, tx + 3.5, [
                [rlcd_y0 - 0.5, z_rlcd_v2_back - 0.85],
                [rlcd_y0 + 0.95, z_rlcd_v2_back + 0.60],
                [rlcd_y0 + 0.95, z_rlcd_v2_back + 1.40],
                [rlcd_y0 - 0.5, z_frame]
            ]);

        // 8. Add the 2 Bottom + 2 Side Bed-Rooted 20 x 1.5 mm In-Layer Compliant RLCD Retainer Arms
        rlcd_retainer_arm(-40.5, +1, rlcd_y1 + 0.15, +1);
        rlcd_retainer_arm( 40.5, -1, rlcd_y1 + 0.15, +1);
        for (sx = [-1, 1]) {
            xw = (sx < 0) ? (rlcd_x0 - 0.15) : (rlcd_x1 + 0.15);
            xa_s = min(xw, xw + sx * spring_thick);
            xb_s = max(xw, xw + sx * spring_thick);
            z_arm_top = z_rlcd_v2_back + 3.20;
            z_hook_seat = z_rlcd_v2_back + 0.40;
            reach = 1.05;
            rbox(xa_s, xb_s, 76.5, 99.5, 0, z_arm_top, 0.45);
            rbox(min(xw - sx*0.4, xw + sx*4.2), max(xw - sx*0.4, xw + sx*4.2), 96.5, 100.5, 0, z_arm_top, 0.5);
            xz_prism(76.5, 81.7, [
                [xw + sx*0.4, z_hook_seat - reach - 0.45],
                [xw - sx*reach, z_hook_seat],
                [xw - sx*reach, z_hook_seat + 0.85],
                [xw + sx*0.4, z_arm_top]
            ]);
        }

        // 9. Rear/Top-Crown Integrated Raised Labels (SNAP, PWR, BROWSE at Z_print = 22.45..23.10, X in [-24, +27])
        //    Placed at Y in [22.1, 27.6], Z_print = 22.45 on solid top crown wall outside Selfie landing shelves!
        //    Wait: button cutout opens Y in [22.04, 29.6] at X in [-13.5, +16.5], so put raised labels on the front-lip
        //    top face? No, at Z_print = 6.90 the step face is at Y in [22.04, 25.0] inside X in [-13.5, +16.5]!
        //    EVEN BETTER: Put raised SNAP / PWR / BROWSE on Z_print = 6.90 at Y in [22.2, 25.0] (right in front of the
        //    button slot, visible from rear/top in BOTH Forward and Selfie modes!!)!
        translate([0, 0, 6.90 - 0.05])
            linear_extrude(height = 0.65, convexity = 4) {
                translate([-8.40, 24.4]) square([0.85, 1.4], center = true);
                translate([ 1.60, 24.4]) square([0.85, 1.4], center = true);
                translate([11.60, 24.4]) square([0.85, 1.4], center = true);
                rotate([0, 0, 180]) {
                    translate([ 10.6, -23.1]) text("SNAP",   size = 2.0, font = "Arial:style=Bold", halign = "center", valign = "center");
                    translate([ -1.6, -23.1]) text("PWR",    size = 2.0, font = "Arial:style=Bold", halign = "center", valign = "center");
                    translate([-11.6, -23.1]) text("BROWSE", size = 1.9, font = "Arial:style=Bold", halign = "center", valign = "center");
                }
            }
    }
}

// ============================================================================
// SUBASSEMBLY 2: CAPTIVE ROTATING TOP POD (`pod`) WITH INTEGRAL STEPPED FACEPLATE
// ============================================================================

module pod_outboard_rotor(sx) {
    xa = (sx > 0) ? (pod_half_x - 6.20) : -pod_half_x;
    xb = (sx > 0) ? pod_half_x : (-pod_half_x + 6.20);

    yt = rotor_hub_r / sqrt(2);
    zt = axis_z - yt;
    z_meet = axis_z - rotor_hub_r * sqrt(2) + pod_inner_y; // 6.13 mm at pod_inner_y = -8.80

    union() {
        translate([xa, 0, axis_z])
            rotate([0, 90, 0])
                cylinder(h = xb - xa, r = rotor_hub_r);
        if (sx < 0) {
            yz_prism(xa, xb, [
                [pod_inner_y - 0.15, z_meet - 0.15],
                [yt, zt],
                [0.0, axis_z + rotor_hub_r],
                [pod_inner_y - 0.15, axis_z + rotor_hub_r]
            ]);
        } else {
            // +X rotor: outer 2.20 mm pillar (X in [30.80, 33.00]) connects down to solid corner post at Y=-8.8, Z=6.13;
            // inner 4.00 mm (X in [26.80, 30.80]) rises cleanly at 45 deg from X=30.80 (Z=6.13 -> 10.13) so the
            // 20x1.5 mm battery cassette latch below Z=5.40 stays 100% free and has zero floating islands!
            hull() {
                yz_prism(30.80, xb, [
                    [pod_inner_y - 0.15, z_meet - 0.15],
                    [yt, zt],
                    [0.0, axis_z + rotor_hub_r],
                    [pod_inner_y - 0.15, axis_z + rotor_hub_r]
                ]);
                yz_prism(xa, 30.80, [
                    [pod_inner_y - 0.15, z_meet + 4.00],
                    [yt, zt],
                    [0.0, axis_z + rotor_hub_r],
                    [pod_inner_y - 0.15, axis_z + rotor_hub_r]
                ]);
            }
        }

        if (sx > 0) {
            translate([pod_half_x - eps, 0, axis_z])
                rotate([0, 90, 0])
                    cylinder(h = cone_depth + eps, r1 = cone_radius + eps, r2 = cone_tip_radius);
        } else {
            translate([-pod_half_x + eps, 0, axis_z])
                rotate([0, -90, 0])
                    cylinder(h = cone_depth + eps, r1 = cone_radius + eps, r2 = cone_tip_radius);
        }
    }
}

// Bed-printed 20 x 1.5 mm in-layer PCB retention spring detents engaging BL & BR Ø3.00 mm mounting holes!
module pod_pcb_hole_detents() {
    for (sx = [-1, 1]) {
        scale([sx, 1, 1]) {
            rbox(4.50, 26.80, -21.15, -19.65, 0.0, 7.65, 0.45);
            rbox(3.20, 6.50, -21.15, -17.90, 0.0, 7.65, 0.45);
            // Tapered Ø2.05 mm cylindrical hole-detent pin inside Ø3.00 mm mounting hole (Z_p = 6.35 +/- 1.50 = 4.85..7.85)
            hull() {
                translate([25.01, -21.10, 6.35])
                    rotate([90, 0, 0])
                        cylinder(d = 2.05, h = 2.15);
                translate([25.01, -21.10, 7.42])
                    rotate([90, 0, 0])
                        cylinder(d = 0.70, h = 0.35);
            }
            // Finger-release tab extending from tip out to X_p = +30.8 at Z_p = 0..5.20
            rbox(24.50, 30.80, -21.15, -19.65, 0.0, 5.20, 0.45);
        }
    }
}

module pod_battery_latch() {
    // Kept strictly at Z_p = 0..5.40, X = 8.5..30.0 (free tip clear of 30.8..33.0 corner post & rotor above Z=6.0!)
    translate([0, 0, 0.0])
        linear_extrude(height = 5.40, convexity = 4)
            polygon([
                [8.50, -10.65], [11.50, -10.65], [11.50, -8.80],
                [30.00, -8.80], [30.00, -10.30],
                [29.85, -11.75], [29.25, -11.75], [29.10, -10.30],
                [11.50, -10.30], [8.50, -10.30]
            ]);
}

// Shaped self-supporting 120-deg camera lens aperture + 45-deg peaked gothic arch (zero >45-deg roof overhang!)
module pod_lens_aperture_cut() {
    z_start = camera_lens_min_above_base - 0.60; // 18.30 (clears entire 120-deg cone from 18.90 mm!)
    z_end   = z_e_front_cam + 2.00;              // 27.40
    r0 = 2.40 + lens_window_extra_r;
    r_start = r0 + (z_start - camera_lens_min_above_base) * tan(60);
    r_end   = r0 + (z_end   - camera_lens_min_above_base) * tan(60);

    hull() {
        translate([lens_cx_p, y_p_pcb0 - z_start, lens_cz_p])
            rotate([90, 0, 0])
                cylinder(r1 = r_start, r2 = r_end, h = z_end - z_start);
        translate([lens_cx_p, y_p_pcb0 - z_start, lens_cz_p + r_start * sqrt(2)])
            rotate([90, 0, 0])
                cylinder(r = 0.25, h = 0.05);
        translate([lens_cx_p, y_p_pcb0 - z_end, lens_cz_p + r_end * sqrt(2)])
            rotate([90, 0, 0])
                cylinder(r = 0.25, h = 0.05);
    }
}

// Recessed stepped OLED viewing bezel cut (Y_p = -30.65 out to -36.0, 45-deg flare + 45-deg peaked top!)
module pod_oled_bezel_cut() {
    tw = oled_act_w + 2 * oled_throat_margin_x; // 24.00 mm (X_p in [-6.20, +17.80])
    th = oled_act_h + 2 * oled_throat_margin_y; // 13.40 mm (Z_p in [17.25, 30.65])
    y_in  = y_p_pcb0 - oled_bezel_throat_ze;    // -30.65 (5.00 mm clear of OLED glass at -25.65!)
    y_out = pod_oled_front_y - 0.80;            // -36.20
    ow = tw + 2 * oled_flare_xy;                // 33.80 mm
    oh = th + 2 * oled_flare_xy;                // 23.20 mm

    hull() {
        translate([0, y_in, 0])
            rotate([90, 0, 0])
                linear_extrude(height = 0.06)
                    rrect2d(oled_cx_p, oled_cz_p, tw, th, 1.0);
        translate([0, y_out, 0])
            rotate([90, 0, 0])
                linear_extrude(height = 0.06)
                    rrect2d(oled_cx_p, oled_cz_p, ow, oh, 2.0);
        // Full-width 45-deg upper gable peak so upper window edges slope <= 45 deg cleanly
        translate([0, y_in, 0])
            rotate([90, 0, 0])
                linear_extrude(height = 0.06)
                    polygon([
                        [oled_cx_p - tw/2 + 0.8, oled_cz_p + th/2 - 0.8],
                        [oled_cx_p + tw/2 - 0.8, oled_cz_p + th/2 - 0.8],
                        [oled_cx_p, oled_cz_p + th/2 + 3.8]
                    ]);
        translate([0, y_out, 0])
            rotate([90, 0, 0])
                linear_extrude(height = 0.06)
                    polygon([
                        [oled_cx_p - ow/2 + 1.5, oled_cz_p + oh/2 - 1.5],
                        [oled_cx_p + ow/2 - 1.5, oled_cz_p + oh/2 - 1.5],
                        [oled_cx_p, oled_cz_p + oh/2 + 5.0]
                    ]);
    }
}

module pod_print() {
    union() {
        difference() {
            // 1. Integral Stepped Pod Shell:
            //    - Camera half (X_p in [-33.0, -10.6], chamfering 45-deg back to -6.8 at OLED plane)
            //      clears OLED left_12deg off-axis viewing rays cleanly!
            //    - OLED + J7 half (X_p in [-10.0, +33.0]) steps back to pod_oled_front_y (-35.40, z_e = 13.20)
            union() {
                hull() {
                    rbox(-pod_half_x + 0.6, pod_half_x - 0.6, pod_oled_front_y + 0.6, pod_inner_y, 0, 0.05, 3.0);
                    rbox(-pod_half_x, pod_half_x, pod_oled_front_y, pod_inner_y, 0.6, pod_top_z - 1.2, 3.4);
                    rbox(-pod_half_x + 1.0, pod_half_x - 1.0, pod_oled_front_y + 1.0, pod_inner_y - 0.6, pod_top_z - 0.05, pod_top_z, 2.8);
                }
                // Camera protective front step block (flat right face at X_p = -11.0 for clean -15 deg OLED FOV & 0 edge slivers)
                hull() {
                    rbox(-pod_half_x + 0.6, -11.0, pod_front_y + 0.6, pod_front_y + 2.0, 0, 0.05, 1.6);
                    rbox(-pod_half_x, -11.0, pod_front_y, pod_front_y + 2.0, 0.6, pod_top_z - 1.2, 1.8);
                    rbox(-pod_half_x + 1.0, -11.0, pod_front_y + 1.0, pod_front_y + 2.0, pod_top_z - 0.05, pod_top_z, 1.6);
                    rbox(-pod_half_x + 0.6, -11.0, pod_oled_front_y, pod_oled_front_y + 2.0, 0, 0.05, 1.6);
                    rbox(-pod_half_x, -11.0, pod_oled_front_y, pod_oled_front_y + 2.0, 0.6, pod_top_z - 1.2, 1.8);
                    rbox(-pod_half_x + 1.0, -11.0, pod_oled_front_y, pod_oled_front_y + 2.0, pod_top_z - 0.05, pod_top_z, 1.6);
                }
            }

            // --- SUBTRACTIONS ON POD ---

            // A. Smooth Integral Battery Cassette Bay (+X side slide-in, 45-deg pointed arch roof)
            rbox(-(battery_len/2 + battery_xy_clearance), pod_half_x + 2.0,
                 y_p_pcb0 - (-5.60), y_p_pcb0 - (-11.60),
                 z_p_pcb0 - battery_xy_clearance, z_p_pcb0 + battery_wid + battery_xy_clearance, 0.8);
            yz_prism(-(battery_len/2 + battery_xy_clearance), pod_half_x + 2.0, [
                [y_p_pcb0 - (-5.60),  z_p_pcb0 + battery_wid + battery_xy_clearance - 0.05],
                [y_p_pcb0 - (-11.60), z_p_pcb0 + battery_wid + battery_xy_clearance - 0.05],
                [y_p_pcb0 - (-8.60),  z_p_pcb0 + battery_wid + battery_xy_clearance + 3.05]
            ]);

            // B. Flex relief slot for +X in-layer battery cassette latch (stops at X=30.60 so 30.80..33.00 corner pillar stays solid!)
            xz_prism(-10.80, -8.50, [
                [11.50, -eps], [30.60, -eps],
                [30.60, 5.75], [26.60, 5.75], [11.50, 5.75 + (26.60 - 11.50)]
            ]);

            // C. Rear Battery Wall Diamond Ventilation Windows
            for (vx = [-18.0, -9.0, 0.0, 7.5], vz = [14.5, 25.0, 35.5])
                translate([vx, pod_inner_y - 1.2, vz])
                    rotate([90, 45, 0])
                        cube([3.2, 3.2, 4.0], center = true);

            // D. Underside PCB Component Cavity: z_e in [-4.20, 0.00] (keeps 4 bare-FR4 corner seats & central root!)
            //    Open out top (+Z_p = 50.2) across X_p in [-9.50, +29.60] so BAT1 & J3 MicroSD slide cleanly top-down!
            difference() {
                union() {
                    pod_board_prism(-board_xy_clearance, pcb_len + board_xy_clearance,
                                    -board_xy_clearance, pcb_wid + board_xy_clearance,
                                    -underside_clearance, 0.05, pcb_corner_r + board_xy_clearance);
                    yz_prism(-(pcb_len/2 + board_xy_clearance), -9.20, [
                        [y_p_pcb0 - 0.10, z_p_pcb0 + pcb_wid + board_xy_clearance - 1.2],
                        [y_p_pcb0 + underside_clearance + 0.05, z_p_pcb0 + pcb_wid + board_xy_clearance - 1.2],
                        [y_p_pcb0 + underside_clearance/2, z_p_pcb0 + pcb_wid + board_xy_clearance + 2.4]
                    ]);
                    rbox(-9.50, pcb_len/2 + board_xy_clearance,
                         y_p_pcb0 - 0.10, y_p_pcb0 + underside_clearance + 0.05,
                         z_p_pcb0 + pcb_wid - 1.2, pod_top_z + 2.0, 0.25);
                }
                // Keep BL & BR lower bare-FR4 corner seats (behind spring beam at Y_p >= -19.5)
                pod_board_prism(-1.0, 5.20, -1.0, 5.50, -underside_clearance - 0.2, -2.50, 0.6);
                pod_board_prism(52.80, pcb_len + 1.0, -1.0, 5.10, -underside_clearance - 0.2, -2.50, 0.6);
                // Keep small bare-FR4 rear pad right at BL/BR edges (X_p in [-29.3, -26.5] & [+26.5, +29.3])
                pod_board_prism(-1.0, 2.40, -1.0, 5.50, -underside_clearance - 0.2, 0.0, 0.4);
                pod_board_prism(55.60, pcb_len + 1.0, -1.0, 5.10, -underside_clearance - 0.2, 0.0, 0.4);
                // Keep central spring-root pedestal (X_p in [-6.5, +6.5], Z_p in [0, 7.65])
                rbox(-6.50, 6.50, y_p_pcb0 + 1.05, y_p_pcb0 + underside_clearance + 0.2, -eps, 7.65, 0.4);
                // Keep TL & TR upper bare-FR4 corner seats WITH 45-deg lower corbels
                yz_prism(-pcb_len/2 - 1.0, -pcb_len/2 + 6.20, [
                    [y_p_pcb0 + underside_clearance + 0.2, z_p_pcb0 + 35.60 - underside_clearance - 0.2],
                    [y_p_pcb0, z_p_pcb0 + 35.60],
                    [y_p_pcb0, z_p_pcb0 + pcb_wid + 1.0],
                    [y_p_pcb0 + underside_clearance + 0.2, z_p_pcb0 + pcb_wid + 1.0]
                ]);
                // Keep TR bare-FR4 rear seat at X_p in [6.20, 12.80] (between BAT1 <= +5.65 and J3 >= +13.80) WITH 45-deg lower corbel
                yz_prism(6.20, 12.80, [
                    [y_p_pcb0 + underside_clearance + 0.2, z_p_pcb0 + 37.20 - underside_clearance - 0.2],
                    [y_p_pcb0, z_p_pcb0 + 37.20],
                    [y_p_pcb0, z_p_pcb0 + pcb_wid + 1.0],
                    [y_p_pcb0 + underside_clearance + 0.2, z_p_pcb0 + pcb_wid + 1.0]
                ]);
            }

            // E. Internal Top-Down Insertion Cavity for PCB + XIAO + Sense + Camera + OLED + Grove + J7!
            //    Keep X_p >= -29.35 behind Y_p = -26.5 so TL/BL corner rails stay solidly attached to -X wall,
            //    widen to -29.85 in front (Y_p <= -26.5) to clear the 8.5x8.5 mm camera head at X_p = -29.40,
            //    and join Right/OLED compartment cleanly at X_p = -11.20 (outside OLED bezel flare >= -10.15!).
            rbox(-(pcb_len/2 + board_xy_clearance), -11.20,
                 y_p_pcb0 - 4.30, y_p_pcb0 + 0.85,
                 z_p_pcb0, pod_top_z + 2.0, 0.25);
            rbox(-29.85, -11.20,
                 y_p_pcb0 - face_cam_inner_ze, y_p_pcb0 - 4.00,
                 z_p_pcb0, pod_top_z + 2.0, 0.25);
            //    Right/OLED+Sense+J7 internal compartment (X_p in [-11.50, +29.60], Y_p in [-37.20, -21.35], Z_p >= 2.60 -> open out top!)
            rbox(-11.50, pcb_len/2 + board_xy_clearance,
                 y_p_pcb0 - face_oled_inner_ze, y_p_pcb0 + 0.85,
                 z_p_pcb0, pod_top_z + 2.0, 0.25);

            // F. Integral Stepped Faceplate Windows (Camera Lens 120-deg Aperture + Recessed OLED Bezel!)
            pod_lens_aperture_cut();
            pod_oled_bezel_cut();

            // G. Left (-X_e) Port Portal: XIAO USB-C + Sense MicroSD (z_e in [1.6, 18.6] -> Y_p in [-40.8, -23.8])
            //    Includes vertical USB-C entrance groove (Z_p >= 11.60 out open top) so protruding shield clears top-down insertion!
            yz_prism(-pod_half_x - 2.0, -pcb_len/2 + 1.5, [
                [y_p_pcb0 - 18.60, z_p_pcb0 + 9.00],
                [y_p_pcb0 - 1.60,  z_p_pcb0 + 9.00],
                [y_p_pcb0 - 1.60,  z_p_pcb0 + 33.80],
                [y_p_pcb0 - 18.60, z_p_pcb0 + 33.80 + (18.60 - 1.60)]
            ]);
            rbox(-31.00, -29.00, -36.30, -32.00, z_p_pcb0 + 9.00, pod_top_z + 2.0, 0.25);
            yz_prism(-pod_half_x - 2.0, -pcb_len/2 + 0.5, [
                [-21.80, -eps], [-18.50, -eps], [-18.50, 6.50], [-21.80, 6.50 + (21.80 - 18.50)]
            ]);

            // H. Right (+X_e) Port Portal: single clean polygon covering J2 JST + J3 MicroSD with >=45-deg slope below TR rail!
            yz_prism(pcb_len/2 - 2.5, pod_half_x + 2.0, [
                [y_p_pcb0 - 9.00,    z_p_pcb0 + 6.50],
                [y_p_pcb0 - (-3.80), z_p_pcb0 + 6.50],
                [y_p_pcb0 - (-3.80), z_p_pcb0 + 29.00],
                [y_p_pcb0 - 1.20,    z_p_pcb0 + 34.00],
                [y_p_pcb0 - 9.00,    z_p_pcb0 + 19.00]
            ]);
            yz_prism(pcb_len/2 - 0.5, pod_half_x + 2.0, [
                [-21.80, -eps], [-18.50, -eps], [-18.50, 6.50], [-21.80, 6.50 + (21.80 - 18.50)]
            ]);

            // I. Top (+Y_e) Port Access: SW1 Power Switch + D16 LED + Grove IIC J4/J1 + XIAO Antenna Clearance
            //    Already open out +Z_p across [-29.35, +29.35], Y_p in [-33.0, -18.0] (and [-45.6, -18.0] on left),
            //    plus rear relief over SW1 actuator (y_e = 44.07) & antenna!
            pod_board_prism(7.50, 49.00, pcb_wid - 1.5, pcb_wid + 6.0, -underside_clearance - 0.05, 8.20, 0.8);

            // J. Bottom (-Y_e, Z_p = 0..2.60) Port Openings: K2 Reset + BUZ1 Buzzer + Grove J5/J6 + K1 D1
            //    Kept behind Y_p = -31.0 so the front bottom faceplate skirt stays solid and continuous!
            pod_board_prism(6.00, 12.00, -4.0, 2.20, 0.15, 8.50, 0.6);
            pod_board_prism(14.20, 19.80, -4.0, 2.00, 1.60, 8.50, 0.6);
            pod_board_prism(23.00, 52.00, -4.0, 2.20, 0.15, 8.50, 0.8);
        }

        // 2. Four Bare-FR4 Front Hold-Down Rails (z_e = 1.85..3.40, clearing ALL tall top components on 0..50mm insertion!)
        //    - BL corner: X_p in [-29.35, -24.80], Z_p = 2.60..7.80 (well below XIAO/J10 at Z_p >= 12.13)
        rbox(-pcb_len/2 - 0.35, -pcb_len/2 + 4.20, y_p_pcb0 - 3.60, y_p_pcb0 - 1.85, z_p_pcb0, z_p_pcb0 + 5.20, 0.4);
        //    - BR corner: X_p in [27.95, 29.35], Z_p = 2.60..7.80 (outside J2/J7 edge at X_p <= 27.65)
        rbox(pcb_len/2 - 1.05, pcb_len/2 + 0.35, y_p_pcb0 - 3.60, y_p_pcb0 - 1.85, z_p_pcb0, z_p_pcb0 + 5.20, 0.4);
        //    - TR corner: X_p in [27.95, 29.35], Z_p = 38.20..45.20 (outside J7 edge at X_p <= 27.64, anchored to +X wall)
        yz_prism(pcb_len/2 - 1.05, pcb_len/2 + 0.35, [
            [y_p_pcb0 - 3.60, z_p_pcb0 + 38.20 - 1.80],
            [y_p_pcb0 - 3.60, z_p_pcb0 + 42.60],
            [y_p_pcb0 - 1.85, z_p_pcb0 + 42.60],
            [y_p_pcb0 - 1.85, z_p_pcb0 + 38.20]
        ]);
        //    - Top-Mid-Right bare-FR4 gusset rail: X_p in [18.30, 19.35] (strictly between OLED <= 18.17 & Grove <= 16.60 and J7 >= 19.51!)
        //      Slopes up at 45 deg from the OLED top front wall (Y_p = -38.4, Z_p = 36.6) to lock PCB front at Y_p = -24.05, Z_p = 45.20.
        yz_prism(18.30, 19.35, [
            [-38.40, 36.60],
            [-38.40, pod_top_z - 0.80],
            [y_p_pcb0 - 1.85, pod_top_z - 0.80],
            [y_p_pcb0 - 1.85, z_p_pcb0 + 40.50],
            [y_p_pcb0 - 3.60, z_p_pcb0 + 40.50],
            [y_p_pcb0 - 3.60, 42.80]
        ]);

        // 3. Bed-Rooted 20 x 1.5 mm In-Layer PCB Mounting-Hole Spring Detents (BL & BR bare-FR4 corners)
        pod_pcb_hole_detents();

        // 4. Bed-Rooted 20 x 1.5 mm In-Layer Battery Cassette Latch (+X mouth)
        pod_battery_latch();

        // 5. Add the 2 Outboard Male-Cone Rotor Stubs
        pod_outboard_rotor(-1);
        pod_outboard_rotor(+1);
    }
}

// ============================================================================
// APPROXIMATE HARDWARE REFERENCE ENVELOPES (for Collision Check & Dashboard)
// ============================================================================

module hw_rlcd_print() {
    z_off = 0.05;
    union() {
        rbox(-43.90, 47.93, y_lcd_center - 33.80, y_lcd_center + 33.80,
             rlcd_tft_front_z + z_off, rlcd_tft_front_z + 1.80 + z_off, 0.4);
        rbox(-44.60, 47.70, y_lcd_center - 34.50, y_lcd_center + 34.50,
             z_rlcd_pmma_front + z_off, z_rlcd_pmma_front + 2.00 + z_off, 0.8);
        rbox(-44.66, 47.86, y_lcd_center - 34.56, y_lcd_center + 34.56,
             z_rlcd_pcb_front + z_off, z_rlcd_pcb_front + 1.60 + z_off, 0.8);
        for (sx = [-41.15, 44.35], sy = [-31.05, 31.05])
            translate([sx, y_lcd_center - sy, z_rlcd_pcb_front + 1.60 + z_off])
                cylinder(d = 5.4, h = 7.00);
        rbox(-44.65, 47.85, y_lcd_center - 34.55, y_lcd_center + 35.55,
             z_rlcd_v2_back - 2.75 + z_off, z_rlcd_v2_back + z_off, 1.2);
        rbox(40.16, 47.87, y_lcd_center - 4.78, y_lcd_center + 4.80, z_rlcd_pcb_front + 1.5 + z_off, z_rlcd_pcb_front + 4.85 + z_off, 0.5);
        rbox(31.81, 46.82, y_lcd_center - 27.20, y_lcd_center - 11.10, z_rlcd_pcb_front + 1.5 + z_off, z_rlcd_pcb_front + 3.45 + z_off, 0.4);
        for (kz = [-10.68, -0.68, 9.32])
            rbox(kz, kz + 4.55, y_lcd_center - 34.36, y_lcd_center - 30.71, z_rlcd_pcb_front + 1.4 + z_off, z_rlcd_pcb_front + 3.4 + z_off, 0.3);
        rbox(-41.90, 45.10, y_lcd_center + 8.85, y_lcd_center + 29.95,
             z_rlcd_pcb_front + 1.60 + z_off, rlcd_tft_front_z + 19.55 + z_off, 0.8);
        translate([-33.0, y_lcd_center + 19.40, z_rlcd_pmma_front + 14.10 + z_off])
            rotate([0, 90, 0])
                cylinder(d = 18.60, h = 66.00);
    }
}

// Expansion Base + XIAO Sense stack WITHOUT LiPo (for exact top-down PCB insertion sweep!)
module hw_xiao_stack_print() {
    z_xiao_bot = pcb_thick + header_stack_h;
    z_xiao_top = z_xiao_bot + 1.25;
    z_sense_bot = z_xiao_top + 2.93;
    z_sense_top = z_sense_bot + 1.25;
    z_cam_top = z_xiao_bot + 13.96;
    z_off = 0.05;

    mh_coords = [[3.990, 3.749], [3.990, 38.749], [53.990, 3.749], [53.990, 38.749]];

    union() {
        // Expansion Base PCB with actual 4x Ø3.00 mm unplated mounting holes
        difference() {
            pod_board_prism(0, pcb_len, 0, pcb_wid, z_off, pcb_thick + z_off, pcb_corner_r);
            for (p = mh_coords)
                translate([p[0] - pcb_len/2, y_p_pcb0 - z_off + 0.5, z_p_pcb0 + p[1]])
                    rotate([90, 0, 0])
                        cylinder(d = 3.00, h = pcb_thick + 1.0);
        }
        // Underside CR1220 BAT1 + J3 SD (note: MicroSD card removed during initial slide-in, inserted via +X slot)
        pod_board_prism(22.55, 34.65, 14.41, 27.99, -3.20 + z_off, z_off + 0.02, 0.8);
        pod_board_prism(42.80, 57.80, 13.77, 28.76, -1.85 + z_off, z_off + 0.02, 0.3);
        // Top Base Components (OLED U6, 4x Grove, SW1, J2 JST, J7 Header, Socket Headers)
        pod_board_prism(22.43, 47.17, 12.80, 29.70, pcb_thick, pcb_thick + 1.85 + z_off, 0.3);
        pod_board_prism(24.00, 45.60, 32.30, 42.30, pcb_thick, pcb_thick + 6.20 + z_off, 0.3);
        pod_board_prism(24.00, 45.60, 0.30, 10.30, pcb_thick, pcb_thick + 6.20 + z_off, 0.3);
        pod_board_prism(10.44, 19.54, 37.37, 44.07, pcb_thick, pcb_thick + 3.20 + z_off, 0.2);
        pod_board_prism(48.65, 56.65, 6.87, 15.00, pcb_thick, pcb_thick + 6.10 + z_off, 0.3);
        pod_board_prism(48.51, 56.64, 16.89, 27.05, pcb_thick, pcb_thick + 8.50 + z_off, 0.2);
        pod_board_prism(1.65, 19.43, 9.53, 14.90, pcb_thick, z_xiao_bot + eps, 0.2);
        pod_board_prism(1.65, 19.43, 27.60, 33.07, pcb_thick, z_xiao_bot + eps, 0.2);
        // XIAO + Sense + Camera stack
        pod_board_prism(0.06, 21.02, 12.36, 30.14, z_xiao_bot, z_xiao_top, 1.5);
        pod_board_prism(-1.46, 5.84, 16.78, 25.72, z_xiao_top - 0.2, z_xiao_top + 3.21, 0.6);
        pod_board_prism(17.60, 20.54, 19.10, 27.70, z_xiao_top - eps, z_sense_bot + eps, 0.2);
        pod_board_prism(6.65, 21.32, 12.86, 29.64, z_sense_bot, z_sense_top, 1.2);
        pod_board_prism(-3.04, 12.02, 14.39, 26.48, z_sense_top - eps, z_xiao_bot + 8.50, 0.3);
        pod_board_prism(-0.40, 8.10, 17.00, 25.50, z_xiao_bot + 8.40, z_cam_top - 1.60, 0.4);
        translate([lens_cx_p, y_p_pcb0 - (z_cam_top - 1.75), lens_cz_p])
            rotate([90, 0, 0])
                cylinder(d = 7.10, h = 1.75);
    }
}

module hw_xiao_pod_print() {
    union() {
        pod_board_prism(0, battery_len, 0, battery_wid, -11.40, -11.40 + battery_max_thick, 0.8);
        hw_xiao_stack_print();
        // Also include protruding inserted SD cards when seated
        pod_board_prism(57.70, 60.20, 16.00, 27.60, -1.80, -0.15, 0.2);
    }
}

// Separate thin reference surface at the ACTUAL OLED active-screen plane (21.74 x 10.86 mm at z_e = 3.45 mm)
module hw_oled_active_print() {
    z_oled_glass_top = pcb_thick + 1.85; // 3.45 mm above base PCB underside
    pod_board_prism(34.80 - oled_act_w/2, 34.80 + oled_act_w/2,
                    21.35 - oled_act_h/2, 21.35 + oled_act_h/2,
                    z_oled_glass_top - 0.05, z_oled_glass_top + 0.30, 0.15);
}

module hw_lens_cone_print() {
    translate([lens_cx_p, y_p_pcb0 - camera_lens_min_above_base, lens_cz_p])
        rotate([90, 0, 0])
            cylinder(d1 = 4.80, d2 = 4.80 + 2 * 12.0 * tan(60), h = 12.0);
}

// ============================================================================
// KINEMATIC TRANSFORMS
// ============================================================================
module pod_at_fold_in_print_space(angle_deg) {
    translate([axial_test_shift, 0, axis_z])
        rotate([angle_deg - 90, 0, 0])
            translate([0, 0, -axis_z])
                children();
}

module print_to_use_space() {
    translate([0, 0, y_sole])
        rotate([-90, 0, 0])
            children();
}

module maybe_use_pose() {
    if (pose == "use") print_to_use_space() children();
    else children();
}

// ============================================================================
// TOP-LEVEL DISPATCHER
// ============================================================================
if (part == "print_assembly") {
    color("#3b4450") main_body_print();
    color("#526070") pod_print();
} else if (part == "body") {
    maybe_use_pose() main_body_print();
} else if (part == "pod") {
    if (pose == "use") {
        print_to_use_space() pod_at_fold_in_print_space(fold) pod_print();
    } else {
        pod_at_fold_in_print_space(fold) pod_print();
    }
} else if (part == "use_assembly") {
    print_to_use_space() {
        color("#3b4450") main_body_print();
        color("#526070") pod_at_fold_in_print_space(fold) pod_print();
        if (show_hw) {
            color("#1f6b45", 0.85) hw_rlcd_print();
            color("#c88428", 0.85) pod_at_fold_in_print_space(fold) hw_xiao_pod_print();
            color("#38e5c8") pod_at_fold_in_print_space(fold) hw_oled_active_print();
            color("#33aaff", 0.30) pod_at_fold_in_print_space(fold) hw_lens_cone_print();
        }
    }
} else if (part == "hw_rlcd") {
    maybe_use_pose() hw_rlcd_print();
} else if (part == "hw_xiao_pod") {
    if (pose == "use") {
        print_to_use_space() pod_at_fold_in_print_space(fold) hw_xiao_pod_print();
    } else {
        pod_at_fold_in_print_space(fold) hw_xiao_pod_print();
    }
} else if (part == "hw_xiao_stack") {
    if (pose == "use") {
        print_to_use_space() pod_at_fold_in_print_space(fold) hw_xiao_stack_print();
    } else {
        pod_at_fold_in_print_space(fold) hw_xiao_stack_print();
    }
} else if (part == "hw_oled_active") {
    if (pose == "use") {
        print_to_use_space() pod_at_fold_in_print_space(fold) hw_oled_active_print();
    } else {
        pod_at_fold_in_print_space(fold) hw_oled_active_print();
    }
} else if (part == "hw_lens_cone") {
    if (pose == "use") {
        print_to_use_space() pod_at_fold_in_print_space(fold) hw_lens_cone_print();
    } else {
        pod_at_fold_in_print_space(fold) hw_lens_cone_print();
    }
}
