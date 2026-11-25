# Face v2 implementation record

October 1, 2026. Owner chose playful doodles, requested movement/curvature simulation, then explicitly authorized the agent's best version to be uploaded while away. This supersedes the earlier wait-for-review gate. Execute locally, preserve v1, and leave actual appearance judgment for the owner.

Design: sparse two-pixel curved strokes on a 128×64 SH1106G, with happy, surprised, angry, sorry and nervous presets plus a neutral stop pose. Left stick Y bends eyes, left X bends brows, right Y bends the mouth, right X pulls it sideways. Right trigger opens the mouth; left trigger closes both eyes; D-pad left/right winks. A happy, X surprised, B angry, Y sorry, right shoulder nervous, left shoulder neutral. Preserve 15% stick dead zone, latest-pose coalescing and acknowledged touch-off prerequisite.

Add atomic `doodle EYE BROW MOUTH SKEW JAW LEFT RIGHT EXPRESSION`; four axes -100…100, jaw/lids 0…100. Preserve existing `face` and `eyes` commands/renderers, device identity, flash partitions, private key, uploader and boot validation. Negotiate `doodle=1` in the app. Draw curve changes with a short damped spring and feature-specific wiggle; stop animation once settled, with no idle jitter. Disconnect/Stop resets to a centered neutral doodle. USB reports both target fields and whether the frame has settled.

Execution and verification:

1. Preserve v1 source/app/image and confirm the physical chip/MAC again.
2. Write failing host mapping/parser/render/animation checks; implement the pure mapping and renderer.
3. Inspect static device-resolution renderings, test bounds, animation convergence and legacy regressions.
4. Compile; review the change; authenticate/upload over Bluetooth to the identified OLED toy.
5. Verify running build, all five presets, each axis, mouth/blinks/winks, settled reference frame hashes, animation progress, invalid-command atomicity and BLE-disconnect neutralization over independent USB.
6. Run applicable legacy/peripheral/update checks. Restart the native app, connect and enable controller input; corroborate the real neutral controller pose. Record synthetic tests separately from later human use.
7. Save/publish source and teaching/recovery notes; preserve `v1.0.0-eyes` and its private return image.
