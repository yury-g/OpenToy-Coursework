# CrowPanel revision log

## 2026-11-17 — Eyes 1 0.3.0 source/build candidate; not deployed

- Replaces the footer with common Menu pages and full performance mode. A fresh
  top-center 0.8-second hold restores information and Menu. Help teaches restore;
  press feedback and selected-personality text accompany labeled controls.
- Portable navigation/gesture model tests pass for release-inside, drag-out,
  multi-contact cancellation, independent eye play, freshness/failure and wrap.
  The full CrowPanel/reused Eyes host suite and P4 build pass (614,096 bytes).
- Fresh review found no blocking issue. Raw contact-report semantics and a
  deferred-layout adapter regression test remain minor follow-ups; see [candidate
  checklist](MENU.md). No new live control or performance result is claimed.
- Copy-Face retains hardware ownership. No serial connection, reset, camera
  access or flash write occurred. Preserve installed 0.2.2 recovery; deployment
  awaits explicit handback and separate live acceptance. Battery stays BAT--;
  audio remains roadmap. Root/shared UI files were not changed.

## 2026-11-17 — Eyes 1 0.2.2 installed

- Native P4 Eyes preserves existing personalities, independent reactions and
  blinking, with crisp analytic curves and antialiasing. Added a labeled actual
  renderer preview, fixed landscape touch mapping, visible ECO and session info
  controls. XIAO is not used; no BOOT/RESET app controls.
- Canonical metadata, pinned IDF5.4.2 dependencies, private build staging and
  generated PSRAM/configuration checks make the build reproducible. Only the
  application slot was written, with complete readback and unchanged outside data.
- Host tests and final live bench pass. Normal-mode samples measure 12.6–15.1 fps
  across all three personalities and both eye sizes; no 30 fps claim. Controls,
  synthetic reactions/recovery, stale-input handling and serial load pass.
- Owner reported the smooth eyes look great while 0.2.0 ran. Detailed physical
  multi-touch, power-cycle and battery-runtime tests remain open.
- Exact V1.0 battery wiring is documented. STC8 supplies coherent candidate bytes;
  exact protocol/calibration remain unqualified. Final 0.2.2 shows neutral BAT--,
  candidate voltage only in diagnostics, and unknown charging. No charger changes.
- Fresh reviews resolved touch, layout, timing and battery-label issues. See
  [verification](EYES1-VERIFICATION.md) and [battery evidence](BATTERY.md).

Next: transfer hardware ownership to the separate Copy-Face project. Camera,
audio and family-menu migration remain distinct future qualifications. Backups,
compiled images, device identities and raw records remain private.

## 2026-11-17 — track established; no firmware release

- Created a separate OpenToy checkout/branch from freshly fetched main;
  retained the active Feather/Nesso gallery and separate Waveshare ownership.
- Owner confirmed DHE04209D V1.0. Compared all three vendor schematic revisions,
  recorded V1.0 power/programming connections and separated XIAO/C6/P4 roles.
- Directed initial USB connections with the XIAO removed from the panel.
- Verified physical Mac operation and XIAO S3 ROM identity/security/flash size.
  Completed two verified 8 MiB XIAO backups and decoded its selected app.
  Independently identified P4 ROM revision 1.3, 16 MiB flash and security flags zero.
- Installed the official signed WCH driver; owner enabled it in macOS Settings,
  which resolved the missing serial connection. After large streamed reads
  failed, checksummed 4 KiB reads with explicit flash-size setup and bounded
  retries completed two matching 16 MiB panel backups. Each matched the device
  digest; the live partition table and original factory app were verified.
  Neither board's flash was written. Backup restoration remains untested.
- Built the unchanged V1.0 manufacturer screen example with ESP-IDF v5.4.2;
  verified its application image checksum and revision header. Its sample
  partition layout differs from the live panel, so no upload was performed.
- Inventoried active projects, hardware gaps, native/companion roles and test
  gates; adopted the shared app-status-bar requirement and device-family strategy
  for future ports, with separate build/install/live/owner-observation evidence.

**Evidence classification:** documentation/source inspection and read-only USB
bring-up. Manufacturer example build only; no OpenToy installation, display/touch/audio test,
combined-power qualification or battery-runtime measurement yet.

**Historical next action (superseded above):** build the first native Eyes 1 port against the verified live
layout, then qualify display and touch. Hardware and recovery details remain
outside Git.
