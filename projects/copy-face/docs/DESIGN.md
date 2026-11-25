# Copy-Face design — 2026-11-17

Copy-Face is a separate OpenToy app (`copy-face`), initially for the Elecrow
DHE04209D V1.0. Eyes follow a stable primary face in the camera frame; a mouth
copies visible geometry when a suitable model is verified. No identification,
emotion inference, camera recording, uploads, radio or XIAO dependency.

The owner authorized real camera capture and an application-only installation.
The private hardware handoff released this panel on 2026-11-17; preserve exclusive
ownership and recovery when coordinating further tests.

## Approach and boundaries

Use native P4 processing with independent capture/inference and presentation.
Keep only the latest bounded observation; no queued stale frames. Render elapsed
time smoothing independently of inference cadence. Associate faces geometrically,
retain the primary through brief misses, and require consistent observations
before acquiring another. These are short-lived spatial tracks, not identities.

The alternative of host/cloud inference conflicts with the local-first starting
point and is not implemented. A dense mouth model on P4 remains a feasibility
task. Five-point face detection alone cannot establish lip opening or curvature.
Version 0.2 therefore integrates real face capture/detection while keeping live
lip capability absent. Explicit simulation exercises lip and eyelid contours with
a persistent simulation label. Build, installation and camera observation are
separate evidence in VERIFICATION.md.

Reuse the CrowPanel native signed-distance eye renderer and large pupils through
a source-tracked snapshot until its device-owned implementation is published.
Compose two eyes above a separately rendered mouth; do not enlarge pixel art.
The 1024×600 screen reserves a readable 48-pixel header and a 56-pixel controls
row. Header displays COPY-FACE, canonical version/date, Menu and local battery
`--`. Display → Performance hides UI; a top-center 0.8-second hold restores Menu.
This is session-only and resets visible on boot. Synthetic state remains visible.

## Observation contract

At most eight normalized face boxes per result, capture timestamp and confidence.
Reject non-finite, inverted, out-of-frame boxes and stale/reordered results.
Acquire at confidence 0.70, retain at 0.50; three consistent results to acquire;
450 ms retention; results older than 250 ms cannot revive a face. Match by
overlap or bounded center displacement with size consistency. Render smoothing
uses elapsed time with a 90 ms gaze time constant and a 120 ms mouth constant.
Loss smoothly returns to centered gaze and neutral closed mouth.

Mouth observations include nine paired upper/lower lip landmarks with image
aspect and validity. Normalize roll/scale, preserve asymmetry and draw a single
line only for adjacent samples within a 1.5%-mouth-width contact tolerance.
Separated sections have two edges. Legacy scalar aperture/bend supports fixtures.
Only a validated lip-landmark/model adapter may set them for real input.
Five-keypoint detections leave the capability absent. Real mounting orientation
and horizontal mirror must be checked with physical left/right/up/down movement;
the prototype transform is configurable and host tested, not bench calibrated.

## Acceptance and limitations

Host tests cover retention, reacquisition, confidence, invalid/reordered/stale
input, clock wrap, mirroring, mouth capability and frame-rate independence.
Build and synthetic native-renderer previews are separate evidence from real
camera, display, touch and expression fidelity. Proposed live target is fluid
presentation at 30 fps with vision at its measured sustainable rate; this is not
a performance result. Real image-to-display latency, jitter, dropped detections,
camera/display contention, power and mouth accuracy remain unmeasured.

When hardware is released, preserve factory recovery and latest Eyes build,
verify sensor identity without broad probing, and install only a compatible
app-slot image preserving all other flash. If missing, request one physical
camera check. Never use V1.2 wiring or the vendor whole-device flash command.

## Camera implementation

See [pinned adapter evidence](../locks/camera-notes.md). SC2336 identity is checked
before driver initialization. Two V4L2 buffers feed a latest-frame mailbox;
separate inference publishes the corresponding 320×180 center crop and points.
Finite dequeue timeout is read back. Three consecutive capture failures stop the
worker. UI snapshot copying is nonblocking. Capture age is estimated; the pinned
driver does not supply an exposure timestamp. The lip model remains absent.
