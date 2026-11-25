> **Historical mechanical handoff note (pre-wireless enclosure revision, November 2026):** Preserved as the original CAD/dashboard reference note. The "Current video behavior" section below predates the November 25, 2026 two-board wireless photos-only firmware; see [README.md](README.md) and [the ditherCam project guide](../../ditherCam/README.md) for current enclosure/firmware behavior and [`cad/`](cad/) / [`verification/`](verification/) for the repo paths (`model/` and `validation/` below correspond to `cad/` and `verification/`).

# Top-flip camera — faceplate revision / PLA / Snapmaker U1 prototype

Open `index.html` for the offline 3D dashboard. It uses the exported CAD meshes. Drag to orbit, scroll to zoom, use the angle slider or Forward/Selfie buttons, and toggle Print/use to inspect its bed orientation. Electronics are approximate vendor-based envelopes, not scans of your installed devices.

The rotating pod now has an integral stepped faceplate with separate camera and Expansion Base OLED openings. Its board stack slides in through the open top. RLCD KEY is labeled **Snap**, BOOT is labeled **Browse**, and Power stays separate; this revision changes the case, not button firmware. The dashboard opens in selfie mode to show both windows.

The enclosure has a fixed landscape RLCD cradle, an integral flat standing base, and a captive top pod carrying the whole XIAO Sense + Seeed Expansion Base + its battery. It rotates between outward-facing and selfie positions. Gravity seats the pod against rigid stops; the board retainers are compliant. No separate printed hinge pieces, pins, screws or glue are needed. Electronics still have to be installed.

## Printable prototype

Import `model/Camera-print.3mf` as one object and retain its arrangement. The STL is an alternative. Do not split, individually arrange, or scale the two captive solids.

- PLA; 0.4 mm nozzle; 0.20 mm layers and first layer; 4 walls; 20% infill.
- Supports off, raft off, brim off. A brim crossing the pivot gaps could fuse the joint.
- Print envelope: **108 × 152.1 × 58 mm**. The assembly prints lying down with its pod pointing upward; turn the whole object onto its flat sole for use.
- Standing base: **108 × 58 mm**. Overall height is approximately **126.5 mm forward / 130.7 mm selfie**.
- Pivot nominal clearance: 0.50 mm normal to the cone, 0.65 mm axial face gap. Release, looseness and wear are untested on the U1.

This is a prototype for review and fit testing, not a physically validated enclosure. No print has been started.

## What was checked

- Body and pod are each one watertight connected solid; the printable assembly has exactly two shells. STL and 3MF preserve their relative positions.
- Independent check: 181 angular positions at each of three axial positions (center and ±0.6 mm), **543 poses total**, with zero detected moving-part/hardware collisions. No static RLCD overlap in the modeled envelopes. Assembly matches the inspection meshes.
- Layer-section checks found no floating islands at 0.20 mm layers. A **Bambu Studio reference slice** completed with no support or raft toolpaths. This is not a validated U1 printer profile or a physical overhang test. The model includes small bridges and local chamfers that still depend on actual PLA settings.
- Estimated level-table balance passed 2,048 mass/center-of-mass sensitivity cases at the two end positions. The most front-heavy modeled selfie case had only about **0.8 mm** front margin. This is a marginal result, not a robust stability guarantee. These are assumptions, not measured weights; test actual loaded balance. Cable pull, slope and movement were not evaluated.
- The complete OLED active rectangle clears the shell straight on and at tested ±15° horizontal/vertical angles (675 independent sample rays). The modeled 120° lens cone remains unobstructed.
- Camera-stack insertion was checked at 101 positions over a 50 mm slide, excluding only the two designed bottom detent tips: maximum rigid overlap 0.00104 mm³, below the 0.01 mm³ numerical gate; no seated overlap. Physical spring behavior remains untested.
- Dashboard controls, animation, desktop rendering and mobile layout were checked in a browser.

Full records are in `validation/`. The original author report includes nominal solid-volume mass estimates; use `balance-sensitivity.json` for the broader final sensitivity check. Neither is a measurement of printed weight.

## Fit assumptions to confirm before committing to a full print

- Expansion Base: 58 × 42.5 mm; nominal seated header gap 7.6 mm.
- Camera stack: existing short-flex, directly plugged stack; actual camera variant is unconfirmed.
- Pod battery: **58 × 42.5 × 4 mm assumed**, in a 6 mm bay. Include wrapping, leads and connector in the physical fit check. This model does not establish your battery's actual size.
- RLCD: vendor V2 geometry, with provision for V1/V2 bare depth and the existing 18650 holder/cell. Actual revision remains unconfirmed. Remove the original acrylic stand legs before fitting the unit.
- Antenna outline and coax route remain unmeasured. Keep access and airflow open.

## Electronics installation concept

With power off, fit the RLCD through the open rear, tucking one edge beneath the fixed keepers and gently deflecting the accessible retainers. The bezel clears the modeled glass; the case is intended to support the assembly at its perimeter.

Remove protruding MicroSD cards and cables, then slide the plugged XIAO/base stack through the pod's open top behind the integral faceplate. Bare-board guides position the stack; the two accessible bottom spring detents engage its mounting holes. Reinsert cards through the side openings after seating. Insert the assumed-size battery through the side bay with its latch released. Do not use the pouch to force a latch aside. The camera stack's rigid loading path has been checked digitally; physical installation and clip deflection remain untested. The separate RLCD tilt/snap installation remains a proposed, physically untested method.

The housing carries hinge loads. Physical installation must avoid loading the screen glass, board headers, camera ribbon or battery pouch. Removing/inserting electronics is still necessary; “no assembly” refers to the printed mechanism.

## Current video behavior

The OLED window exposes the existing screen; live video on that OLED is **not implemented**. Snap/Browse labeling does not implement capture or browsing firmware. The earlier tested RLCD preview uses a Mac relay. Direct camera-to-RLCD wireless operation is not implemented by this enclosure work. The mechanical layout anticipates separate batteries; USB access remains available. The top flip reverses camera orientation, so one mode requires software image rotation.

## Source and regeneration

`model/Camera.scad` is the editable parametric model in millimeters. Default export is the captive print pose. OpenSCAD CGAL generated the verified meshes:

```sh
openscad --backend=CGAL -o Camera-print.stl -D 'part="print_assembly"' Camera.scad
openscad --backend=CGAL -o body.stl -D 'part="body"' Camera.scad
openscad --backend=CGAL -o pod.stl -D 'part="pod"' Camera.scad
```

Inspection meshes are in `model/`; the dashboard's authoring source is in `source/`. Only the combined Camera-print model is intended for printing. The HTML is self-contained and can be opened without a server or internet access. The model's hard-coded placements include vendor-specific geometry; changing dimensions requires rechecking fit, motion, printability and balance, not just re-exporting.
