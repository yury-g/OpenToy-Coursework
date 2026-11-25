FOUR PRINT-IN-PLACE SERVO TOYS - EXPLORATORY CAD v0.1
2026-10-01 | dimensions mm | ordinary hobby FDM / PLA

Included: giraffe with mouth; taller plant with face; elbow/wrist/finger arm; swan with gripping beak.
These are concrete CAD prototypes, not physically printed or classroom-qualified products. The Ender-3 V2 / 0.4 mm / 0.20 mm slicer completed all drafts but flagged cantilevers and emitted clipping diagnostics. Do not treat the STL or illustrations as a proven successful print. Trial-print one before asking a class to print it.

FILES AND WHAT THEY MEAN
Open each .scad in OpenSCAD; keep servo-toy-library.scad next to it. .blend files provide editable, animated 3D previews. The STL marked print-in-place-DRAFT contains the plastic mechanism plus intentionally removable starters, with all servos and purchased horns excluded. The renders show the actual CAD mesh; illustrative eyes/spots/display pixels and colors are decorative, not additional printable components. The red print-layout structures are sacrificial supports. The motion GIFs are illustrations of sampled CAD motion, not footage of a working robot.

PRINTER STARTING POINT
220 x 220 x 250 mm build space (Ender-3 class); 0.4 mm nozzle; 0.20 mm layers; PLA; 3 walls; 15% infill; elephant-foot compensation 0.15 mm. The torque model uses SOLID plastic volumes instead of assuming the infill saves every gram. Print in the supplied upright orientation. No specialty material, dual nozzle, bought bearings or threaded joint pins are required for the captive joint geometry.
Default joint gap is 0.45 mm PER radial side: a 7 mm-radius pin runs in a 7.45 mm-radius bore. The captive transverse shaft is 10.2 mm long through a 9 mm socket; the neighboring shoulder/root arrangement leaves about 0.5 mm end clearance. The yaw sleeve settles down 0.45 mm onto its thrust surface after release; the fixed motor mount accounts for that drop.
Print clearance-joint-coupon.stl first. It tests 0.30, 0.40, 0.45 and 0.50 mm radial gaps on the same captive shaft profile. Cut the tiny tie, work the joint by hand and choose the smallest gap that releases freely. Change gap in each .scad and re-export if needed. This check does not qualify the tall overhangs, stiffness or durability of the full models.
Do not fill captive joint gaps with automatically generated support. External red supports are deliberately accessible cutting/removal targets. Inspect the full slice, including long bridges, upright thin walls and the giraffe roof. A brim around tall exterior supports may help. Slicer warnings remain unresolved until a physical trial establishes which bridges need revised manual scaffolding.

MOTORS AND SIMPLE COUPLING
Each joint takes one purchased positional hobby servo, its original horn, the supplied center screw, and two small screws through the printed driven-plate slots into the horn. The printed shafts retain the mechanism; the horn transmits torque. No printed spline is used. Ear fasteners/appropriate washers or nuts still have to secure each motor to its mount; the servo kit may not include every screw needed.
Print the motor-specific fit gauge before buying a classroom batch or printing a full mechanism. Check the actual case, mounting ears, shaft/flange distance, horn height and holes against it. Horn meshes are representative two-arm shapes; they are not manufacturer-certified dimensions. The plate has radial slots spanning 8 to 13.5 mm, diameter 2 mm, plus a 4 mm center access hole. Use a stock horn whose inner holes line up with those slots. Verify that supplied horn screws engage the horn and do not bottom into the motor.
SER0039 / DMSMG90: DFRobot drawing envelope 23 x 12 x 30 mm including output; 12 g, rated stall torque 1.8 kg.cm at 4.8 V. The listing gives a smaller body envelope, so this CAD uses the drawing dimensions. Mount shaft offset 6 mm, ear pitch 29 mm and flange span 33 mm come from its dimensional drawing.
MG92B: manufacturer envelope 22.8 x 12 x 31 mm; 13.8 g; rated stall torque 3.1 kg.cm at 5 V. Its flange span 35 mm, ear pitch 31 mm and shaft offset 6 mm are PROVISIONAL CAD fit assumptions; measure the gauge against the exact servo. The mount spans 38 mm and has case clearance 0.55 mm per side.
MG996R: manufacturer envelope 40.7 x 19.7 x 42.9 mm; 55 g; rated stall torque 9.4 kg.cm at 4.8 V. CAD shaft offset 10.5 mm, flange span 54 mm, 47.6 mm nominal ear spacing and 5 mm lateral hole offsets are provisional. Its mount uses elongated 3.6 mm holes. The mounting-face drop is 16.3 mm below the shaft tip, inferred from 42.9 mm overall height minus the drawing's 26.6 mm bottom-to-flange dimension. Drawing/listing revisions differ slightly; confirm with your motor.

VARIANTS AND MOTION

GIRAFFE
Servo order from base toward tip: MG92B, MG92B, SER0039.
Print envelope: 107.5 x 108.0 x 197.0 mm.
Moving solid-PLA masses by member: carrier 52.8 g, neck 86.0 g, jaw 18.3 g.
Three joints: turn +/-45 degrees, neck +/-20 degrees, mouth 0 to -18 degrees. Head is 62 mm across, on a hollow 85 mm neck. Motor and bearing brackets remain exposed for easy insertion. It expresses movement; the mouth is not designed as a loaded gripper. Its neck torque budget is close to the conservative MG92B planning target; test real friction before extending its bend angle.
Planning torque by joint (kg.cm): 0.527, 0.733, 0.127.
25% of rated stall, chosen as a conservative comparison target (kg.cm): 0.775, 0.775, 0.450. This target is a design choice, not a manufacturer continuous-duty rating.

PLANT
Servo order from base toward tip: SER0039, SER0039, SER0039.
Print envelope: 125.6 x 108.0 x 215.0 mm.
Moving solid-PLA masses by member: carrier 66.5 g, nod 27.3 g, face 45.1 g.
Three joints: turn +/-64 degrees (128 total), nod +/-15 degrees, face tilt +/-20 degrees. The face is 70 x 46 mm, with six petals extending the flower to about 106 mm. The front pot opening permits motor service after printing. Keep an external wire loop clear of the rotating stem. Electronics mass allowance is 10 g; the illustrative screen is not an exact fitted PCB.
Precise full 360-degree looking needs position feedback. An FS90R continuous servo sets speed/direction, not heading. A 360-degree encoder servo such as STS3215 needs a different mount; unlimited repeated rotations also require a slip ring or all electronics above the rotating joint. Neither 360-degree mount nor slip ring is included here.
Planning torque by joint (kg.cm): 0.351, 0.164, 0.185.
25% of rated stall, chosen as a conservative comparison target (kg.cm): 0.450, 0.450, 0.450. This target is a design choice, not a manufacturer continuous-duty rating.

ARM
Servo order from base toward tip: MG996R, MG92B, SER0039.
Print envelope: 145.0 x 100.0 x 159.0 mm.
Moving solid-PLA masses by member: forearm 36.1 g, hand 29.5 g, finger 13.8 g.
Three joints: elbow 0 to -90 degrees; wrist -15 to +30; finger 0 to -75. Two link lengths are 55 and 35 mm, plus the 27 mm rounded finger reach. It moves in a vertical plane: turn/reposition the base to target different lateral keyboard spots. The swan has a separate pan joint.
Anchor/clamp the base before applying force. A sample press pose is elbow -90, wrist -15, finger -75: calculated fingertip contact is about 6 mm above the table and 89 mm forward of the elbow axis. Calibrate against the actual key height by hand. No desk clamp is included.
Planning torque by joint (kg.cm): 0.779, 0.264, 0.112.
25% of rated stall, chosen as a conservative comparison target (kg.cm): 2.350, 0.775, 0.450. This target is a design choice, not a manufacturer continuous-duty rating.
Additional torque for an assumed 0.3 N vertical tip load, by joint (kg.cm): 0.358, 0.190, 0.083. Adding it conservatively to the no-contact budgets gives 1.137, 0.454, 0.195. Actual keyboard and object forces must be measured; 0.3 N is not a verified keyboard specification.

SWAN
Servo order from base toward tip: MG92B, MG996R, MG92B, MG92B.
Print envelope: 119.0 x 93.0 x 169.0 mm.
Moving solid-PLA masses by member: carrier 66.5 g, lower 36.5 g, upper 39.5 g, beak 9.4 g.
Four joints: pan +/-45 degrees; lower neck 0 to -105; upper neck +/-20; beak -15 to +8. Neck spans are 48 and 35 mm. Its 80 mm-diameter printed turn bearing supports the bent neck; the front service window clears the folding mechanism above the bearing contact region. The upper head and tapered lower beak form a small gripper. Add 1 mm stick-on rubber to each contact face; no TPU printer is needed. The nominal beak gap is 6 mm at rest; at +8 degrees its end gap is about 2.4 mm before the pads.
The load model includes a 25 g sock and 2 g pads. This is an assumed test payload, not a demonstrated lifting rating. Weigh your sock; fabric slip, pad compression and pulling force from the couch can dominate its weight. First test a free, loose sock. No claim is made for dragging a trapped sock or operating across a couch from one fixed position.
The sample low reach pose is pan 0, lower neck -105, upper neck -20, beak 0. It puts the lower-beak tip roughly 9 mm above the table, about 58 mm forward of the neck hinge. These are ideal CAD locations; the two faces are tilted in that pose. Place the anchored base close to the object. The extra reach still and animated scene show the actual mechanism.
Planning torque by joint (kg.cm): 0.669, 1.030, 0.419, 0.186.
25% of rated stall, chosen as a conservative comparison target (kg.cm): 0.775, 2.350, 0.775, 0.775. This target is a design choice, not a manufacturer continuous-duty rating.

FRICTION, GRAVITY AND ALL CARRIED MASS
The calculation propagates the actual mesh volume/center of mass through each joint. It includes all downstream plastic, the relevant 12/13.8/55 g motors, a 1 g horn and 2 g wiring allowance per stage, and the extra face electronics or swan payload. A motor driving a joint loads its parent; the motors farther along the neck load that joint. Removed red scaffolding is not moving mass.
PLA density is ASSUMED 1.24 g/cm3. Gravity is evaluated through sampled poses. An acceleration of 2 rad/s2 provides a modest inertia term. Coulomb friction is evaluated at ASSUMED coefficients 0.20, 0.35 and 0.50, using 27 mm yaw for giraffe/plant, 40 mm yaw for swan, 7 mm transverse and 8 mm face-bearing radii. The overhung plant face also uses a contact-load multiplier for its 18 mm bearing span. Yaw friction also includes an assumed radial contact pair when the moving center of mass extends beyond the thrust radius: multiplier = 1 + 2 * max(0, horizontal COM offset - thrust radius) / 15.4 mm contact span. This allows the thrust face to carry an off-center load up to its radius; extra overturning moment loads the radial faces. The wider swan bearing reduces that additional wedging load. The 15.4 mm contact span and this simplified load-sharing model are design assumptions, not measured joint behavior. These values are sensitivity inputs, not measured PLA-on-PLA coefficients for your printer.
The stated budget is worst sampled gravity + worst inertia + the mu=0.50 friction estimate + an additional UNMEASURED 0.10 kg.cm startup allowance. Summing separate maxima is deliberately conservative. Fit error, layer ridges, fused seams and printed bearing load sharing can exceed it. The servo shaft can carry some force too; this estimate assumes printed joints carry the downstream load and motor axes are aligned after the mechanism settles.
Measure startup friction with motors disconnected: apply a small spring scale at a known lever distance. Torque in kg.cm = force in newtons * radius in cm / 9.80665. Do it with dummy masses representing all carried motors and the chosen payload, through the desired bend angles. Compare measured peak plus gravity/contact torque with the chosen motor before increasing motion speed or range.

ASSEMBLY AND POWER
Remove every starter wall/post and bearing tie before powering a motor. Move each joint through its range by hand. The tall exterior walls may attach to more than one member along the way; remove the entire wall, not just its top. Clean seam lumps and print strings, without sanding away the retaining shoulders. Captive joints should remain retained after the red scaffolding is removed.
Insert motors through the open bracket face and fit one stock horn to each driven plate. Use the center-access hole for the servo screw. Let the yaw thrust face seat under gravity before tightening case fasteners. Leave ear screws loose while aligning the motor shaft to the released joint; tighten after free motion is established. The fit gauge must establish horn-plane alignment on your specific motor. Do not assume any product called MG90S, MG92B or MG996R has identical accessories or dimensions.
Angles in the CAD are relative to the printed pose, not absolute PWM commands. Set each servo to a suitable neutral command before fitting its horn. For a -105-degree swan neck sweep, one possible starting calibration is printed neutral at about 135 degrees, reaching 30 degrees; verify the motor's actual travel before using those numbers. Do not force a servo beyond its mechanical stops.
Use a regulated 5 V motor power rail and common ground with the controller. MG92B is specified for 5 V or higher supply; Adafruit explicitly calls for a 5 V control signal as well. A 5 V-compatible PWM driver/level translation avoids relying on unverified 3.3 V signal thresholds. The swan needs four independent position channels; the other models need three. Power the servos from their own supply rather than through a tiny controller regulator. Supply current must be checked with the exact motors; a 4-5 A bench supply is an initial budget, not a verified stall-current sum. The older MG996R sheet reports 2.5 A stall at 6 V while the current manufacturer page reports 1.4 A, so do not size power from one optimistic number.
Positional hobby servos command angle, not force. The 0.3 N contact and 25 g payload values are engineering load cases, not implemented force limits. Start with slow movements, pads/compliance and an anchored base. Control electronics, a clamp, force sensing and firmware are not included in these mechanical prototypes.

VERIFICATION LIMITS
OpenSCAD mesh exports completed. Each intended plastic moving member was checked for watertight volume and a single positive connected component. The mechanism was examined at 0.20 mm layer spacing for newly unsupported islands; that test does not prove bridges or overhangs will print. Sampled boolean collision checks include purchased motor/horn envelopes, the table and every GIF frame. Finite sampling does not prove clearance at every continuous angle, and illustrative servos cannot certify your hardware dimensions.
Full slicer logs and generated G-code are kept out of the deliverable. verification.txt and engineering-results.json summarize the checks and remaining warnings. This package contains no physically measured print fit, friction, temperatures, tip force, wear, payload demonstration or successful classroom print.

SOURCE LINKS (checked 2026-10-01)
DFRobot SER0039: https://www.dfrobot.com/product-1338.html
DFRobot dimensional drawing: https://raw.githubusercontent.com/Arduinolibrary/DFRobot_DMS_MG90/master/DMS-MG90%20Servo%20Dimension.pdf
TowerPro MG92B: https://towerpro.com.tw/product/mg92b/
Adafruit MG92B control-signal note: https://www.adafruit.com/product/2307
TowerPro MG996R: https://towerpro.com.tw/product/mg996r/
Older TowerPro MG996R drawing mirror: https://www.mysensors.org/dl/57c421ad4d04abe84cd93e17/design/MG996R_Tower-Pro.pdf
Pololu FS90R continuous servo behavior: https://www.pololu.com/product/2820
FEETECH STS3215: https://www.feetechrc.com/20210430-56680.html
