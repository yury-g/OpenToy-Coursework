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
