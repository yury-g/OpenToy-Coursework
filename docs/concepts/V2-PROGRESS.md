# Face v2 progress

October 1, 2026. Source event: `opentoy-face-v2-20261001`. This log records results and the user's changing instructions, not an untested completion claim.

- Owner chose **playful doodles**, five emotions, and requested continuous curvature and a movement-following wiggle.
- Interactive 128×64 simulation drawn and visually inspected: happy, angry, nervous and combined curvature extremes. No browser warnings/errors were reported in that inspection.
- Owner then authorized **the agent's best version to be uploaded while away**, with human device testing afterward. The earlier wait-for-visual-review instruction is superseded for this iteration.
- Owner additionally requested algorithmic art versus preset morphing. The intended device path generates line/curve geometry from parameters; the alternative will be compared in the preview. Neither technique requires bitmap crossfades.
- Exact v1 source, Mac adapter, tested EyeLayers application and manifest copied to the private `projects/xiao-class/backup/before-face-v2/` folder. Public `v1.0.0-eyes` remains preserved.
- Physical native Mac USB access rechecked; ROM identified the intended ESP32-S3 factory MAC before any update.
- Six new mapping tests failed first because doodle mapping was absent. The new protocol test failed because the old firmware rejected `doodle`; implementation in progress.

Initial pending work (resolved below where results are recorded): render/animation tests, build, code review, authenticated BLE upload, independent USB checks and native-app handoff. Owner's actual v2 display/controller judgment remains pending.

## Procedural iteration and comparison

- All 45 Python host tests and 11 C++ programs pass. Six mapping tests exercise the new controls. Renderer tests cover six distinct moods, mouth isolation, guarded buffers, animation settling, clock wrap, hidden transitions and exact final presentation.
- Generated 19 settled pixel/hash fixtures from the pure firmware renderer; visually inspected the six-mood gallery at actual 128×64 pixels. The browser design approximation was also inspected for all five emotions, combined extreme inputs, and a finite ten-second demo.
- Added **Procedural curves / Preset morph** comparison to the preview. Same inputs produce clearly different geometry. Procedural mode gives independent direct feature bends; morph mode blends selected preset geometry toward expression anchors. Neither crossfades still images. No browser errors or warnings in the inspected preview.
- Review caught two issues before final handoff: hidden motion could redraw indefinitely, and the final snapped frame could be skipped. Fixed both, added regressions, and obtained a second review with no remaining important findings.
- First BLE iteration `DoodleV2` transferred 652556 bytes and was accepted/reconnected in 65.1 seconds. It was superseded by `DoodleV2r2` for the exact final-frame fix. The initial iteration is not the acceptance build.

## Hardware acceptance

- Refined `DoodleV2r2` compiled to **652568 bytes**, SHA256 `ebc4ce8744b52313f8d805ab24b05a765be8080d705c2c4c606113b6424fecde`. Authenticated BLE transfer accepted in **64.4 seconds**, rebooted and reconnected with `build=DoodleV2r2`, `eyes=1`, `doodle=1`, `ota=0`, `txerr=0`. Only the inactive application slot was written; no whole-flash erase or physical intervention.
- Final real BLE + independent USB face suite passed **19 exact target hashes**, **20 combined synthetic controller poses**, five invalid-command atomicity cases, six observed intermediate framebuffer hashes, hidden off/test transitions, no repeated idle transfers and a real disconnect-to-neutral reset. ACK range **38–120 ms**. Both hidden-screen cases started while animation was still moving.
- Legacy `eyes` suite passed 15 original pixel references plus 20 streamed poses; ACK **59–123 ms**. Legacy `face` suite, extended peripheral/touch suite and authenticated OTA negative cases passed. Corrupted/truncated/incomplete images, wrong key, wrong offsets and partial-transfer disconnect did not replace the running application.
- USB `i2c_ok=1`, `errors=0` and BLE `txerr=0` held across passing checks. This is electronic evidence, not a photograph/observation of OLED glass or physical pad-touch feel.
- Private raw logs remain in the local evidence/work folders. Public source has reusable tests and simulations, never the key-bearing binary.

Pending: native Face v2 app build/update handoff, source release publication and owner's physical judgment of the new face. iPad/direct Xbox/classroom repeatability remain separate unfinished tests.

## Native app handoff

- Restarted the local native alias app, selected the class toy, negotiated `doodle=1`, acknowledged Touch off and recognized the physically attached Xbox extended gamepad. Its **actual resting input** produced neutral doodle state over BLE, independently matched through USB; no claim of operating the new physical buttons/sticks while the owner was away.
- With controller input enabled, clicked **Build + update**. It compiled the same source as `Workshop-20261001-200710`: **652736 bytes**, SHA256 `68a0cf9c52110b7390d68854eee2ff106315e488c4b538918c207c62f20f79bc`. The app uploaded, rebooted, reconnected, acknowledged Touch off again and reapplied actual neutral input. Upload/reconnect took **73.3 seconds**; compilation was additional.
- Independent USB corroborated the new running build, `ota=0`, `doodle=1`, neutral `FACE seq=2`, framebuffer `59988171`, touch off and zero I2C/BLE errors. Native Stop delivered a neutral `FACE seq=3`, also seen over USB. Re-enabled the controller for the owner's return; saved the actual app screenshot.
- The app remains connected with controller input enabled. The accepted Face v2 source and comparison are ready for publication; the working v1 source tag/private return image remain intact.

Remaining: owner's physical Face v2 judgment and complete button/trigger/unplug/held-input-update qualification; repeat on student hardware; iPad uploader/client and direct Xbox-to-XIAO remain unfinished. No background monitoring was installed.

Final handoff readback: independent USB reported `Workshop-20261001-200710`, actual controller pose `FACE seq=5` neutral, `touch=0`, `doodle=1`, `settled=1`, framebuffer `59988171`, `i2c_ok=1`, `errors=0`, `txerr=0`. Frame count stayed8 across consecutive heartbeats. No new physical input range/button qualification is implied.

## Publication

Published the verified source as [v2.0.0-face](https://github.com/yury-g/OpenToy-Coursework/tree/v2.0.0-face), a bench prototype prerelease, on the verified personal account `yury-g`. Release code commit: `5ff99a7840b345308966000dc87d163109b7e11d`. The 372-file source archive excludes keys, profiles, backups, logs and compiled images; GitHub's SHA256 matches the local archive. A subsequent documentation commit corrects comparison screenshot framing and records this publication. The accepted firmware code is unchanged. The restricted Personal Brain checkpoint was updated by revision and read back once under the existing source event.

## Owner-selected stopping checkpoint

The owner said “Excellent,” chose this stopping point and explicitly requested that GitHub become private, everything be saved again and an animated tour be created. The verified personal `yury-g/OpenToy` repository was changed to **PRIVATE**. This does not add physical button/OLED qualification beyond the recorded evidence.

Generated the 38-second looping `docs/images/v2-face-tour.gif` directly from the unchanged firmware animator and pixel renderer: five expressions, all eye/brow/mouth extremes, jaw opening, blink, two winks, combined controls and a neutral return. There are 760 sampled frames, 330 encoded frames and 311 distinct OLED buffers. All 20 final pixel hashes match after GIF decoding. Representative decoded frames were visually inspected. The GIF is clearly labeled as a simulation. Export source, pinned Pillow requirement, full license and a hash/timing manifest are included.

The named stopping tag/release is `checkpoint-2026-10-01-face-v2`; see [the stopping-point guide](../STOPPING-POINT.md) for the source and local preservation inventory. No firmware behavior or device image changed during this save step. Existing source tags, firmware recovery paths and deferred tests remain preserved.
