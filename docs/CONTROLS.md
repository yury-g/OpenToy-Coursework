# OpenToy controls — family standard v1.0

Created 2026-11-17 from the owner's request for consistent exposed buttons and touch controls. This is the target interaction design. Existing firmware retains its documented mappings until a versioned migration is installed and checked. The [home-page matrices](../README.md) show both current and intended behavior.

## Shared meanings

Use the same words and action roles on every device: **Play, Menu, Next, Previous, Select, Back**. A device profile maps its accessible user buttons or touchscreen to those roles. Never assign ordinary play, menu, personality, mute or performance actions to **BOOT, RESET or EN**, even if software can read them. Keep power controls electrical unless separately qualified. Audit boot-strapping functions as well as printed labels before choosing a button.

| Intent | Button interaction | Touch interaction |
| --- | --- | --- |
| Play | Short press of Primary performs the app's labelled primary action | Touch the displayed object: an eye reacts where touched; Water responds at the contact point |
| Open Menu | Hold Primary for 0.8 seconds | Tap the visible **Menu** control in the safe header |
| Choose an item | Next/Previous moves focus; Select activates it | Tap a labelled item; do not require hidden swipes |
| Return to play | Back, or the explicit **Return to play** item | **Back** or **Return to play** |
| Enter performance | Select **Display → Performance** | Same menu item |
| Leave performance | Hold Primary for 0.8 seconds to restore information and open Menu | Hold the top-center safe-edge zone for 0.8 seconds to restore information and open Menu |

Primary is a role, not a universal physical button name. Print the chosen physical mapping in each device guide and **Controls** help. Keep app-specific manipulation distinct from menu navigation; a stick moving the eyes must not unexpectedly move menu focus while playing.

## Profiles for differing controls

| Available controls | Mapping in play | Mapping while Menu is open |
| --- | --- | --- |
| One qualified user button | Short press: app primary action; hold: Menu | Short press: Next; hold: Select. Include explicit Back/Return entries; no double-click or extra-long-press requirement |
| Two qualified user buttons | Primary: short action / hold Menu; Secondary: labelled secondary action | Primary short: Next / hold: Select; Secondary short: Previous / hold: Back |
| Three or more qualified user buttons | Assign a clearly identified Primary/Menu role; remaining controls remain app-labelled | Next, Previous and Select; provide Back on screen if no fourth button |
| Touchscreen, with or without buttons | Direct object interaction plus visible Menu | Same labelled menu, finger-sized targets adapted to screen shape; user buttons, if present, invoke the same actions |
| QT gamepad | Retain stick and expression controls; proposed Menu entry is a stationary Select+Start hold | Stick/D-pad: navigation; A: Select; B: Back |

The gamepad proposal conflicts with the existing Select+Start blink/close chord. It is a **migration item**, not a working shortcut today. Before enabling it, document/test a short-chord blink versus long-chord Menu without leaking a blink into Menu, and retain a clearly labelled alternative blink action. Existing standalone BOOT-based RLCD gestures likewise require a versioned replacement using **KEY** after its role is qualified.

For one-button Eyes, the primary action is a brief blink/reaction; expose **Poke left / Poke right** as explicit app menu actions if separate direct inputs are unavailable. Do not pretend that one button provides simultaneous independent eye holds. Touch and gamepad retain their richer direct controls.

## Menu and presentation

Keep this order and wording wherever supported: **Return to play**, **Apps**, **Personality**, **Sound**, **Display**, **Controls**, **About**. Back returns one level. Initial focus is Return to play. Personality names remain Curious, Dreamy and Cheeky where implemented. Sound contains mute/volume and, once implemented, listening sensitivity. Display contains Performance and applicable brightness/orientation settings. About contains app/version/date, board and honest battery information.

Show only installed apps as launchable. A single-app build can show its current app and “More apps require installation”; it must not imply a working multi-app launcher. Mark unavailable options with a short reason or omit irrelevant controls; never present dead buttons. A reflective screen has no brightness/backlight slider. Future features belong in the roadmap, not as working controls.

The [status-bar standard](APP-STATUS-BAR.md) still requires app, version, creation date and battery together. Add Menu without displacing those fields: adapt height/rows and safe bounds. Use text labels with icons, strong contrast and a clear focused/pressed state. Color cannot be the only signal. Keep targets legible on monochrome displays and reachable inside a round screen; scale for physical touch size instead of a universal pixel size.

Performance mode hides the status area, menus and persistent control bars. It stays hidden until a deliberate restore action; reboot restores normal information. Teach the restore gesture in Controls and when entering performance, without recurring overlays. The top-center restore zone follows display orientation and uses the same safe geometry on rectangular and round displays. Define and test its bounds per target. It must be reachable without using BOOT/RESET or a hidden button.

## Input rules and acceptance

- Debounce buttons; issue a short press on release only if no hold fired. A hold fires once, consumes the pending short action and requires release before retriggering.
- Touch menu actions require a touch begun in the control and released there; cancel when dragged out. Restore holds require a contact begun and held in their zone. Do not let a gesture started in play accidentally activate Menu.
- On opening/closing menus, changing apps, or losing input, release held app actions. Resume only after a fresh contact/press. Touch-to-scene transforms must match rendering after rotation/header reflow.
- Verify one-button navigation can reach and exit every submenu, including performance. Test accidental holds, stuck/disconnected inputs, long labels, all supported orientations and simultaneous touch/button use.
- Record target/version, physical button identity, mapping, tests and owner observations. A documentation change does not migrate installed firmware. Preserve old releases and publish a clear before/after controls note.

## Adoption sequence

New Copy-Face follows these roles from its first maintained UI. RLCD replaces BOOT play/chords with the qualified KEY menu. CrowPanel consolidates its footer controls into the common menu; round Waveshare replaces hidden mute/personality zones with labelled menu items. Nesso and Feather gain the menu while preserving their existing direct play controls. Exact button qualification and physical checks belong to each device track.
