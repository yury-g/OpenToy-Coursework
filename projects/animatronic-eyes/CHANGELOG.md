# Animatronic Eyes revision log

Versions describe firmware behavior. Installation and measurement records distinguish tested hardware from simulations and estimates. No battery-runtime improvement is claimed without a current measurement or discharge test.

## Feather 1.4.0 & Nesso 1.4.0 (source & target-compiled candidates, not yet flashed) — November 17, 2026

- **Shared `SoundWake.h` (`shared/SoundWake.h`, `tests/sound_wake.cpp`)**: adds local volatile-RAM acoustic wake and expressive listening (adaptive ambient noise floor, 2-block onset debounce, Curious/Dreamy/Cheeky attack/settle/perk profiles, speaker self-trigger ringdown suppression, and priority yield to pokes/camera). Off by default; zero audio storage or upload.
- **Nesso N1 `1.4.0` (`firmware/NessoEyes/{Metadata.h,NessoMenu.h,Presentation.h,Nesso.cpp}`, `tests/nesso_menu.cpp`)**: upgrades the rotating `15-px` HUD to the full v1.0 status contract (`EYES1 V1.4.0`, `2026-11-17`, truthful `BAT --` or `XX%` gauge, and `[MENU]` pill fitting all `0..360°` angles including `45°` diagonals), adds the gravity-rotated 7-entry touch Menu (including Classic/Curious/Dreamy/Cheeky selection on Nesso), and adds reversible session Performance mode (`0.8s` top-center hold restore). Passed ESP32-C6 target compile (`446,094` bytes); physical bench Nesso retains `1.3.2` until connected.
- **Feather Reverse TFT `1.4.0` (`firmware/AnimatronicEyes/{Metadata.h,FeatherMenu.h,Power.h,Feather.cpp}`, `tests/feather_menu.cpp`)**: upgrades the `240x15` status strip to `EYES1`/`EYES2 V1.4.0 2026-11-17` + truthful `BAT --` / `XX%`, maps front bezel buttons **`D1` (`GPIO1` Primary)** and **`D2` (`GPIO2` Secondary)** while excluding **`D0` (`GPIO0` BOOT strap)**, disambiguates stationary `Select+Start` (`< 800 ms` blink on release vs `>= 800 ms` Menu/restore without blink leak), and updates `EYES_ECO` `RefreshGate` stamps. Passed ESP32-S3 target compiles for Eyes 1 Curious (`346,961` bytes) and Eyes 2 (`349,161` bytes); physical bench Feathers retain `1.3.0` until connected.

## Waveshare 1.1.1 — November 17, 2026

- Enlarged all round-screen touch targets (`>= 48 px` tall inside `r < 176`), paginated root Menu to 2 rows across 4 pages (`Menu 1 of 4`), added 8 ms inter-strip touch polling + `FrameSnapshot` tear-free rendering, high-contrast cyan press feedback, and `TOUCH` lift/release telemetry (`lifts`, `lift0`, `rel`).
- Installed in bounded factory slot (`430,393` bytes), verified via USB `?` telemetry (`version=1.1.1`) and live Tab5 Camera Eye captures (Play eyes, `Menu 1 of 4`, and 12-frame Play gaze burst); physical fingertip tap check pending.

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
