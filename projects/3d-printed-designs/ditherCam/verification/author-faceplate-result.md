# Faceplate Revision Result (`FACEPLATE-RESULT.md`)

**Status:** Prototype CAD and slicer geometry checks passed; **not physically printed, assembled, or fit-verified**. No firmware, dashboard source (`deliverables/index.html`), electronics, hardware devices, or printer actions were touched.

---

## 1. Summary of Mechanical Changes

1. **Snapshot Preserved:** Prior baseline deliverables were snapshotted unmodified under `previous-faceplate-baseline/`.
2. **Integral Stepped Pod Front Faceplate (`deliverables/Camera.scad`, `pod_print()`):**
   - **Camera Side (`X_p <= -11.00 mm`, `Y_p` front at `-47.60 mm` / `z_e = 25.40 mm`):** Protective shroud encloses the `23.16 mm` Sense camera stack with an integral `120°` FOV conical aperture topped by a self-supporting `45°` gothic peak (`pod_lens_aperture_cut()`), keeping `0.0 mm³` interference with the `120°` FOV cone (`hw_lens_cone.stl`) from `18.90 mm` across `0–180°` and `-0.6 / 0 / +0.6 mm` axial play.
   - **Recessed OLED Step (`X_p >= -11.00 mm`, `Y_p` front at `-39.40 mm` / `z_e = 17.20 mm`):** Stepped back `8.20 mm` behind the camera faceplate (`face_oled_inner_ze = 15.00 mm` clears Sense PCB `14.63 mm`, `J7`, Grove headers, and the OLED glass without relocating the `0.96"` OLED to the camera lens plane). `pod_oled_bezel_cut()` provides a `25.44 × 13.76 mm` throat at `z_e = 10.00 mm` (`6.55 mm` forward of the OLED glass top at `3.45 mm`, `0.0 mm³` glass overlap) flaring outward to `33.24 × 21.56 mm` with a `45°` upper gable peak (`0` supportless islands).
   - **Reference Surface (`deliverables/hw_oled_active.stl`):** Added thin active-screen slab (`21.74 × 10.86 mm`, centered at Eagle `(34.80, 21.35)`, `z_e = 3.45..3.75 mm`) for dashboard integration.
3. **Top-Down Sliding PCB Insertion & Integral Retention (`0–50 mm` Rigid Path Cleared):**
   - Because the front is now covered by the stepped faceplate, Expansion Base + XIAO Sense + Camera (with protruding MicroSD cards removed prior to installation) slides **top-down (`-Z_p`, open `+Z_p` pocket)** along bare-FR4 guide tracks.
   - **Unobstructed Component Lanes Verified Across `0–50 mm`:**
     - Cut open top exit in rear underside cavity `D` across `X_p ∈ [-9.50, +29.60 mm]` through `pod_top_z` so `BAT1` RTC holder and `J3` MicroSD slot clear top-down slide; relocated `TR` rear bare-FR4 seat to `X_p ∈ [6.20, 12.80 mm]` (between `BAT1` and `J3`).
     - Added vertical `-X` USB-C shield entrance channel (`X_p ∈ [-31.00, -29.00]`, `Y_p ∈ [-36.30, -32.00]`, `Z_p >= 11.60 mm`) joining the `-X` USB-C/Sense MicroSD side portal.
     - Positioned bare-FR4 front hold-down rails strictly in component-free lanes across `0–50 mm`: `BL` (`[-29.35, -24.80], Z_p = 2.60..7.80`), `BR` (`[+27.95, +29.35], Z_p = 2.60..7.80`), `TR` (`[+27.95, +29.35], Z_p = 38.20..45.20`), and a `45°` sloped Top-Mid-Right gusset rail (`[+18.30, +19.35]`, strictly between OLED `<= +18.17` / Grove `<= +16.60` and `J7 >= +19.51`).
   - **Compliance & Locking:** Two bed-printed `20 × 1.5 mm` compliant cantilevers (`BL`/`BR` corners) carry tapered `Ø2.05 mm` pins at `X_p = ±25.01 mm` that deflect `1.05 mm` (`~0.59%` PLA strain estimate) during final `2–7 mm` travel before seating into the Expansion Base `Ø3.00 mm` mounting holes (`0.0 mm³` seated overlap). The `58 × 42.5 × 4 mm` LiPo still slides independently via the `+X` open pocket (`0.0 mm³` seated interference behind a `20 × 1.5 mm` in-layer latch).
4. **Waveshare RLCD Button Verification & `SNAP` / `PWR` / `BROWSE` Access (`body_print()`):**
   - Primary schematic check (`https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf` + `reference/rlcd-v2.stp` coordinates):
     - **`KEY4` (`X_print ∈ [-10.68, -6.12 mm]`)** -> net `KEY` -> `GPIO18` -> **`SNAP`**
     - **`KEY3` (`X_print ∈ [-0.68, +3.88 mm]`)** -> net `PWR` -> `U3` (`ECJ23001-4FCBD6` power latch IC) -> **`PWR`** (preserved distinctly)
     - **`KEY1` (`X_print ∈ [+9.32, +13.88 mm]`)** -> net `BOOT` -> `GPIO0` -> **`BROWSE`**
   - Lowered rear RLCD bezel shoulder (`y_sole = 104.50 mm`, button bay `Z_print >= 6.90 mm`) preserves finger reach in both Forward (`0°`) and Selfie (`180°`) poses. Front stencil-bridged engravings and top/rear-step tactile characters add integrated `SNAP` / `PWR` / `BROWSE` labels with `0` floating islands.

---

## 2. Exact Changed & Exported Files

- **Modified CAD / Verification / Metadata:**
  - `deliverables/Camera.scad` (added stepped pod faceplate, lens/OLED windows, top-down insertion lanes, `hw_oled_active_print()`, RLCD button opening + integrated labels)
  - `deliverables/parameters.json` (updated `body.rlcd_button_mapping`, `body.sole_depth_y_use_mm`, and `pod.faceplate` parameters)
  - `scripts/build_and_verify.py` (added `hw_oled_active.stl` export, exact no-battery `hw_xiao_stack.stl` `0–50 mm` `0.5 mm`-step rigid vs detent-tip sweep, `0°/±12°/±15°` OLED ray frustum checks, and glass-contact verification)
  - `PROGRESS.md` & `FACEPLATE-RESULT.md` (new revision summary; `deliverables/index.html` and `scripts/build_dashboard.py` untouched)
- **Regenerated Meshes, Reports & Renders:**
  - `deliverables/body.stl`, `deliverables/pod.stl`, `deliverables/Camera-print.stl`, `deliverables/Camera-print.3mf`
  - `deliverables/hw_rlcd.stl`, `deliverables/hw_xiao_pod.stl`, `deliverables/hw_xiao_stack.stl`, `deliverables/hw_oled_active.stl`, `deliverables/hw_lens_cone.stl`
  - `deliverables/verification.json`, `deliverables/render-forward.png`, `deliverables/render-selfie.png`, `deliverables/render-print.png`
- **Snapshot Directory Created:** `previous-faceplate-baseline/`

---

## 3. Build, Verification & Gate Evidence

Commands executed on the final CAD:
```bash
QT_QPA_PLATFORM=offscreen /opt/homebrew/bin/openscad --backend=CGAL -D 'part="pod"' -D 'pose="print"' -D 'fold=90' -o deliverables/pod.stl deliverables/Camera.scad
QT_QPA_PLATFORM=offscreen /opt/homebrew/bin/openscad --backend=CGAL -D 'part="print_assembly"' -o deliverables/Camera-print.stl deliverables/Camera.scad
.venv/bin/python scripts/build_and_verify.py
.venv/bin/python accept.py
```

Observed results in `deliverables/verification.json` and gate output:
- **Watertight Two-Shell Assembly (`Camera-print.stl` & single-compound `Camera-print.3mf`):**
  - `body.stl`: `shell_count = 1`, `watertight = true`, `volume = 69070.1 mm³`, `bed_area = 3038.86 mm²`, `z_min = 0.0 mm`, `com_xy_in_bed_hull = true`.
  - `pod.stl`: `shell_count = 1`, `watertight = true`, `volume = 26453.7 mm³`, `bed_area = 1725.05 mm²`, `z_min = 0.0 mm`, `com_xy_in_bed_hull = true`.
  - `Camera-print.stl` & `Camera-print.3mf`: `shell_count = 2`, `equals_body_plus_pod = true` (`95523.81 mm³`), `bbox_print_mm = [108.0, 152.1, 58.0]` (fits `270×270 mm` bed), `print_pose min_surface_gap_mm = 0.502 mm`, `boolean_intersection = 0.0 mm³`.
- **Full `0–180°` Sweep & Independent Review (`accept.py` / `scripts/independent_review.py`):**
  - `97`-angle sweep + `181`-pose independent sweep across axial play `-0.6`, `0`, `+0.6 mm`: **`0.0 mm³` max shell, moving hardware, RLCD static, and lens-cone intersection**, `assembly_difference_mm3 = 0.0`.
  - Endpoint hard stops (`-3.0°: 25.849 mm³`, `-1.5°: 8.648 mm³`, `181.5°: 233.473 mm³`, `183.0°: 515.435 mm³`) and axial cone retention (`±1.6 / ±2.2 mm`) verified.
- **Rigid Top-Down Insertion & OLED Window Checks:**
  - `xiao_stack_top_down_insertion_sweep` (`0–50 mm`, `0.5 mm` increments, `101` samples excluding ONLY the two bottom `BL`/`BR` detent-tip boxes): `rigid_max_interference_mm3 = 0.00104 mm³` (< `0.01 mm³` gate), `seated_interference_mm3 = 0.0 mm³`, `detent_tip_max_deflection_mm3 = 6.682 mm³`. Passes `accept.py` rigid gate (`Command exited with code 0`).
  - `oled_active_visibility_and_glass_check`: `oled_glass_contact_mm3 = 0.0 mm³`; active `21.74 × 10.86 mm` frustum ray clearance (`25 mm` out) = **`0.0 mm³` obstacle overlap** across ALL 9 tested angles (`0°`, `±12°` H, `±15°` H, `±10°` V, `±15°` V).
- **Supportless Layer Islands & Slicer Identity:**
  - `body_layer_islands` (`290` layers @ `0.20 mm`): **`0` floating islands**.
  - `pod_layer_islands` (`241` layers @ `0.20 mm`): **`0` floating islands**.
  - Slicer identity: `Snapmaker_Orca --info` confirmed `manifold = yes, number_of_parts = 2`; `Snapmaker_Orca` headless multi-extruder CLI exited non-zero, so **`BambuStudio CLI` fallback** (`0.4 mm` nozzle, `0.20 mm` layers, PLA, `support=off`, `raft=off`) generated `290`-layer G-code (`9,717,544` bytes, `support_toolpaths_present = false`, `raft_toolpaths_present = false`). This is a Bambu Studio fallback check, **not** an on-machine Snapmaker U1 validation.
- **Analytic Tabletop CoM Balance (`0–180°`):** Stable across all 4 mass cases (`min_front_margin >= 10.88 mm`, `min_rear_margin >= 32.39 mm`, `endpoint_gravity_hold` positive in both `0°` Forward and `180°` Selfie).

---

## 4. Remaining Assumptions & Limitations

- **Unmeasured Hardware Envelopes:** XIAO Sense camera height (`18.90–23.16 mm` range), module body extents, header stack height (`7.60 mm`), OLED module profile (`3.45 mm` glass top), RLCD V1 vs V2 depth (`13.5` vs `14.1 mm`), and LiPo pouch swelling (`<= 4.0 mm` nominal in `6.0 mm` bay) rely on vendor drawings/reference CAD and remain physically unmeasured on the target units.
- **Installation Order Note:** Both protruding MicroSD cards (Sense + Expansion Base `J3`) must be removed while sliding the Expansion Base + XIAO Sense stack top-down into `pod`, then reinserted through the open `-X` / `+X` side portals once seated; USB-C relay cable access stays open on `-X`.
- **Material/Print Assumptions:** Compliant snap retention (`0.59%–0.60%` surface strain estimate on `20 × 1.5 mm` PLA arms), `0.502 mm` hinge clearance break-free torque, and real Snapmaker U1 bed adhesion/dimensional tolerances are **CAD/analytic estimates only** until test-printed.
