# Three Feather personalities — November 16, 2026

This is the retained **1.2.0** installation record. For the current blink/power revision, see [revision history](../CHANGELOG.md) and [power verification](POWER.md). The README animation now reflects the newer revision.

## Scope

The three newly connected Reverse TFT Feathers receive Eyes 1 v1.2.0 with separate compile-time personalities: Curious, Dreamy and Cheeky. The previous Nesso and Feather pair was disconnected and is not reflashed in this step. All variants preserve 2× pupil geometry and the raised upper lids. The personality variants work without a gamepad; the classic build retains its previous controller-loss behavior.

## Backup and installation

Each board was independently identified through ESP32-S3 ROM as revision 0.2 with 4 MB physical flash and security flags 0. Each new stopped 4 MB snapshot matched its hardware digest. The original selected application is ota_0 at 0x10000 on all three, with two different capacities (0x2c0000 and 0x160000). Raw identities, full snapshots and prior slots remain in the private local `eyes-three-feathers-private` recovery folder.

Each enrolled identity is bound to its intended personality image by the installer. Curious and Dreamy images are 340,480 bytes; Cheeky is 340,496 bytes. All three bounded app writes occupy sectors 0x10000–0x63fff. Independent application verification and before/after hardware digests confirmed that all bytes outside each write range remain unchanged, preserving TinyUF2, partitions, OTA selection, NVS, secondary application and filesystems.

## Software and animation checks

The complete OpenToy host suite passes. New checks exercise one minute of no-gamepad animation for each personality, distinct rendered shapes and gaze tempos, independent blink activity, retained 2× pupils, synthetic poke/recovery without a controller, optional controller loss/reconnect and fitting inside the screen during eye-shape travel. All three Eyes 1 profiles and the classic Eyes 2 build compile with the pinned 2.0.17 toolchain.

The README animation runs the real personality, idle, reaction and rendering classes without gamepad input. Over 24 seconds Curious makes 11 gaze changes, Dreamy 6, and Cheeky 10, with 69, 52 and 65 distinct rendered frames respectively. Both eyes blink on every profile. The GIF loops for exactly 24 seconds; decoding verifies its pixels against the source frames. The image is a simulation, not a recording of the physical screens.

## Live checks

All three identify as `feather-eyes-1.2.0` with their expected personality names and autonomous mode enabled. Live telemetry confirms changing gaze, activity in both blink counters, targeted synthetic left/right pokes, independent angry reactions and return to happy. Animation also survives a 1.5-second sensor pause and 20 seconds of unread USB output while telemetry requests continue.

| Personality | Frame progress during stress/recovery | Elapsed | Measured rate |
| --- | --- | --- | --- |
| Curious | 635 frames | 24.104 s | 26.34 fps |
| Dreamy | 631 frames | 23.943 s | 26.35 fps |
| Cheeky | 640 frames | 24.270 s | 26.37 fps |

The initial brief synthetic poke could fall between telemetry reports; holding the diagnostic command through the sampling window made the live check deterministic. This required no firmware change.

No physical QT gamepads are detected on this batch. Optional controller controls and loss/reconnect are covered by host simulation; left/right live reactions use labeled serial diagnostics. These measurements do not establish physical gamepad operation on these three devices. Earlier owner confirmations for the prior pair remain separate.

## Owner observation

On November 16, 2026, the owner confirmed that Curious, Dreamy and Cheeky look distinct on their physical screens and blink/look around autonomously: “Yes, all three look good.” This completes the appearance check for the current three-board installation.
