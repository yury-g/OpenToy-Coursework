# CrowPanel revision log

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

**Next action:** build the first native Eyes 1 port against the verified live
layout, then qualify display and touch. Hardware and recovery details remain
outside Git.
