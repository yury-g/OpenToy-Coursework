# Working on the RLCD target

Read root AGENTS.md, docs/DEVICE-FAMILY-ARCHITECTURE.md and docs/APP-STATUS-BAR.md. This folder owns only the Waveshare ESP32-S3-RLCD-4.2; no other device chat hardware may be opened. Match fresh native USB enumeration, physical board evidence and ROM identity before access or writes. Exact private identities and recovery artifacts are outside Git.

Preserve two matching full-flash originals with a device digest. Inspect live partitions and OTA selection; never upload the build-generated partition table. Bound writes to the verified original selected app slot. Verify app, tail padding and all outside bytes; distinguish later runtime NVS/coredump changes from installer changes. Keep keys, compiled images, backups and raw logs private.

Application entry points belong in Rlcd.cpp. Arduino prototype rewriting caused a documented recursion failure when they lived in the .ino. Reuse canonical Eyes behavior and renderer via build staging; do not fork their logic or modify another target. PanelInit.h retains exact pinned vendor values/license. Never infer touch, IMU, SOC or charging from the chip family. Maintain explicit app-by-app evidence and source branches.

Run tools/check.py and tools/build.py with documented pinned dependencies. Host tests and transfers do not prove visible pixels or physical buttons. Keep owner observations separate. Publish a source/build checkpoint only after current main is fetched, preserving concurrent device work.
