SWAN NECK v02 — five-motor mechanical motion prototype
2026-10-01

The extra motor adds a third neck bend. Together with body turning and an opening beak, it can make an S-curve, lower its head, lift and sweep. The motors sit in recessed service pockets; covers are removable. This version has no camera, microphone, expressive eyes or sensing electronics. It is a remote-control mechanism.

START WITH A SMALL PRINT
Print clearance-fit-gauge.stl and the motor fit gauges, then embedded-middle-joint-coupon-DRAFT.stl. The coupon reproduces the captive bearing, A20-family pocket and stock-horn attachment. Test release, motor entry, screw access and dry joint friction before committing to the tall complete mechanism.

This is an unprinted prototype. The Ender-class slicer review completed, but both the main mechanism and small joint coupon still report floating/overhang regions and clipping diagnostics. Separate 0.20 mm layer-connectivity checks found no new disconnected layer islands in either print. That does not certify bridges, support removal, friction, strength or print reliability. The full STL and coupon remain marked DRAFT; no qualified G-code is included.

The default Ender-3 V2 slicer estimate for the main print is about 43.5 hours and 332 g of PLA including sacrificial structures. The small joint coupon estimate is about 4 hours and 29 g. These are estimates from the reviewed profile, not observed print times or measured masses. Inspect your own slicer's layer preview before either trial.

WHAT TO PRINT
swan-print-in-place-DRAFT.stl: one captive mechanism, including sacrificial supports. Purchased motors and horns are not included in this print.
motor-covers-DRAFT.stl: four flat service covers on a separate plate.
embedded-middle-joint-coupon-DRAFT.stl: the recommended first mechanism trial.
clearance-fit-gauge.stl: left-to-right gaps 0.30, 0.40, 0.45 and 0.50 mm per radial side.
MG996R-fit-gauge.stl, MG92B-fit-gauge.stl, SER0039-fit-gauge.stl, A20-family-fit-gauge.stl: check actual cases, flange holes and horns.
swan-neck.scad and servo-toy-library.scad: editable CAD; select parts and adjust gap.
swan-neck.blend: actual mesh assembly with the 96-frame motion study.
swan-motion.gif: animated motion, with illustrative timing.

ORDINARY HOBBY PRINTER STARTING SETTINGS
220 x 220 x 250 mm build volume, 0.4 mm nozzle, PLA, 0.20 mm layers, three walls, 15% infill. Keep the upright orientation. A 4 mm brim can help the tall sacrificial posts. Use your filament's temperature settings. The main envelope is 160 x 182 x 249 mm: check actual available Z travel, including printer-specific exclusions. The covers print flat as supplied.

The model already includes removable starter walls and posts. Automatic supports placed into captive running gaps can lock the joints, so inspect the slicer's preview before a test. The supplied scaffolds also need physical removal trials. They are red in the print-layout illustration; an ordinary single-colour printer prints them in the same PLA as the mechanism. Render colours can be reproduced by optional paint, without a multi-material printer.

The default radial gap is 0.45 mm per side, not 0.45 mm total diametral clearance. The shaft/body surfaces have roughly 0.4–0.6 mm axial relief, depending on the feature. Measure your printer's elephant-foot and hole shrinkage on the coupons. If a 0.45 mm joint does not release and turn by hand, adjust the gap and reprint the coupon before powering a motor.

MOTOR PLAN — REGULATED 6 V FOR THIS REVISION
0. Body turn: TowerPro MG996R, fixed in the base.
1. Lower neck bend: TowerPro MG996R, embedded in the body pocket.
2. Middle neck bend: AGFRC A20CLM V2, in the lower moving neck pocket.
3. Head bend: TowerPro MG92B, in the middle moving neck pocket.
4. Beak: DFRobot SER0039 / DMS-MG90, in the head pocket.

MG996R is listed at 55 g and 11 kg.cm stall at 6 V. MG92B is 13.8 g and 3.5 kg.cm stall at 6 V. The A20CLM V2 manufacturer's sheet lists 21 g and 7.5 kg.cm stall at 6 V. SER0039 is 12 g; this calculation conservatively uses its 1.8 kg.cm stall specification at 4.8 V even though the proposed rail is 6 V. Stall torque is not a continuous operating rating.

The A20 pocket is PROVISIONAL: the manufacturer's A20CLS download links a STEP file named A20BHM-whole.stp. Its shaft, flange and mounting geometry were used as an A20-family reference. They have not been confirmed on an A20CLM unit. The CAD proxy includes the shaft, so its total height differs from the advertised 27.5 mm body dimension. Print the A20-family gauge before buying a class set or assuming drop-in fit. The other flange patterns and stock horn drawings also require physical checks; compatible splines and actual screw positions vary between products and clones.

Use the purchased spline horn on each servo. The printed plate has adjustable radial slots for a representative two-arm horn, with holes around 8–13.5 mm radius. Prefer two small screws between the stock horn and printed plate. Shaft-centre screwdriver bores and radius-12 access ports are provided. Confirm screw head and screwdriver access on the coupon, using the actual supplied horn. The plastic journals carry the bending load; the stock horn transmits torque. No extra gears, belt, cable-driven tendon or fabricated spline is required.

HARDWARE AND POWER
Five positional servos and their correct stock horns and shaft screws; suitable motor-ear bolts, washers and nuts; eight M2 x 6 self-tapping screws for the optional service covers; two adhesive rubber pads about 11 x 7 x 1 mm; a 5 V logic PWM controller and regulated 6 V motor supply. The ear holes are clearance holes: do not assume the supplied servo screws self-thread into them. Standard mounts use adjustable 3.6 mm slots, mini mounts 2.8 mm holes, and the A20-family mount 2.6 mm slots. Choose fasteners after checking the gauges.

Budget roughly 8–10 A for a current-limited bench supply as a planning starting point. The combined real stall/peak current has not been verified for this mixed set. Keep motor power off the small controller board's regulator, and connect controller and motor grounds. The MG92B vendor calls for 5 V control pulses; use an appropriate logic interface with a 3.3 V controller. The final controller, wiring and thermal behaviour are not part of this CAD verification.

ASSEMBLY ORDER
1. Let the print cool. Remove the sacrificial structures and break the tiny bearing ties. Clear every running gap and move each joint by hand with no motor installed. The full-assembly support access remains a physical test requirement.
2. Check each servo alone, establish its direction and measured travel, and clock its stock horn to the required assembly angle. The supplied angle files use CAD-relative degrees, not ready-to-run PWM commands.
3. At body-turn zero, lower the base motor through the top opening. Add its stock horn separately through the service opening and attach it to the rotating roof. This separate horn-installation step was not mechanically rehearsed with real hardware.
4. Slide the lower-neck motor and its stock horn in from the front. A narrow flange-entry slot in the stationary base lines up with the moving carrier at the assembly angle.
5. Slide the middle-neck motor in from the front. Verify the A20-family flange and horn first.
6. Raise the empty head to approximately +60 degrees at the head bend, then insert the head-bend motor from the front. This moves the head shell clear of the installation path. Return the head to assembly zero before securing horn screws through the service ports.
7. Open the beak to about -23 degrees and insert the beak motor from the rear. The rear service corridor and two outer head braces keep the beak structure clear. Return it to assembly zero for horn fastening.
8. Secure motor ears, stock-horn screws and optional covers. Cover pilot holes start at 1.6 mm; test the chosen M2 screws in PLA first. Add the two rubber pads and route slack wire loops outside the journals. Cable routing is a physical check, not part of the collision model.

REACH, MOTION AND CONTACT
The checked open-beak desk pose is [turn 0, lower -89, middle -30, head +60, beak -23] degrees. The nominal lower-beak contact point is 179.6 mm forward from the turn axis and 3.4 mm above the desk.

The animation uses synchronized quintic easing, which slows into and out of each key pose. For initial physical motion, start around 20–30 degrees/second, with smooth acceleration no higher than the 2 rad/s2 used in the torque estimate. The GIF's playback speed is illustrative. Gear backlash, actual servo quality, PLA stick-slip and control tuning determine how fluid the real mechanism feels.

The motion file includes rest, extension, reach, close, lift, turn and release. The beak closes to +8 degrees in the tested animation; +13 degrees causes a plastic clash. Do not use +13. The rubber pads target light, bunched cloth; the model has not picked up a sock. Start empty, then test a small cloth around 10 g. The 25 g case in the calculation is a planning load, not an established payload rating. Flat material on the desk and different keyboard actuation forces need separate trials.

The mechanism has a broad range but cannot use all extreme angles simultaneously. Of 375 deliberately broad grid poses, 166 were clear; the others included self-contact or desk penetration. The complete 96-frame delivered path is clear at the sampled frames. Use that path and the validated pose bank as starting points, rather than independently combining extreme slider values. Digital sampling does not prove every continuous trajectory is safe.

Raising the neck before turning reduces the yaw-bearing load. Anchor the base through the four mounting holes or secure it for contact and sweeping tests. No dynamic throw, real pushing force, tipping, cloth friction or sustained-load thermal performance has been demonstrated. The head can perform a reach/sweep/release motion; a reliable toss needs additional physical grip and acceleration tests.

WEIGHT AND TORQUE ESTIMATE
All printed bodies are treated as fully solid PLA at 1.24 g/cm3, including the covers. All downstream motors, representative 1 g horns, 2 g wire allowance per stage, 2 g pads and a 25 g load are included. This intentionally overestimates many 15%-infill bodies; weigh the real print before refining the estimate. Fully solid base mass is not a verified stability benefit for an infill print.

The assumed dry-contact friction range is mu = 0.20, 0.35 and 0.50. These are sensitivity assumptions, not measured PLA coefficients. The calculation includes yaw overturning load, a 46.9 mm assumed radial-contact span and a 0.1 kg.cm unmeasured startup reserve. Peaks are summed conservatively. It uses 2 rad/s2 for inertia. The taller base sleeve supports the yaw mechanism over a longer span than v01.

Rows below are [body turn, lower bend, middle bend, head bend, beak].
Downstream mass, g: 370.1, 217.7, 152.3, 94.5, 40.2
Planning torque with mu 0.50, kg.cm: 2.654, 2.466, 1.352, 0.616, 0.230
Chosen comparison target (25% of stall), kg.cm: 2.750, 2.750, 1.875, 0.875, 0.450

The 25%-of-stall target is our conservative comparison, not a manufacturer's continuous rating. Yaw has little margin at the worst extended static geometry. The approved animation turns with the neck partly raised; its active-yaw planning estimate is 2.397 kg.cm. Contact forces add to these estimates. engineering-results.json includes the extra moments from an assumed 0.3 N contact; the 25 g gravity-only case should not be treated as a proven keyboard-press or pushing rating.

VERIFICATION AND LIMITS
The final digital checks, including mesh, sampled collisions, installation paths, framing and slicer review, are recorded in verification.txt. The motor-entry check samples each modeled case, and four preattached stock horns, along its specified route against the plastic and previously installed modeled motors and horns, with covers and scaffolds removed. It does not establish real spline engagement, fastener handling or assembly ease.

SOURCE LINKS
TowerPro MG996R: https://towerpro.com.tw/product/mg996r/
TowerPro MG92B: https://towerpro.com.tw/product/mg92b/
AGFRC A20CLM V2 manufacturer sheet: https://usaimages.oss-us-west-1.aliyuncs.com/4939/Lucy%202026%E6%96%87%E4%BB%B6%E5%A4%B9/MINI%20SERVO/A20CLM/A20CLM%20-%20EN%2020260126.pdf
AGFRC A20-family CAD download source: https://www.agf-rc.com/agfrc-a20cls-7-5kg-fast-compact-coreless-digital-servo-p1671217.html
DFRobot SER0039: https://www.dfrobot.com/product-1338.html
DFRobot dimension drawing: https://raw.githubusercontent.com/Arduinolibrary/DFRobot_DMS_MG90/master/DMS-MG90%20Servo%20Dimension.pdf
MG92B control-voltage vendor note: https://www.adafruit.com/product/2307

Original v01 variants are preserved in the sibling toy-design-variants folder. This folder is the separate v02 swan revision.
