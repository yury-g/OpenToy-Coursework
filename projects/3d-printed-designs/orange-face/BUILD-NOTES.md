# Orange Face — build notes

[Visual project guide](README.md) · [3D printed designs](../README.md)

First-fit prototype; button-access revision, November 16, 2026. One printed object holds the three boards from the existing Orange Face assembly: Adafruit Reverse TFT Feather #5691, MPR121 #4830 and LSM6DS3TR-C #4503. No separate tray, screws, nuts, inserts or printed retainers are required. This version has an open back and an integral protective rim; it is not a closed case. A battery was not part of the reference assembly and is not fitted or modeled here.

The exterior is 110 × 176 × 16 mm. The front skin is 3.2 mm thick. The touch board's back electrode surface remains flush with the front. Both touch sockets and all modeled leads sit underneath the faceplate, within the rim. The display and touch positions are retained; the motion sensor moves beside the display.

## Print

Open **[Orange-Face-One-Piece-X1C-PLA.gcode.3mf](print/Orange-Face-One-Piece-X1C-PLA.gcode.3mf)** in Bambu Studio. It contains the single model and verified sliced G-code. The saved settings are the previously used Bambu Lab X1 Carbon / 0.4 mm nozzle / 0.16 mm High Quality / Bambu PLA Basic / Cool Plate SuperTack, with wall loops increased from two to four and support disabled. All other inherited machine and filament settings are retained, including the saved extruder offset. Estimated total time is **2 h 19 m 36 s**, with **68.41 g** of PLA. This has not been sent to a printer.

Print with the flat exterior face on the bed and the open back facing upward, as supplied in the STL. Do not auto-orient or uniformly scale to tune fit. Tiny touch seating shelves project about 0.8 mm underneath the PCB edges; inspect their first unsupported lines after printing. No support material is present in the supplied slice. The render's orange color, lettering, pad labels and display face are illustrations; the print is a single-material part without lettering. Select the intended PLA spool when printing.

## How the mounts work

Ten integrated planar cantilever fingers flex sideways within the printed layers: four for the Feather, four for the touch board and two for the IMU. Each nominal beam is 16 mm long and 1.6 mm wide, with a 0.8 mm outer relief slot. The unloaded contact pads extend 0.15 mm into the nominal PCB edge on each side. That deliberate interference is taken up by spring deflection during insertion. These dimensions are initial targets, not measured retention forces.

Four small fixed shelves set the touch PCB's depth using its nominal 1.60 mm thickness. The touch board relies on side friction to prevent it lifting out toward the front; the fixed shelves carry finger pressure inward. The Feather and IMU also have shallow release ramps over their rear edges and fixed end stops. The supports contact PCB regions, not the screen glass or electronic components.

## Assemble and test

1. Let the print cool. Remove any strings from the spring slots, ledges and lead recesses. Gently check each finger can move outward; do not force it past its relief gap.
2. With power disconnected, insert the touch PCB **from the exterior**, back electrodes outward and components inward. Seat it evenly on all four shelves. Check the exposed face is flush and does not rock. Connect its two QT leads from the open back after it is seated.
3. Fit the Feather and IMU from the open back. Ease the opposing fingers outward and press on the bare PCB edges until the boards rest on their supports. Release the fingers so the retaining ramps sit behind the board edges. Do not press on the glass, chips or sockets.
4. Route the leads as in **[02-Electronics-Held-in-Faceplate.png](images/02-Electronics-Held-in-Faceplate.png)**, keeping the bend beside the Feather clear of its battery socket. Use suitable lead lengths and keep slack inside the rim; the render shows an intended route, not a measurement of your actual cables. The wires have no separate strain-relief clamps in this revision.
5. Check all boards stay seated when gently inverted, the USB plug reaches the socket freely, the buttons work, and all twelve electrodes respond. Record any looseness, excess force, whitening or cracking. To remove a board, flex its fingers outward and lift the PCB evenly; do not use the glass or connectors as handles.

Physical grip, repeated removal, impact resistance, exact PCB thickness, printer tolerance and actual cable stiffness have not been tested. If a board needs substantial force, stop and adjust the contact dimensions rather than forcing it. Repeated-flex versions may benefit from PETG, but that needs its own compatible material/plate profile and a new fit test; the included G-code is specifically PLA. [Protolabs' snap-fit guidance](https://www.hubs.com/knowledge-base/how-design-snap-fit-joints-for-3d-printing/) discusses ductile materials and printed joint design; [Prusa's PETG guidance](https://help.prusa3d.com/article/petg_2059) covers PETG for holders and clamps.

The IMU's installed orientation differs from the earlier tray version. Check motion direction and remap the firmware axes if needed after assembly; this mechanical update does not change firmware.

## Files and validation

- **[Orange-Face-One-Piece.stl](print/Orange-Face-One-Piece.stl)** — the one printed part, already oriented for the bed.
- **[Orange-Face-One-Piece.step](cad/Orange-Face-One-Piece.step)** — the faceplate as one solid, in millimeters.
- **[Orange-Face-One-Piece.FCStd](cad/Orange-Face-One-Piece.FCStd)** — faceplate plus all three boards and routed leads. Hide the Hardware group to inspect mounts. The spreadsheet records dimensions; it does not drive a live feature tree. source/design.py is the reproducible dimension-controlled builder.
- **[Orange-Face-One-Piece.blend](blender/Orange-Face-One-Piece.blend)** — four native Blender scenes: front, installed rear, empty print and detached parts. Hardware and faceplate geometry come from FreeCAD; render artwork is separate.
- **Validation.json / Slicer-result.json** — numerical checks and slicer estimates.
- **source/** — headless FreeCAD/Blender builders, supporting reference geometry and fonts. Set the Source folder on Python's module path before running design.py inside FreeCAD. Run render.py with Blender in background mode after the CAD build. Changing the model requires re-slicing; the G-code does not update itself.

Verified: one valid CAD solid; STEP and FreeCAD reopen with a 110 × 176 × 16 mm envelope; one connected, watertight 14,246-triangle STL; no unintended hardware or shell intersections; all ten intentional interference contacts touch only PCB substrates. All modeled hardware and cables stay inside the protective envelope. The rendered installed contact geometry trims the nominal preload overlap to depict contact without penetration; it is not a deformation simulation.

The Bambu archive contains one sliced object, no supports, 100 layers and a matching G-code checksum. Toolpaths were checked at the first layer and at 1.0 mm height, at three locations along each spring's outer relief slot: all sampled slot centerlines retain at least 0.399 mm clearance to the deposited line footprints. This is a geometric/toolpath review, not physical qualification.

Hardware reference geometry derives from the [OpenToy Orange Face chapter](https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/orange-face) and [Adafruit's MPR121 PCB files](https://github.com/adafruit/Adafruit-MPR121-PCB). See the accompanying attribution and license files. Earlier enclosure outputs are preserved separately.

## Front and clip removal close-up

**[01-Front.png](images/01-Front.png)** shows the flush touch face. **[05-Clips-Hold-and-Release.png](images/05-Clips-Hold-and-Release.png)** shows an actual CAD section of a Feather mount, alongside an illustrative released position. The long spring moves sideways, allowing the short retaining lip to clear the PCB edge. The section is only for teaching; the printable part remains whole. **[Clip-Removal-Closeup.blend](blender/Clip-Removal-Closeup.blend)** contains this editable diagram.

Disconnect power and leads first. For the display and motion boards, flex the clip tips away from the board with a fingernail or thin plastic pick, release the clips along one edge, lift that bare PCB edge slightly, then free the opposite side. Work on the PCB edges, not the glass or components. The displayed displacement is illustrative, not a measured required travel or a verified force.

The flush touch board differs: its side fingers supply friction and its fixed ledges support touch pressure. Ease those side fingers outward, then push gently on bare PCB edge regions from underneath to move the board **out through the front opening**. It cannot be extracted through the smaller rear seat. Physical removal and retention remain to be tested.

## Feather button correction — November 16, 2026

All five simplified switch blocks were replaced with Adafruit's STEP solids:
three user switches on the display side, one display-side Reset and one rear
Reset. The source model's rear switches are adjusted by 0.03 mm to sit on this
assembly's nominal 1.60 mm PCB; front switch coordinates are preserved exactly.
The switch body is 3.8 × 2.8 mm (Reset is rotated 90°), with a 2 mm diameter
actuator reaching 1.9 mm above the board surface. At the existing board seat,
the front actuator is therefore 1.8 mm below the exterior cover.

The user-button openings now form one bay joined to the screen opening. The
reset opening is widened separately. FreeCAD checks confirm no cover material
inside 7 mm diameter approach cylinders for D0, D1 and D2, and 5 mm for Reset,
from the exterior through 0.3 mm beyond the resting actuator plane. This checks
cover access only; it does not establish switch travel, comfortable fingertip
access or the installed board revision. Do not use the display as a hand rest.

Render labels use D0, D1, D2 and RST. D0 is active-low and doubles as BOOT; D1/D2
are active-high inputs. Reset is separate. Their behavior in a particular OpenToy
program depends on that program; this update does not change or flash firmware.
Test individual press/release and D0-plus-Reset access on the real assembly.

Source: [Adafruit's STEP model](https://github.com/adafruit/Adafruit_CAD_Parts/tree/main/5691%20Feather%20ESP32%20S3%20Reverse%20TFT),
[PCB source](https://github.com/adafruit/Adafruit-ESP32-S3-Reverse-TFT-Feather-PCB),
and [button pinouts](https://learn.adafruit.com/esp32-s3-reverse-tft-feather/pinouts).
The preserved clip close-up depicts an unchanged section of the faceplate.
