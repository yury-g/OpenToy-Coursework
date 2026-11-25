# ditherCam Assembly Guide — November 25, 2026

Open [`index.html`](index.html) offline directly in a browser (zero network requests), or visit the hosted public assembly webpage at **https://dithercam-assembly.narwhal2025.chatgpt.site** ([3D fold viewer](https://dithercam-assembly.narwhal2025.chatgpt.site/flip/)). Choose RLCD or Camera Pod, use Next/Prev, drag the insertion slider, and toggle transparent shell to inspect the highlighted retainers. Front/rear/top and exploded views are available. The printed hinge remains captive in every view.

Geometry comes from the CAD meshes in [`../cad/`](../cad/). Electronics are vendor envelopes. The camera stack's rigid slide was checked digitally, excluding flexible detent tips; RLCD tilt seating is illustrative and physically unverified. Stop if the actual parts bind or PLA whitens.

Editable authoring files are in [`source/`](source/). Run `npm ci` inside `source/` and `python3 source/build.py` to rebuild `index.html` from [`../cad/`](../cad/) and [`../verification/independent-insertion.json`](../verification/independent-insertion.json).
