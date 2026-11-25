# Pink Face and camera family

Requested outcome: enroll M5Stack StickS3 as Pink; then enroll Seeed XIAO ESP32S3 Sense as Teal camera. Existing faces continue reporting battery measurements and Tab5 tiles fresh enrolled devices. Green stays retired and disconnected for board heat.

Pink uses its native 135×240 portrait panel and M5PM1 voltage readback, with approximate voltage-derived SOC explicitly labeled. Preserve charge-control configuration. Both user buttons remain useful: next source and next page.

Camera sends 80×60 RGB332 stills every two seconds over channel-1 ESP-NOW broadcast, in bounded CRC-checked fragments under 250 bytes for classic ESP32 compatibility. Keep latest complete frame only in RAM; do not record audio or SD. Battery packets remain separate. Blue serves complete CRC-checked camera snapshots over TCP4244 to Tab5 (after the hosted radio dropped too many relayed UDP fragments), using its existing private AP. Camera has no native battery measurement; its family tile states camera/no gauge.

Each display has an explicit camera view, available only with fresh complete frames. Touch taps cycle face, trend, camera; horizontal swipes leave camera and cycle faces. Blue D1/Pink B cycle pages. Tab5 tap toggles feed/grid and swipes return grid. Camera loss after five seconds returns to faces automatically. Retain equal image scaling and letterboxing.

Success requires verified original backups, app-only installs within actual original partition capacities, fresh Pink telemetry in peers and Tab5, real camera frame delivery and captures, malformed/partial/expired frame tests, camera-loss fallback, battery telemetry continuing during camera traffic, and private GitHub code/gallery publication. Disconnected nodes are reported explicitly, never claimed tested. No charging, runtime, discharge or health certification tests.
