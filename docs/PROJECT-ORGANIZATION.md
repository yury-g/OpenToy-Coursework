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
