# Working on OpenToy

Work on the student's physical machine. Verify native OS, USB serial access and chip identity before claiming hardware access. Full permissions in a cloud container are not local USB/Bluetooth access.

Identify ESP32-S3 and the intended factory MAC before flashing. Preserve existing source. Back up original flash before first replacement. Do not issue erase-flash routinely. Keep private profiles, keys, backups, compiled images and logs outside Git; compiled images embed a private key.

Prioritize bounded live behavior changes. Keep controller input/mapping separate from BLE delivery and rendering. Preserve device identity, update protocol/partition compatibility and deferred boot validation when changing animations. Run host tests and applicable real hardware tests. Report synthetic-input tests separately from physical controller tests, and USB/I2C state separately from observed OLED/touch behavior.

Do not claim iPad operation, direct controller connection, fresh student deployment or a portable installer until each has been tested. Keep third-party licenses and source references intact. Publishing destination must be the owner's verified account/domain; Teaching and Studio identities can differ.
