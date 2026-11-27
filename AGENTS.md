# Working on OpenToy

## Canonical repository — November 25, 2026

The owner approved this repository for public access and ongoing work: `https://github.com/yury-g/OpenToy-Coursework`. Use its current `main` as the base for new task branches/worktrees and publish changes here. The old private `yury-g/OpenToy` is historical reference; do not maintain matching changes in both repositories. Follow [CONTRIBUTING.md](CONTRIBUTING.md#one-active-repository) for imports, review and website updates. Imported device/feature branches are snapshots, not automatic active development destinations. Never merge the old private history into this clean repository or repoint an old worktree and push. Review individual unfinished changes before porting them. GitHub READMEs are the project home. Use GitHub Pages only for interactive HTML viewers; do not create a separate courseware homepage. See `docs/INTERACTIVE-VIEWERS.md`. Existing hardware stops remain in force.

Work on the student's physical machine. Verify native OS, USB serial access and chip identity before claiming hardware access. Full permissions in a cloud container are not local USB/Bluetooth access.

Identify ESP32-S3 and the intended factory MAC before flashing. Preserve existing source. Back up original flash before first replacement. Do not issue erase-flash routinely. Keep private profiles, keys, backups, compiled images and logs outside Git; compiled images embed a private key.

Prioritize bounded live behavior changes. Keep controller input/mapping separate from BLE delivery and rendering. Preserve device identity, update protocol/partition compatibility and deferred boot validation when changing animations. Run host tests and applicable real hardware tests. Report synthetic-input tests separately from physical controller tests, and USB/I2C state separately from observed OLED/touch behavior.

Do not claim iPad operation, direct controller connection, fresh student deployment or a portable installer until each has been tested. Keep third-party licenses and source references intact. Publishing destination must be the owner's verified account/domain; Teaching and Studio identities can differ.

## Project browsing and documentation

Use the hardware/project directory in `projects/README.md`. Feather projects come first in current browsing, with Nesso separate. XIAO and pre-Feather projects are archived through `archive/README.md`; preserve their source paths, releases and shared-code dependencies. Follow the selected project's exact board and setup guide, not historical XIAO instructions by default. Keep external repositories and feature branches explicitly labeled. Older checkpoint notes do not establish what is installed on a reused board today.

Use direct descriptions and control labels in documentation and artwork. Reuse the existing README text and the owner’s words. Keep new copy short; do not invent taglines or promotional phrasing. The owner’s current description is: “Open courseware under active development. Projected launch out of draft: January 10, 2027.” Do not add marketing slogans or invent a voice for the owner.

Face v1 (eye puppetry) and Face v2 (doodle puppetry) belong in the active Feather project list, per the owner’s November 15 correction. Do not archive the whole projects based on older XIAO build notes; retain those dated records as history.

The owner named the OpenToy projects **Animatronic Eyes 1 and 2** on November 15, 2026: https://github.com/yury-g/OpenToy-Coursework/tree/main/projects/animatronic-eyes (within OpenToy). Keep both active in the Feather gallery. Use that project folder for current direct QT gamepad builds; retain the historical Face v1/v2 guides and release tags.

3D printed designs belong under `projects/3d-printed-designs/`, with a visual index and each design's own README, print files, CAD, existing renders and assembly/removal guide. Keep the home-page entry compact and link the design from its related hardware project. Reuse existing visuals for documentation organization; distinguish CAD/slicer validation from physical fit tests.

## Shared app status bar

Use the root README as the family device/app/control overview and `docs/CONTROLS.md` for shared interaction roles. Ordinary app controls must use qualified exposed user buttons or touch, never BOOT/RESET/EN. Keep current installed mappings distinct from the target standard until a versioned migration is tested. Update matrices and source links without duplicating changing version/state facts across galleries; preserve existing app guides and archived releases.

Follow `docs/APP-STATUS-BAR.md` for all maintained app changes and new device ports. Render apps across the full native canvas edge to edge by default, and draw the running app, installed version, creation date, and genuine local battery reading overlaid **after** the app using a compact contrast shadow/halo rather than an opaque full-width reserved bar or tank borders. Use two readable non-overlapping rows on narrow displays, keep physics/projection/touch geometry fixed in screen coordinates regardless of header visibility, and consume taps only inside actual visible control overlays. Keep a reversible, documented session-only performance mode that only hides overlays without resetting physics or layout. Adapt text to safe screen bounds, including round displays; do not force an unreadable universal 10-pixel layout. Keep canonical metadata and version history consistent, preserve old release tags, and track unmigrated existing apps as migration pending rather than claiming every existing installation already conforms.
