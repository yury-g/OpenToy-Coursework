# Regenerating ditherCam 3D Renders and Animations

The scripts here render the hero still and three animated CAD GIFs directly from [`../cad/`](../cad/) and the firmware-rendered UI textures in [`../../../ditherCam/docs/images/`](../../../ditherCam/docs/images/).

1. First generate the firmware UI PNGs (if not already present):
   ```sh
   uv run --with pillow --with numpy python3 ../../../ditherCam/tools/generate_ui_previews.py
   ```
2. Render the hero PNG and frame cache (individual frames go into `.frame-cache/`, which is git-ignored):
   ```sh
   /Applications/Blender.app/Contents/MacOS/Blender -b -P render_dithercam.py
   ```
3. Assemble the GIFs, strip metadata, verify decoded frame counts/motion, and update `../verification/media-manifest.json`:
   ```sh
   uv run --with pillow --with numpy python3 assemble_media.py
   ```
