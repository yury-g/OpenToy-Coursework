# OpenToy course work

This collection is an evolving record of toy experiments and course development, with work planned through **January 10, 2027**. Working builds, prototypes, simulations and ideas each have a place here. Project guides record the evidence available for each device. The gallery captions identify rendered demonstrations and photographed results.

Start with a device in the [visual home page](../README.md), open an app guide and check its required hardware and build instructions. Firmware targets a particular chip, display and wiring. Board addresses in this copy are example identities; configure your own devices before pairing or connecting them.

## Project source map

| Project | Source included in this copy |
| --- | --- |
| Animatronic Eyes 1 and 2 | [Feather, Nesso and round Waveshare](../projects/animatronic-eyes/README.md) |
| Gravity Water | [Nesso and Feather](../projects/gravity-water/README.md) |
| Orange Face | [Firmware and controls](../projects/orange-face/README.md) |
| Nesso Face | [Touch and motion face](../projects/nesso-n1/README.md) |
| Copy-Face | [CrowPanel prototype](../projects/copy-face/README.md), [Tab5 integration](../projects/tab5/README.md) |
| Camera Eye | [Camera helper](../projects/vibe-coding-camera-eye/README.md) |
| UNO Q Eyes | [Parallel TFT shield port](../projects/uno-q/README.md) |
| Reflective LCD Eyes | [Waveshare RLCD](../projects/waveshare-rlcd-4.2/README.md) |
| CrowPanel Eyes | [Large-screen port](../projects/crowpanel/README.md) |
| Nearby Buddy | [Full companion source and enclosure](../companions/nearby-buddy/main/README.md) |
| Battery Faces | [Base](../companions/battery-faces/main/README.md), [gamepad](../companions/battery-faces/feature/seesaw-gamepad/README.md), [buttons and haptics](../companions/battery-faces/feature/qwiic-accessories/README.md), [archived camera experiment](../companions/battery-faces/archive/camera-test-v17/README.md) |
| ditherCam | [Flip camera firmware](../projects/ditherCam/README.md) and [enclosure](../projects/3d-printed-designs/ditherCam/README.md) |
| CYD tri-fold | [A6 frame, viewer, print files and design history](../projects/3d-printed-designs/cyd-trifold/README.md) |
| Printed parts | [Orange Face and other designs](../projects/3d-printed-designs/README.md), [Plant](../mechanical/plant-v02/README.md) |
| Earlier XIAO toys | [Archived boards and guides](../archive/README.md) |
| Manufacturer designs | [Eight-platform hardware gallery](../hardware/README.md) |

## What is still in progress

Tab5 camera capture worked over USB, but its local display was dark at the November 17 checkpoint. Copy-Face smile and frown copying still needs mouth tracking. UNO Q touch is unqualified. Battery readings and charging indicators vary by board and need the checks listed in the device guides. [Full roadmap](ROADMAP.md) · [Saved Tab5 patch](development/README.md).

## Source history and reuse

This repository starts with reviewed source snapshots. Feature and device branches retain their build layouts with fresh histories. Historical tags identify sanitized source snapshots of the earlier releases. The companion directories retain their own licenses and project structure. Source revision references in [SOURCE-SNAPSHOTS.json](SOURCE-SNAPSHOTS.json) record where the imports came from.

Device addresses and workstation paths were replaced with examples. Preview metadata was removed. Configure actual devices locally. Original hardware licenses and attribution accompany the manufacturer files. The original development repository and its history are retained separately.

This is a private review copy. The owner will choose when to enable public access. Once public, readers can browse its contents without signing in.
