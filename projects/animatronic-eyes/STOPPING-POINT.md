# Stopping point — November 16, 2026

The three newly connected Reverse TFT Feathers now run **Curious, Dreamy and Cheeky**, all on Eyes 1 v1.3.0. Each retains the 2× pupils and raised upper lids, with its own eye shape, tint, glance tempo and blink timing. They animate without a gamepad; an optional QT gamepad enables the existing controls.

## Installed devices

| Device | Installed build | Behavior |
| --- | --- | --- |
| Curious Feather | `feather-eyes-1.3.0`, Curious | Taller cool-white eyes, quick wide glances and double blinks |
| Dreamy Feather | `feather-eyes-1.3.0`, Dreamy | Shorter warm-white eyes, slower smaller glances and lingering blinks |
| Cheeky Feather | `feather-eyes-1.3.0`, Cheeky | Uneven mint-white eyes, alternating solo winks |
| Earlier Nesso N1, now disconnected | `nesso-eyes-1.3.2` retained | Touch reactions, independent anger/recovery, smooth gravity rotation and idle play |
| Earlier Eyes 2 / Gravity Water Feather, now disconnected | `feather-eyes-1.1.3` retained | Large-pupil Eyes 1 with QT gamepad and idle behavior |

The prior pair was not reflashed in this step. Eyes 2 remains a selectable firmware version. Classic Eyes 1 still returns to neutral without its required gamepad; the three personality builds instead resume autonomous play.

The personality builds use always-visible eco mode: dimmer backlight, 80 MHz CPU, 20 Hz updates and skipped identical screen transfers. Battery runtime has not been measured. [Power research and results](docs/POWER.md) · [Revision history](CHANGELOG.md).

## Saved source and visuals

- [Project README, personality animation and controls](README.md)
- [Three-personality animation](docs/images/feather-personalities.gif)
- [Animated device family](docs/images/animatronic-family.gif)
- [Nesso controls](NESSO.md)
- [Current three-board verification](docs/PERSONALITIES-VERIFICATION.md)
- Earlier [Feather verification](docs/VERIFICATION.md) and [Nesso verification](docs/NESSO-VERIFICATION.md)

Animations run firmware rendering code; they are simulations, not physical-screen recordings. Original shared renderers and historical release tags remain preserved.

## Verification

The full OpenToy host suite and all three personality builds plus classic Eyes 2 pass. Each installation was independently checked, including unchanged flash outside its application write. The earlier 1.2.0 builds passed live checks at about 26 fps. The current eco builds pass at about 18 updates/s, with 69–74% of redraws skipped in idle samples. The power verification document includes measured results and Dreamy’s USB diagnostic recovery event. No physical gamepad was attached to this batch; optional controls were checked in host simulation. The owner confirmed the original three appearances and autonomous motion, then requested more visibly distinct blink timing/order; this revision implements that refinement. See the detailed verification record above.

## Recovery

Fresh full-flash backups, original application slots, identities, installation logs and recovery instructions for the three current boards are saved outside Git in the private local `eyes-three-feathers-private` folder. The 1.2.0 snapshots and the 1.2.1 comparison image preceding this update are retained in `eyes-distinct-blinks-private`. Earlier pair records remain separately in `animatronic-nesso-private` and `feather-ani-companion-private`. Any future flashing must re-identify the target and preserve its original layout.
