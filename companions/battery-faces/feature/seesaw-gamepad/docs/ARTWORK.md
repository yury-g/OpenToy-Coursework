# Preserve and regenerate the artwork

The SVGs are original editable board illustrations, not CAD renders. Their screen contents come from the exact native framebuffer captures produced by `tools/verify_faces_family.py` on v18, with Pink/Nesso and Yellow retaining explicitly labeled last-verified v15 captures. Source bitmaps for the illustrative mood animation are exported by `tools/export_poses.cpp` using the preserved OpenToy renderer and battery pose rules.

1. Capture the connected family battery views with `tools/verify_faces_family.py`. This records missing USB devices instead of claiming their current state.
2. Compile/run `tools/export_poses.cpp` with a C++17 compiler; save its JSON output as `assets/animation/renderer-poses.json`.
3. Run `python3 tools/render_artwork.py` with Pillow installed. This preserves all workspace PNG captures in `assets/archive`, then emits self-contained pixel-path SVGs. No externally loaded image or script is required to display them.
4. Run `node tools/render_assets.js` with Playwright and Chromium or the configured local Chrome executable available. This renders the board SVGs and captures five deterministic times in the animated SVG.
5. Run `python3 tools/finalize_assets.py` with Pillow to assemble `battery-mood.gif` and create the all-images ZIP and refresh `assets/manifest.json` after all raster files exist.

The GIF is the GitHub-compatible README animation. The SMIL SVG stays available for editing and compatible viewers. Both demonstrate changing estimated charge/mood; neither is real-time telemetry or a battery-cycling controller. Historical screenshots retain their version names and can include retired Green before the current heat hold.

Manufacturer CAD and reference files are preserved separately in `hardware/references`, with attribution and original licenses in `hardware/BOARDS.md`. The README drawings do not derive their simplified component placements from a precision mechanical model.
