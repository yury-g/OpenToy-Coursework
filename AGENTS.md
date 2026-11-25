# Working on OpenToy

Work on the student's physical machine. Verify native OS, USB serial access and chip identity before claiming hardware access. Full permissions in a cloud container are not local USB/Bluetooth access.

Identify ESP32-S3 and the intended factory MAC before flashing. Preserve existing source. Back up original flash before first replacement. Do not issue erase-flash routinely. Keep private profiles, keys, backups, compiled images and logs outside Git; compiled images embed a private key.

Prioritize bounded live behavior changes. Keep controller input/mapping separate from BLE delivery and rendering. Preserve device identity, update protocol/partition compatibility and deferred boot validation when changing animations. Run host tests and applicable real hardware tests. Report synthetic-input tests separately from physical controller tests, and USB/I2C state separately from observed OLED/touch behavior.

Do not claim iPad operation, direct controller connection, fresh student deployment or a portable installer until each has been tested. Keep third-party licenses and source references intact. Publishing destination must be the owner's verified account/domain; Teaching and Studio identities can differ.

## Project browsing and documentation

Use the hardware/project directory in `projects/README.md`. Feather projects come first in current browsing, with Nesso separate. XIAO and pre-Feather projects are archived through `archive/README.md`; preserve their source paths, releases and shared-code dependencies. Follow the selected project's exact board and setup guide, not historical XIAO instructions by default. Keep external repositories and feature branches explicitly labeled. Older checkpoint notes do not establish what is installed on a reused board today.

Use direct descriptions and control labels in documentation and artwork. Do not add marketing slogans or invent a voice for the owner.

Face v1 (eye puppetry) and Face v2 (doodle puppetry) belong in the active Feather project list, per the owner’s November 15 correction. Do not archive the whole projects based on older XIAO build notes; retain those dated records as history.
