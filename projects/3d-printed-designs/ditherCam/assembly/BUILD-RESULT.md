# Top-Flip Enclosure Interactive Assembly Guide — Build Notes

## Deliverables & Editable Source Paths

- **Self-contained HTML deliverable:** [`index.html`](index.html) (embeds Three.js 0.185.1 bundle + binary-converted CAD STLs + `cad/parameters.json` + `verification/independent-insertion.json` summary; zero runtime network or CDN requests)
- **Editable source files:**
  - [`source/assembly-template.html`](source/assembly-template.html)
  - [`source/assembly.js`](source/assembly.js)
  - [`source/build.py`](source/build.py)
  - [`source/package.json`](source/package.json) & [`source/package-lock.json`](source/package-lock.json) (`three@0.185.1` and `esbuild@0.28.2`)
  - [`source/THIRD-PARTY.md`](source/THIRD-PARTY.md)

## Design & Verification Notes

- **Physical plain-language copy:** Step titles, subtitles, summaries, bullets, clip cards, and callout labels use plain physical descriptions; exact coordinates and validation figures live in `window.assemblyGuideState` and the **CAD vs Fit** tab.
- **Loading orientation & downward insertion alignment:** Steps 1–5 default to `orientation: 'print'` (housing lying flat face-down on a soft cloth, open rear RLCD bay and `90°` pod top entry pointing straight upward), matching the downward insertion motion shown in the viewport. Step 6 switches to `orientation: 'use'` (standing upright on the flat foot sole) for inspection, `0°–180°` captive hinge sweep, and optional battery side insertion.
- **Default transparent shell & truthful callouts:** `transparentShell: true` by default so internal clips and detents are visible immediately; `hw_xiao_pod.stl` (which bundles battery + SD) is excluded in favor of the bare stack `hw_xiao_stack.stl` + `hw_oled_active.stl`.
- **Safety guidance:** Stop immediately if binding or PLA stress whitening occurs and identify which highlighted tab or clip is making contact first.

## Honest Limitations

- **CAD validation vs real physical fit:** Rigid pod slide (`0–50 mm`, 101 samples, worst `0.00104 mm³`) excludes the two bottom detent tips; RLCD tilt animation is illustrative only and not validated; clips/retainers are not animated as validated elasticity; camera geometry is a vendor envelope (`OV2640` firmware sensor); actual header heights, RLCD revision, and battery thickness remain unmeasured.
