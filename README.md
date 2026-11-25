# OpenToy

OpenToy is an MIT-licensed teaching prototype for changing a small toy over Bluetooth. This working repository is private at the owner's request as of October 1, 2026. Start with live behavior changes, then add carefully tested firmware updates. The bench project includes two Seeed Studio XIAO ESP32-S3 boards and a native Mac control app; the OLED class board was attached at the Face v2 handoff.

## Watch Face v2

![OpenToy Face v2 animated tour: expressions, eye and brow bends, mouth puppetry, blinks and winks](docs/images/v2-face-tour.gif)

**38-second looping firmware simulation.** Five expressions, eye/brow/mouth controls, jaw opening, blink, both winks and combined movement. It uses the actual firmware's curve renderer and spring/wiggle animation; it is not a video of the physical OLED. [Controls and animation](docs/V2-FACE.md) · [Saved checkpoint](docs/STOPPING-POINT.md).

## Visual development history

![OpenToy development: USB and Bluetooth checks, v1 eye puppetry, face concepts, procedural versus morph preview, accepted Face v2 and saved checkpoint](docs/images/history-overview.png)

[**Open the complete visual design log →**](docs/DESIGN-HISTORY.md) Each major stop records what changed, what was checked, the visual evidence and its source/recovery checkpoint. The sequence includes early USB/Bluetooth work, the class OLED/touch circuit, Xbox smoke test, layered eyes, face drawings, both animation approaches, firmware refinements, native app handoff and saved releases.

| Preserved stop | Revisit it |
| --- | --- |
| First controller/face smoke test | [Historical source archive](https://github.com/yury-g/OpenToy-Coursework/tree/checkpoint-2026-10-02-visual-history) · [Evidence](docs/DESIGN-HISTORY.md#04--first-controller-smoke-test) |
| V1 · separate whites, pupils and eyelids | [v1.0.0-eyes](https://github.com/yury-g/OpenToy-Coursework/tree/v1.0.0-eyes) · [Visuals and controls](docs/V1-EYES.md) |
| Face v2 · drawing alternatives | [Design-review source](https://github.com/yury-g/OpenToy-Coursework/tree/9126153) · [Concept sheet](docs/concepts/V2-FACE.md) |
| Face v2 · procedural curves | [v2.0.0-face](https://github.com/yury-g/OpenToy-Coursework/tree/v2.0.0-face) · [Procedural/morph comparison](docs/V2-FACE.md#try-both-approaches-without-flashing) |
| Owner-selected stopping point | [checkpoint-2026-10-01-face-v2](https://github.com/yury-g/OpenToy-Coursework/tree/checkpoint-2026-10-01-face-v2) · [Recovery inventory](docs/STOPPING-POINT.md) |
| Visual-history edition | [checkpoint-2026-10-02-visual-history](https://github.com/yury-g/OpenToy-Coursework/tree/checkpoint-2026-10-02-visual-history) · [Complete design log](docs/DESIGN-HISTORY.md) |

The installed version still uses **controller → Mac → Bluetooth → toy**. Power-only, computer-free controller operation is the next requested experiment and is not implemented yet. The controller remains usable for Apple TV games; no controller firmware was modified.

**The working path:** Mac app → Bluetooth LE → toy. The Mac can keep its internet connection on Ethernet or Wi-Fi. Campus Wi-Fi is not needed for live control or for transferring a locally built update.

**Face v2** adds playful curved eyes, brows and a mouth, with continuously adjustable curvature and a short damped wiggle. Xbox input uses Apple's GameController framework: left stick bends eyes/brows, right stick bends/pulls the mouth, triggers control jaw/blink, D-pad winks and buttons select five expressions. The [guide](docs/V2-FACE.md) includes exact pixel simulations and an algorithmic-versus-preset-morph comparison. Version 1 eye puppetry remains preserved as `v1.0.0-eyes`.

A physical Microsoft USB controller is recognized by Apple's Xbox Series X driver. The owner confirmed the original eyes work. New face electronic verification and remaining physical judgments are tracked in the [progress log](docs/concepts/V2-PROGRESS.md).

## Start here

- [Visual design log and checkpoints](docs/DESIGN-HISTORY.md): development history with drawings, exact pixel simulations, real app screenshots and saved versions.
- [Saved stopping point, October 1](docs/STOPPING-POINT.md): current version, animated tour, reproduction and recovery.
- [Version 1: eye puppetry, screenshots and controls](docs/V1-EYES.md): the working baseline preserved as `v1.0.0-eyes`.
- [Face v2: procedural doodle puppetry](docs/V2-FACE.md): controls, algorithm, simulations, build/test commands and recovery.
- [Face v2 progress](docs/concepts/V2-PROGRESS.md) and [concept drawings](docs/concepts/V2-FACE.md): the selected direction and dated iteration record.
- [Setup and reproducible commands](docs/SETUP.md): identify the chip, preserve source, back up flash, generate each toy's private key, seed once by USB, build the local Mac launcher.
- [Student workshop](docs/WORKSHOP.md): live changes first, controller mapping, then one small firmware change.
- [Controller design and compatibility](docs/CONTROLLERS.md): tested boundaries, mappings, USB/Bluetooth model checks and the later iPad path.
- [Protocol and firmware updates](docs/PROTOCOL.md): authenticated upload, inactive slot, verification and recovery.
- [Recovery](docs/RECOVERY.md): interrupted transfers, rollback and USB fallback.
- [Bench evidence and limitations](docs/TEST-RESULTS.md): what actually passed and what still needs hardware/student testing.

## Hardware examples

| Example | Required hardware | Confirmed configuration |
| --- | --- | --- |
| `projects/xiao-toy` | Bare XIAO ESP32-S3 | USB serial, GPIO21 onboard LED state, BLE live controls and OTA |
| `projects/xiao-class` | XIAO ESP32-S3 plus the documented class circuit | SH1106G 128×64 at I2C 0x3C; SDA GPIO5/D4, SCL GPIO6/D5; native touch inputs GPIO1–4/D0–D3 |

The class configuration came from preserved existing source and successful electronic tests. Do not apply it to an unknown OLED or touch module. An I2C acknowledgment alone cannot identify its display controller. No OLED glass appearance or physical pad-touch response was observed remotely.

## Reproducibility status

Two physical boards were ROM-identified, backed up and USB-seeded; live Bluetooth control and full-image updates were exercised. Actual failed-boot rollback worked on both. A local native app compiled, updated and verified each toy. Face commands were independently corroborated through USB and checked I2C writes.

The launcher uses local alias packaging: it needs its project and Python runtime to remain installed. There is no signed portable installer. The local executor launched the app successfully; background first-launch behavior still needs investigation. First-time provisioning on additional student hardware and other Macs remains unvalidated. The included preparation helper is covered by host identity checks; it is not a claim that a whole classroom deployment has been tested.

iPad live control has a researched native-client route, but iPad testing was deferred. The custom Bluetooth firmware uploader is implemented for Mac, not iPad. An AI-to-iPad build-delivery service and direct Xbox-to-XIAO connection are separate, unfinished work.

## License and private files

Original OpenToy code is MIT licensed. Third-party licenses and notices are preserved under each example's `licenses/` and `THIRD-PARTY-NOTICES.md`. Arduino/SDK dependencies retain their licenses; vendor radio binaries are not entirely open source.

The optional GIF exporter uses Pillow 12.3.0; its full license is preserved in [licenses/](licenses/README.md).

Keep `device.json`, `Secrets.h`, keys, flash backups and all compiled images private. The images embed that toy's key. They are ignored by Git and excluded from the source archive. Preserved pre-existing class source is a private preservation artifact, not relicensed by OpenToy.
