# Copy-Face verification

Date: 2026-11-17. Source/build: **0.2.1-proto**. Installed: **0.2.0-proto**. Target: **DHE04209D V1.0 P4**.

| Evidence | Result | What it does not establish |
| --- | --- | --- |
| Tracking checks | 16 host scenarios pass, including selection, confidence, loss/reacquisition, stale/reordered/duplicate frames, mirror, clock wrap and lip retention | Detector accuracy or physical latency |
| Lip/eye geometry checks | Contact/separation, smile/frown/neutral/open, roll/aspect correction and invalid landmarks pass | Real lip or eyelid inference |
| Preview checks | Pixel copy/bounds, freshness, image/overlay timestamp match and mirror pass | Camera optical quality |
| Scene checks | Header/developer bounds, lip edges, open-mouth gap, buffer guards and gaze pass | Physical screen appearance |
| Menu checks | Release activation, drag/multi-contact/stale cancellation, menu paths and restore hold pass | Owner physical touch acceptance |
| Reused behavior suite | Full Animatronic Eyes host suite passes, including unchanged renderer/telemetry digests | Hardware behavior on this target |
| P4 build | IDF 5.4.2, revision 0.1–1.99, 16 MiB, 200 MHz PSRAM, RGB565; camera/model image 3,277,664 bytes fits preserved 11 MiB slot | Installed or running firmware |
| Preview artifacts | Synthetic smile/frown/neutral/open lip contours with actual C++ rendering | Camera footage, physical contact or target frame rate |
| Real mouth / eyelids | No qualified contour model; live mouth neutral and labels unavailable | No expression-copying or eye-activity accuracy claim |
| Battery | Sensing remains unqualified; displayed `--` | No charge, voltage or runtime claim |

The camera adapter pins esp_video 2.0.0, human_face_detect 0.5.2, sensor 2.0.1,
esp_ipa 1.3.1 and esp-dl 3.3.15, retaining display pins. Full resolution,
revision-1 tuning provenance, RGB565 byte order and timeout evidence are in
[lock notes](locks/camera-notes.md). The model is linked into the application;
no partition-layout change is required.

Build image SHA-256:
`3677625a4da6362e89439ac79b8d97c4046e9ac31cfd4cc3095e8dc497073ceb`.
Source digest is recorded privately with the build. Generated firmware, identity,
recovery, camera stills and raw serial logs are excluded from Git.

## Physical checkpoint

Copy-Face **0.2.0-proto** was installed on the enrolled physical P4 after the
explicit private release. ROM identity/security/flash identity, prior full-flash
digest and live partition bytes matched. Only application sectors were written;
independent 4KiB reads matched the complete padded app, and prefix/suffix hardware
digests confirmed every other flash byte unchanged. Prior Eyes0.2.2 and original
recovery remain preserved. Installed image3,276,976bytes, SHA-256
`52c9894514b9ffbfa51e479fdf0a9892de082552504a2040415f46ee542d7695`.

Live status showed display/touch initialization ready and advancing display
frames (about8 per second during four short status samples; this is not a
sustained performance result). Camera returned error259 before sensor identity;
zero captured frames and no detection. No private camera image was obtained.
**The owner then confirmed no camera is physically connected to the panel.**
This explains the missing camera source; it does not validate the camera adapter.
No further hardware camera probing is needed until a source is fitted.

The diagnostic0.2.1 build adds startup-stage reporting and visible failure text,
but is **not installed**. The panel is left in explicitly labeled Simulation
for the mouth demonstration; reboot returns0.2.0 to its default live/neutral view.
The separate XIAO Sense has not been opened, flashed or connected.

## Remaining acceptance

- Real camera identity/capture/detection and sustained running.
- Physical left/right/up/down orientation, face loss and multi-face selection.
- Measured rendering cadence alongside capture/inference. Age is an estimate;
  the driver does not return a hardware exposure timestamp.
- Owner visual and physical touch acceptance, Performance restore and cold boot.
- Validated dense lip model and calibrated contact/shape fidelity.

Automatic recording and uploads are absent. An explicit UART `C` diagnostic can
export one RAM frame; any captured evidence must remain private.
