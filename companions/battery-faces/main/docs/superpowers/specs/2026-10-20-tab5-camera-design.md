# Two camera feeds

User requests Tab5's onboard camera alongside the XIAO Sense feed, selectable by buttons/swipes. Continue only USB-connected Blue, Rainbow, Purple, Pink, Teal and Tab5; Orange/Yellow remain installation-deferred by owner and Green stays heat-retired.

Preserve the working v16 battery network and original app partitions/charger settings. Tab5 MIPI-CSI sensor is identified through its read-only SCCB ID, then initialized from the manufacturer's pinned SC202CS/SC2356 tables. Display-owned LDO3 is reused without reconfiguration; no charger bits or full M5Unified Power initialization. Driver captures fresh RGB565 and downsamples with a16:9 letterbox into the80×60 RGB332 network frame.

Camera sources have stable IDs0=Teal XIAO,1=Tab5. Each has its own CRC/sequence/boot-session/freshness state. Teal retains direct ESP-NOW. Tab5 uploads complete snapshots over the existing private TCP link to Blue, which rebroadcasts only the Tab5 source to small displays. Receivers validate the permitted source-MAC/source-ID pairing. Tab5 previews its own camera locally and continues receiving Teal through Blue. No internet, SD or microphone recording.

Tap/D1/B cycles face→trend→available camera feeds→face. Touch swipes provide camera selection and return to faces, preserving existing face navigation. A lost selected feed returns to faces; no stale frame is relabeled live. Both camera sources can be disabled/re-enabled independently for verification. Original charge indicators and battery measurements continue during camera traffic.

Verify pure source isolation, framing, malformed input, replay/expiry, selection/fallback, aspect ratio and socket backpressure; build every profile; read actual Tab5 sensor ID and capture real images; verify both feeds on all connected displays and independent loss/recovery. Preserve screenshots, source/license attribution and all image archives; update the private README after real hardware proof.


## Accepted steering during implementation

Owner prioritizes XIAO as the default camera on all connected displays; a short action returns to that device’s battery face. Camera pages supplement the battery faces/trends and never remove them. With no cameras, the battery application remains active. XIAO defaults to160×120 four-shade grayscale, toggled back and forth with80×60color by holding the information button or stationary touch and releasing. The broadcast quality mode is shared; both payloads are4800bytes. Live Tab5 camera remains an additional page and has preliminary fixed color balance.
