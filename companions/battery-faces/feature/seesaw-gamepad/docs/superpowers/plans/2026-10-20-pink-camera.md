# Pink and Camera Implementation Plan

Goal: extend the existing family with Pink StickS3 and a Teal XIAO camera feed.
Architecture: preserve v3 battery packets and stable source indices; append Pink6 and Teal7. Add independently bounded still-frame transport, display navigation and Blue UDP bridge.
Spec: ../specs/2026-10-20-pink-camera-design.md

- [x] Back up both new boards, verify hardware digest, inspect partitions and active slot.
- [x] Test registry/navigation changes, implement Pink hardware adapter, build and install available devices.
- [x] Test camera assembler adversarial inputs, implement camera sender independently.
- [x] Integrate camera UI and bridge, test complete frames and five-second fallback without stopping battery telemetry.
- [x] Build all profiles; verify connected devices, capture actual screens, document unavailable physical tests.
- [x] Update family illustrations, README and archives, review, push private repository and verify remote README.

Constraints: no Green access; no charger reconfiguration; no battery load tests; never open a serial port during backup/upload; original tables, bootloaders and NVS preserved. Review malformed lengths, dropped fragments, resets, duplicate packets, inactive camera and low-memory classic ESP32.

Owner elected to continue with USB-connected devices. Orange/Yellow installation remains deferred; matching profiles build successfully. Pink rotation2 is owner-confirmed upright. Camera relay to Tab5 uses bounded TCP4244 after actual UDP packet loss; small displays retain ESP-NOW.
