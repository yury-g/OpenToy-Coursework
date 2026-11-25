DESK TOY NECK v0.1 — PRINT-IN-PLACE CAD PROTOTYPE
Prepared October 1, 2026

This is an original, editable first mechanical design, not a physically tested classroom print. It targets ordinary hobby FDM printers with a 0.4 mm nozzle, 0.20 mm layers, and PLA. The face is 70 x 46 mm. The print envelope is about 126 x 104 x 114 mm. After installing the side motor, overall width is about 136 mm. Render colors distinguish the members; the model uses a single filament. The displayed face graphics are illustrative; no display module is included or mechanically verified.

WHAT MOVES
Turn (yaw): the whole upper frame turns around the vertical base axis.
Nod (pitch): the carriage, rear roll motor and face rotate between two captive side pivots.
Tilt (roll): the face turns around the forward-pointing shaft inside the carriage.
Initial software limits: turn +/-45 degrees, nod +/-25, tilt +/-25. There are no physical travel stops. The animation shows each axis separately and is a kinematic illustration, not a dynamic simulation.

PRINT-IN-PLACE MEANS
The base, turn frame, nod carriage and face are printed together with their shafts already captive. No axle or rolling bearing needs to be inserted. The motors and three matching stock servo horns go in afterward.

This is NOT a support-free print. The CAD includes red sacrificial walls/posts, small bearing ties, and a segmented starter ring. Remove those after printing, then free every joint by hand. The walls/post bodies need removal, not merely a broken top connection, because they would collide with later motion. The segmented starter ring is accessed through the four openings around the base. Its removal and the horizontal bearing release still need a real print trial.

Do not switch on automatic supports inside the captive bearings: that can fill the intended clearance. Start with the included samples. The full model is explicitly named DRAFT because slicer cantilever warnings remain.

FIRST PRINTS
1. Print clearance-samples-030-040-045-050.stl, flat in its supplied orientation. The four coupons have printed gap labels and are independent. Their original order is: increasing original Y is 0.30, 0.40, 0.45, 0.50 mm nominal radial clearance. The outer housing is fixed and the arm turns with the inner axle. Remove the arm's support post and free the tiny bearing tie. Each gap is PER SIDE: a 0.45 mm radial gap means 0.90 mm diameter difference. Pick the tightest coupon that moves freely without scraping or force. These reproduce the shoulder/journal idea, not the full loaded base thrust bearing.
2. Print servo-and-horn-fit-gauge.stl. It contains a case opening, mounting cradle and slotted horn plate. Try the actual motor, its flange, stock horn, screw heads and screwdriver access. Recheck fit after choosing filament/profile.
3. Set gap in desk-toy-neck.scad to the chosen value, set part to mechanism, and export a new STL. Print the full mechanism flat, base down. Leave it as one assembled object; do not auto-orient or separate its connected volumes. The default gap is 0.45 mm, and the nod joint has an additional approximately 0.5 mm axial space between moving end faces and fixed sockets.

STARTING SLICER SETTINGS
PLA; your filament manufacturer's normal temperature settings; 0.4 mm nozzle; 0.20 mm layers; 3 walls; 15% infill; 4 or more top/bottom layers; normal PLA cooling with slower bridges. Use your printer's normal calibrated flow and first-layer settings. Elephant-foot compensation is especially relevant to the samples. External brim can help tall, thin sacrificial walls; inspect its reach so it does not connect moving sample parts. Avoid raft and support interfaces in the moving clearances.

Clearance is not a coefficient of friction. Printer flow, cooling, first layer squish, seams, filament variation, and the circular underside of a horizontal shaft all affect release. A loose coupon can have play; a tight coupon can fuse. This design trades precision for a forgiving first trial on ordinary student printers.

MOTOR CANDIDATE AND FIT
The mount is based on DFRobot SER0039 / DMS-MG90, a metal gear micro servo. Published product data: 12 g, 1.8 kg·cm STALL torque at 4.8 V, 4.8–6 V operation, and 0.12 s per 60 degrees no-load speed at 4.8 V.
https://www.dfrobot.com/product-1338.html

The supplier listing gives 22.3 x 11.8 x 26.3 mm, but its linked drawing gives a 23 x 12 mm body, 33 mm overall flange, 29 mm ear pitch, 6 mm shaft offset, and 30 mm height including output shaft. CAD uses the larger drawing envelope, not an assumed exact fit. The representative motor meshes simplify the upper housing and are not manufacturer CAD.
https://raw.githubusercontent.com/Arduinolibrary/DFRobot_DMS_MG90/master/DMS-MG90%20Servo%20Dimension.pdf

Stock horn dimensions and supplied accessory contents remain unverified. The plate has slots at radii 8–13.5 mm for attachment to a matching stock horn. Do not print a replacement spline. The fit gauge must pass before buying a class set. Other MG90S-branded servos are not guaranteed interchangeable. A Hitec HS-65MG is a more documented alternative, but needs a separately adjusted holder and its own horn pattern; it is not a drop-in substitute for this CAD.

Assembly is three stock horns plus ordinary screws, with no gear train, custom shaft or press fit. Suggested fasteners, to confirm on the gauge: four M1.6 x 10 mm through bolts with nuts and washers for the pitch/roll motor ears; two approximately 2 mm servo self-tapping screws into the base pedestals' 1.8 mm pilot holes; six M1.6 x 8 mm bolts/nuts for the stock horns to their plates; and each motor's matching center horn screw. Do not assume one generic center screw fits every servo spline. The horn attachment slots are nominal 2 mm wide.

ALIGNMENT AND LOAD PATH
Free the print before mounting motors. The turn sleeve seats downward about the nominal gap onto its printed thrust surface; yaw_servo_drop accounts for this in the fixed motor holder. The motor case opening has 0.55 mm per-side allowance, and 2.8 mm ear holes permit small alignment changes for the nod/tilt motors. Center the servos, let the joints settle, seat the horns, and tighten the motor screws without pulling a free joint sideways. The bearing/horn load sharing depends on this alignment. A captive bearing with clearance does not automatically remove all load from a servo shaft.

The paired nod pivots reduce cantilever loading. The face stays near its axis, while the rear roll motor partly balances its forward weight. The roll bearing remains a short plain bearing carrying a cantilevered face; the friction estimate includes an allowance for that contact amplification. No servo shaft radial or axial load rating has been verified.

LOAD MODEL — CONSERVATIVE SOLID PLASTIC
PLA density is assumed 1.24 g/cm3. Each mesh's volume, center of mass and inertia were calculated. Every permanent printed member is treated as solid, so no optimistic infill mass reduction is needed. Sacrificial supports are excluded from operating mass because they must be removed.
base: 64.9 g solid PLA; center 0.01, -0.04, 5.74 mm.
yaw: 68.6 g solid PLA; center -2.42, -0.03, 46.90 mm.
pitch: 27.3 g solid PLA; center -3.64, -18.84, 89.98 mm.
face: 25.5 g solid PLA; center -0.00, 12.18, 91.00 mm.

The load model adds 12 g per downstream servo, a 10 g face-electronics allowance located 5 mm above the center, 3 g wiring per moving stage, and 1 g per moving stock horn. The fixed yaw motor is carried by the base, so its 12 g does not contribute to upper-axis inertia.

Turn: moving mass 167.5 g; peak gravity 0.000 kg·cm; inertia at 2 rad/s² 0.0053 kg·cm; friction sensitivity 0.090–0.226 kg·cm; total planning budget 0.331 kg·cm.
Nod: moving mass 82.8 g; peak gravity 0.048 kg·cm; inertia at 2 rad/s² 0.0013 kg·cm; friction sensitivity 0.012–0.029 kg·cm; total planning budget 0.179 kg·cm.
Tilt: moving mass 39.5 g; peak gravity 0.002 kg·cm; inertia at 2 rad/s² 0.0002 kg·cm; friction sensitivity 0.018–0.044 kg·cm; total planning budget 0.146 kg·cm.

Gravity torque uses sum of mass x gravity x perpendicular lever arm, with moments signed so real counterbalancing is included. Vertical yaw has no ideal gravity torque, but its loaded thrust face still has friction. Moving the electronics, choosing heavier motors, or changing infill asymmetrically can change the balance.

Friction is a sensitivity calculation, NOT measured PLA-on-PLA data: assumed mu = 0.20, 0.35, 0.50. Plain-bearing estimate T = mu x normal force x effective contact radius. Conservative effective radii: turn 27 mm, nod 7 mm, tilt 8 mm. For tilt, opposing contact reactions from the cantilever increase the modeled normal load by about 2.79 times its carried weight, using an 18 mm bearing length. These estimates omit print interference, shaft ovality, preload, cable drag and temperature effects. Tight or partially fused prints can exceed them by a large amount.

The planning budget adds maximum gravity, inertia at an assumed gentle 2 rad/s² acceleration, mu=0.50 friction, and a separate arbitrary 0.10 kg·cm startup reserve per axis. The reserve is an engineering allowance, not a measured stiction value. If any assumption is exceeded, recalculate.

All three budgets fall below a deliberately chosen 0.45 kg·cm target, which is 25% of the candidate's 1.8 kg·cm stall rating. That fraction is a conservative design choice, NOT a manufacturer continuous-duty torque specification. Stall torque is not a sustainable operating point. A stronger motor is not a fix for a fused printed joint.

MEASURE BEFORE POWERING
Measure the bare, freed joint's breakaway torque with the expected load added. For a spring scale pulling tangentially:
    torque in kg·cm = force in grams x lever length in cm / 1000.
Example: a 20 g pull at a 5 cm radius is 0.10 kg·cm. Keep the lever in the plane perpendicular to the axis. The coupon arm offers roughly a 2.5 cm lever; measure your actual pull point. Measure both directions and several angles, and distinguish startup peak from running force. For the full turn bearing, add enough weighed mass to represent the upper assembly; the solid-model upper moving mass is about 167 g. The coupon is primarily a clearance test and cannot certify the loaded whole mechanism.

If a journal sticks or catches, remove remaining starter material and inspect the surfaces/clearance. Make another coupon with a different gap rather than powering through it. Record the loaded breakaway torque and compare it with the motor's practical operating allowance; update the reserved startup term using the measurement.

ELECTRICAL STARTING POINT
A XIAO ESP32-S3 can command ordinary position servos, but use a separate regulated 5 V servo branch with a common ground. Do not power three servos through the XIAO's 3.3 V regulator. A 5 V / 3 A supply is a reasonable provisional bench starting point for three micro servos; measure current transients on the exact motors. The candidate's 3.3 V input threshold is not published. Verify it, or use a driver that supplies suitable PWM levels. Motion should ramp gently; begin at +/-10 degrees before the intended full envelope. Firmware and hardware wiring have not been changed in this task.

WHAT WAS CHECKED
The four permanent printed members are closed, positive-volume meshes with one connected solid each. CAD Boolean collision checks covered 75 combinations of yaw (-45,0,+45), pitch and roll (-25,-12.5,0,+12.5,+25), across 15 relevant part pairs: 1,125 sampled pair checks with no positive overlap beyond 0.01 mm3. This is a sampled check, not a proof of continuous swept clearance. It covers representative motor envelopes and horn shapes, not actual wires, fastener heads or an exact display PCB. Alignment under real bearing clearance remains untested.

A separate layer-continuity check at 0.2 mm spacing found no new disconnected polygon islands using 0.2 mm proximity. It does not prove adequate bridging, adhesion or surface quality.

Bambu Studio 2.00.03.54 completed an offline slice using a generic Ender-3 V2 / 0.4 mm / PLA profile, 0.2 mm layers, 3 walls and 15% infill. The slicer still emitted floating-cantilever warnings and internal clipping diagnostics. Therefore this is NOT qualified for unattended student printing. Review actual toolpaths, print the coupons, then trial one mechanism before distributing it. No generated printer-specific G-code is included.

PROJECT RESEARCH — DOCUMENTED EXAMPLES, NOT AN EXHAUSTIVE LIST
LeLamp / Human Computer Lab: five FEETECH STS3215 TTL serial bus servos; open robot-lamp build and CAD. Useful example of supported joints and bus wiring. Larger than this toy; do not substitute its servo voltage without checking the chosen variant.
https://github.com/humancomputerlab/LeLamp
https://github.com/humancomputerlab/LeLamp/blob/master/docs/2.%20Servos%20Setup.md

Dheera's jumping Luxo: one Savox SH-0262MG for the head and four Savox SC-1251MG for the other joints. Open printed parts, Raspberry Pi/PCA9685, rolling bearings. Jumping and long lamp arms justify much larger actuators.
https://github.com/dheera/robot-luxo
https://dheera.net/projects/luxo/

Cornell Pixar Lamp (2019): six LewanSoul LX-16A serial servos, mostly 3D printed, Raspberry Pi face tracking. Good classroom precedent for a full lamp arm; not a print-in-place micro-servo neck.
https://courses.ece.cornell.edu/ece5990/ECE5725_Fall2019_Projects/Dec_04_Demo/Pixar_Lamp/Website/index.html

Luci / Jochen Alt: five mixed Hitec servos. The author's article lists 77B x2, 7954SH, 7775MG, and 5065MG in the lampshade. Model spellings are retained as published. Its larger moving arm needed springs; the author discusses backlash, limits on acceleration and a separate motor supply.
https://github.com/jochenalt/Luci
https://github.com/jochenalt/Luci/blob/master/documentation/ODROID%20Magazine%20-%20Article%20Luci.docx

Apple ELEGNT (2025): a repurposed six-degree-of-freedom robot arm with a printed lamp head, light and projector. The inspected paper does not publish exact motor models. Especially useful for expressive motion design, rather than choosing hobby servos.
https://arxiv.org/html/2501.12493v1

Pinokio / Adam Ben-Dror, Shanshan Zhou and Joss Doggett: an early expressive lamp robot. Exact motor models could not be verified from the creator page; do not treat photos as motor specifications.
https://ben-dror.com/index.php/projects/pinokio/

Watti / Nikolay Tyulkin: five motorized joints, Raspberry Pi and RGBD camera; a prototype-preview repository, with complete code not yet public. The creator's robotics post names 24 V RobStride BLDC CAN servos; exact actuator models are unspecified.
https://github.com/Nikolay-Tyulkin/Watti
https://www.reddit.com/r/robotics/comments/1v6gd10/i_moved_my_3dprinted_robot_lamp_to_a_raspberry_pi/

An adjacent neck example, 3dofneck.v02, lists two MG90S for pitch/roll and one MG995 for yaw. It demonstrates the motion arrangement but uses craft-stick construction, not the proposed captive bearings.
https://github.com/v1n47/3dofneckv.02

The CAD in this package was created for this request. Those projects are references; their parts were not copied into this model.

FILES
stl/clearance-samples-030-040-045-050.stl — first print: four clearance samples.
stl/servo-and-horn-fit-gauge.stl — first motor/horn fit check.
stl/neck-one-print-DRAFT.stl — full captive mechanism with sacrificial material.
desk-toy-neck.scad — editable, parametric CAD; dimensions are in mm.
desk-toy-neck.blend — editable mesh assembly and 36-frame motion animation; dimensions in meters internally.
assembled.png / rear-view.png — actual CAD renders with illustrative face electronics.
print-in-place.png — actual CAD render of printed parts with removable material in red.
motion.gif — the three axes shown separately; no physical motion test.
engineering-results.json — the geometry-derived mass/torque assumptions and check results.
README.txt — this guide.
