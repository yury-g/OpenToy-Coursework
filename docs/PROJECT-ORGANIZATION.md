# Project organization record

Reviewed November 15, 2026. The owner requested Feather-first browsing and an
archive for XIAO and pre-Feather projects. Hardware assignments were checked
against project READMEs, source paths and available repository branches.

## Correction · November 15, 2026

The owner clarified that **Face v1 — eye puppetry** and **Face v2 — doodle
puppetry** are Feather projects. Both are restored to the main gallery, Feather
index and project directory, and removed from the archive. The initial archive
classification incorrectly treated their earlier XIAO build records as the
classification of the entire projects. Dated build records and saved releases
remain unchanged; they are not relabeled as newly verified Feather builds.

## Scope and sources

- OpenToy branches: `feature/orange-face` (44ecd87), `feature/nearby-buddy`
  (abc451e), `feature/nesso-n1` (7196c33), `feature/gravity-water` (483bf43),
  `feature/gravity-water-feather` (04a95d5). These show the November 11–15
  Nearby Buddy, Orange Face, Nesso and Gravity Water work.
- [Standalone Nearby Buddy](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/nearby-buddy/main): its main
  README confirms Reverse TFT Feather #5691 and includes enclosure files absent
  from the older OpenToy chapter. Both locations remain explicitly labeled.
- [Battery Faces gamepad branch](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/seesaw-gamepad)
  (2305f0e) and [haptics branch](https://github.com/yury-g/OpenToy-Coursework/tree/main/companions/battery-faces/feature/qwiic-accessories)
  (f83f2f9): Blue/Green use Reverse TFT Feathers. The base and accessory builds
  are distinct checkpoints; gamepad work targets Blue. Battery Faces supports
  other boards too and remains in its existing repository.
- Earlier OpenToy source and the October checkpoint establish the XIAO and
  mechanical archive. Archive status is a browse classification, not deletion.

This review does not assert every firmware ever uploaded locally is published.
It indexes the confirmed repositories and branches, and does not infer current
installed firmware from older handoff records.

## Folder layout

- `projects/feather/`: visual Feather index.
- `projects/nesso/`: Nesso index.
- `projects/README.md`: exact source/repository/branch directory.
- `archive/`: XIAO, historical controller setup and pre-Feather experiments.
- Existing implementation paths stay intact for builds, saved downloads and
  shared renderer imports. No device was reflashed during this reorganization.

## Preview provenance

The previews under `docs/images/projects/` are copied without modification:

| Preview | Source | Meaning |
| --- | --- | --- |
| `gravity-water.png` | OpenToy 04a95d5, `projects/gravity-water/images/cross-platform-poster.png` | Scripted canonical particle model |
| `orange-face.gif` | OpenToy 44ecd87, `projects/orange-face/docs/images/orange-face-tour.gif` | Scripted face renderer |
| `nearby-buddy.png` | OpenToy abc451e, `projects/nearby-buddy/docs/images/nearby-buddy-devices.png` | Illustrated device render |
| `battery-faces.png` | Battery Faces 2305f0e, `assets/screenshots/qwiic-feather-screens-v25.png` | Recorded v25 Feather accessory screens; not a current live feed |

Original licenses and attribution remain in the linked source projects.

## 3D printed designs · November 16, 2026

The owner requested that printable designs be filed with the projects and be
navigable through existing visuals, without taking over the front page.

- Canonical index: [projects/3d-printed-designs](../projects/3d-printed-designs/README.md).
- First entry: [Orange Face one-piece faceplate](../projects/3d-printed-designs/orange-face/README.md), with print files, editable CAD, Blender scenes, assembly/removal views and validation records.
- The home README adds one small navigation link and a print link on the existing Orange Face card. Its gallery and project order remain intact. The project directory has a visual chooser for Feather, Nesso and printed designs.
- All five faceplate images are reused unchanged from the completed design package. No new visuals were generated for this organization work. They are CAD/Blender renders, not physical-test evidence.
- Firmware paths, named feature branches, existing releases and the mechanical archive remain in their original locations. The faceplate is an untested first-fit print; its recorded CAD and slicing checks are kept distinct from physical validation.

## Updating the home gallery

Add one row to [home-gallery.tsv](home-gallery.tsv) per project or interaction. Give it a title, a guide, a repository-relative image, descriptive alt text and a short caption. Identify simulations, device captures and CAD views in the caption. Rows appear in file order, three across. Run `python3 tools/render_home_gallery.py` from the checkout and commit the refreshed README with the catalog. Use `--check` to check for stale cards.

Edit device capabilities, app support, controls and parts rows directly in the root README. Reuse the thumbnail and hardware-guide link from the matching device row. Add a new board's manufacturer references in the hardware gallery first. Keep installed versions and outstanding physical checks tied to the dated device checkpoint.

The Copy-Face previews come from the [prototype renderer gallery at 7cf4f65](https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/copy-face/images). The Nesso Face loop renders scripted idle, left tickle and startled poses with the [Nesso firmware’s drawing code at 7196c33](https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/nesso-n1/firmware/NessoFace). It shows the face area in the default orange shade. The controls table uses illustrated action symbols. The related hardware rows use manufacturer views and a connector identification diagram for the dual-port CYD. Accessory photographs are hosted by the manufacturers.


## Visual storytelling for new projects

Lead a project guide with its best real render, animation or screen capture and a short explanation of what the toy does. Show the interaction, then the hardware and assembly, then the source and build instructions. Keep earlier design views reachable from a visual history section. Label simulated motion and mark the current physical test status beside the relevant image.

Every project belongs in the home gallery and the source map. Hardware assemblies such as ditherCam and the CYD tri-fold also belong in the printed-design index. Keep their project names consistent across those entry points. Store the editable source and original visual assets with the project so a later revision can update the story.
