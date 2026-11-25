# Animatronic Eyes revision log

Versions describe firmware behavior. Installation and measurement records distinguish tested hardware from simulations and estimates. No battery-runtime improvement is claimed without a current measurement or discharge test.

## Waveshare 1.1.0 — November 17, 2026

- Add visible Menu and common ordered pages for Return to play, Apps, Personality, Sound, Display, Controls and About; current-app information does not imply a launcher.
- Curve the default app/version/date/voltage/battery-icon header around the circular screen using tangent-rotated glyphs. Cache its raster to preserve animation timing.
- Replace hidden mute/personality holds with labelled settings, add bounded Soft/Quiet sound and dim/normal backlight, and teach performance restore before hiding the header. Restore opens Menu after an 800 ms top-edge hold.
- Require press/release within the same control; cancel drags, outside lifts, unknown releases and lost input. Stop held app actions/audio at menu transitions. BOOT/RESET/EN remain reserved; shared original renderers, mic-off and radio-off behavior remain unchanged.
- Full host suite, target and both Feather builds pass; independent review fixes verified. Installation/live evidence is recorded in [Waveshare verification](docs/WAVESHARE-VERIFICATION.md).

## Waveshare 1.0.2 — November 17, 2026

- Adds the V2 1.85C BOX adapter: round safe presentation, checked single-contact pokes, original large-pupil Companion behavior and all three idle personalities.
- Implements the common default-visible app/version/creation-date/battery header from the first build, with reversible session-only performance mode, mute and personality holds outside the eye zones.
- Adds quiet 80 ms synthesized reactions, fades/retrigger limits and independent amplifier-off timer. Microphone and radios stay off.
- App-only installation and readback passed with all flash outside the write range unchanged. Full host suite, Waveshare build and existing Feather Eyes 1/2 builds pass. Live tests cover synthetic independent reactions, idle personalities, header/mute, repeated audio, stale sampling and USB backpressure; observed ~17 fps.
- Fixes the first port's slow floating-point projection, small-strip overhead and overstrict codec reserved-bit readback comparison. Intermediate 1.0.0 and 1.0.1 records remain in [Waveshare verification](docs/WAVESHARE-VERIFICATION.md).
- Owner visual/touch/audio acceptance remains pending. Battery input is zero on the USB bench, displayed as `--`; battery switch/connection and runtime are unverified. No change to Feather/Nesso behavior, shared original renderers or old release tags.

## Feather 1.3.0 — November 16, 2026

- Keeps the visibly distinct blink patterns from 1.2.1.
- Adds always-visible eco mode for the three personalities: 80 MHz CPU, backlight duty 28/255, 20 Hz updates, exact unchanged-frame reuse, unused NeoPixel power disabled, slower idle I2C/USB housekeeping.
- Adds `--power normal` for comparison and rollback. Classic Eyes 1, Eyes 2 and the disconnected Nesso remain on their prior power behavior.
- Adds on-device counters for actual CPU setting, backlight setting, screen transfers, skipped redraws and drawing work. Host checks prove skipped frames leave the displayed eyes unchanged.
- Deployed to all three and passed live behavior/stress checks at about 18 updates/s, skipping 69–74% of redraws during idle captures. Dreamy required one USB diagnostic restart while its face continued animating; subsequent verification passed.
- Current and battery runtime remain unmeasured. See [power capabilities, limits and verification](docs/POWER.md).

## Feather 1.2.1 — November 16, 2026

- Curious: quick double blinks, with a separate repeat phase for each eye.
- Dreamy: slow, lingering closures lasting 1.45–1.95 seconds; the right eye leads.
- Cheeky: alternating solo winks, right eye first, with irregular 1–2 second start spacing.
- Preserves large pupils, raised lids, eye shapes, tints, optional gamepad controls and immediate poke priority.
- Host tests cover distinct sequences, duration, non-overlapping solo winks, interruption and timer wrap. Curious build used as the normal-power comparison before the power-saving revision; this revision was not deployed to all three.

## Feather 1.2.0 — November 16, 2026

Introduced Curious, Dreamy and Cheeky with different shapes, tints, gaze tempos and autonomous no-gamepad play. Installed and checked on three Feathers. The owner confirmed appearance and autonomous play, then requested stronger differences in blink order/timing. [Original verification](docs/PERSONALITIES-VERIFICATION.md).

## Earlier retained versions

- Feather 1.1.3: raised happy lids with 2× pupils; installed on the earlier gamepad Feather.
- Nesso 1.3.2: matching pupil/lid refinement; retains continuous gravity orientation, touch reactions, asymmetric anger, trembling, recovery and independent idle eyes.
- Historical Eyes 1 / Eyes 2 source and release tags remain in OpenToy. See the project README and verification records for their controls and deployment history.
