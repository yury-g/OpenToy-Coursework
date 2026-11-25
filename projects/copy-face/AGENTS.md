# Working on Copy-Face

This is a separate app, not a new name for Animatronic Eyes. First target:
Elecrow DHE04209D V1.0 / ESP32-P4. Read README.md, docs/DESIGN.md,
docs/CAMERA-RESEARCH.md, VERIFICATION.md and ../../docs/APP-STATUS-BAR.md.

The first prototype was synthetic-only; 0.2 integrates a live camera adapter.
Never describe scripted mouth or gaze as camera inference. Do not enable real
mouth copying from five-point detections. Use source-linked sensor/model evidence
and current VERIFICATION.md for physical results. Keep simulated lip contact and
real face landmarks explicitly distinct.

The CrowPanel device chat owns hardware until explicit private written release.
No serial open, reset, scan or flash before that handoff. Keep recovery, enrolled
identity, raw logs and binaries outside Git. Use only the enrolled panel after
release and only app-slot updates preserving all other flash. XIAO stays separate.

Keep capture/inference separate from presentation and retain local-only behavior.
Do not add recording, uploads, identity recognition or inferred emotions.
Use the family roadmap for later audio and measured battery follow-up. A battery
being attached does not establish a gauge; use `--` until measured information is
qualified. Preserve concurrent CrowPanel work when refreshing source snapshots.
