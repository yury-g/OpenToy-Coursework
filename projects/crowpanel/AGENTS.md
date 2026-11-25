# Working on the CrowPanel track

This is the DHE04209D 9-inch **V1.0** device track within OpenToy. Read README.md,
HARDWARE.md, BUILDING.md, COMPATIBILITY.md, CHANGELOG.md and the shared
docs/APP-STATUS-BAR.md and docs/DEVICE-FAMILY-ARCHITECTURE.md specifications.
Use a separate checkout from the Waveshare track; fetch main before integration.

The initial setup uses separate USB cables with the XIAO physically removed.
Do not reconnect it to the panel or power an inserted XIAO through USB without
qualifying the exact revision, connector orientation and power isolation.

P4 drives the display. Factory onboard C6 is separate from external XIAO S3.
Identify each intended processor through its own ROM before flash operations;
never probe all serial ports. Do not access other device tracks' hardware.
Keep expected and excluded identities in private local records outside Git.
Never infer authorization to use another device from its old project records.

Preserve two matching fresh full-flash reads and hardware digests where supported,
decode the live partition/OTA layout, and use only compatible bounded application
writes. No routine erase, partition replacement, C6 update or whole-device upload.
Keep recovery data, secrets, binaries, identities and raw logs outside Git.

Update the per-project matrix and dated revision log after meaningful results.
Separate source inspection, host tests, builds, live telemetry, synthetic events
and physical observations. The checklist is not a claim of supported firmware.

## Native Eyes firmware

Read EYES1.md, EYES1-VERIFICATION.md and BATTERY.md before changing the port.
Use tools/build.py with a private work directory and the pinned IDF5.4.2 lock;
never run the stock whole-device flash command. Canonical metadata is in
firmware/metadata.json. Run tools/check.py; native geometry tests cover both
viewport sizes. Shared Eyes behavior remains unchanged.

Only touchscreen controls are app inputs. BOOT/RESET remain recovery controls.
Battery candidate bytes are diagnostics, not qualified voltage/percentage or
charging evidence. Keep neutral BAT-- until exact-board qualification.
Check the private Copy-Face hardware handoff before opening serial; once released,
coordinate ownership instead of reopening it. Camera presence remains unverified.
