# Gravity Water · ready to play

**Saved November 15, 2026 · v0.3.0 · Arduino Nesso N1**

The working toy is installed and left available for play. This checkpoint changes
presentation and documentation, not the firmware on the device.

## The saved toy

- Quiet rest; hold the yellow button to boil; release to cool quickly.
- Blue-to-red energy colors, touch repulsion and quick shake scattering.
- Generated noise/tone texture with smooth start and stop.
- 400 shaded particles, continuous tilt behavior, and the fixed battery/face bar.

Installed firmware source commit: `037d023624d59c1144ae50a347e154f6d4974fa7`.
Owner-confirmation record: `d9b7cdf`.
Release checkpoint: [`gravity-water-v0.3.0`](https://github.com/yury-g/OpenToy-Coursework/tree/gravity-water-v0.3.0).
Development branch: `feature/gravity-water`.
Review: [PR #2](https://github.com/yury-g/OpenToy/pull/2), based on `feature/nesso-n1`.
The separate Face branch and chapter remain preserved.

## Confirmed

The owner confirmed physical direction and particle appearance, boiling,
colors, cooling, audible final sound, and responsive finger dragging and shaking.
The full host suite passes with address/undefined-behavior checks. App-only flash
was independently verified. Final reboot and a short run through 135 seconds
uptime showed no resets. These are bounded tests, not long-term endurance proof.

The display bypasses the observed DMA FIFO stall. Audio uses a fixed-work PWM
routine instead of the stalled speaker queue. The [bench record](BENCH.md)
keeps the measurements and remaining limits separate from owner observations.

## When work resumes

Start with the owner's play observations. No feature changes are queued.
Preserve the installed version until there is a concrete reason to change it.
Review this checkpoint, `BENCH.md`, and the private recovery record before any
new flash. Backups, compiled images, logs and hardware identity are outside Git.
Do not infer authorization to restore Face, erase flash, change repository access,
or merge the draft PR from this stopping-point note.
