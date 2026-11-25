# Working on Animatronic Eyes

Use the owner's physical Mac and verify native USB access. Never identify a device by serial pathname alone. Match its factory identity through ESP32-S3 ROM before flashing, inspect security/flash size/partition layout and selected application slot, and preserve a fresh full-flash backup verified against a hardware digest. Keep all raw identities, logs, backups and compiled images outside Git. Do not access retired/damaged boards from older projects.

The two installed Reverse TFT Feathers have different application capacities. Preserve each board's TinyUF2, partition table, OTA selection, NVS and filesystems. Write only a validated application bounded to its selected original slot; verify the written image and unchanged flash outside the sector-aligned write range. Never use routine whole-flash erase or stock Arduino Upload.

Preserve the original OpenToy eye and doodle renderers in shared/opentoy. Keep mapping separate from hardware and rendering. Run tools/check.py and build both versions. Test USB backpressure and stale/disconnected input on the real boards; distinguish telemetry/synthetic checks from owner observations. See README.md and docs/VERIFICATION.md.

This folder inside OpenToy is the current source for Animatronic Eyes 1 and 2. The owner clarified that the names belong in OpenToy’s project list, not in a separate repository. Keep the OpenToy gallery links and original release records; do not archive those projects or overwrite old tags. No private credentials, radio keys, compiled firmware or device backups belong here.
