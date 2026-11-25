# OpenToy app status bar — design standard v1.0

Created: 2026-11-17. Applies to existing maintained apps and all new apps/device ports. This is the shared design specification; the adoption table below is not a claim that every installed device already implements it.

The owner wants to identify the running app by looking at the device, then hide technical labels for performance. The normal display therefore shows the app, its installed version, the creation date of that version, and the local battery indication together.

![Illustrative layouts for rectangular, small and round displays](images/app-status-bar-spec.png)

*Design mockups, not screenshots of installed firmware. App versions, dates and battery readings in the mockups are illustrative.*

## Required content

| Item | Rule |
| --- | --- |
| App | A stable, recognizable short name, such as `EYES1`, `EYES2`, `WATER` or `ORANGE`. Keep a registry in each app's metadata; do not rename its repository or project folder to fit the screen. |
| Version | The version of the firmware actually installed, including any prerelease suffix. Never display a hard-coded old label while loading newer behavior. |
| Creation date | `YYYY-MM-DD`, the creation/release date of that version. It is fixed in version metadata, not the device clock, boot date, flash date or current date. The original app's first creation date may be recorded separately. |
| Battery | Right-aligned within the visible header. Show a battery icon and genuine available information: gauge percentage when supported, otherwise measured voltage. Use `--` when unavailable. Do not invent a percentage from an uncalibrated voltage reading. Only show charging when the hardware actually reports it. |

Suggested one-line layout: `EYES1 v1.3.0 2026-11-16       82% [battery]`.

Battery refers to the device providing this display. A companion board's reading must not be presented as the display board's battery without an explicit label. Personality names, connection state and a short source revision may be supplementary; they must not displace the required identity fields.

## Layout and readability

- Use **about 10 logical pixels** for a single row when the display and font can fit everything legibly. The current 15-pixel Feather/Nesso strip is an acceptable migration starting point. This is a readability target, not a forced universal height.
- Keep all four required fields visible together. Do not truncate versions/dates, scroll them away or alternate between identity pages to make the bar appear to fit.
- On a narrow display, use a second compact row rather than making text unreadable. For example, app/version/battery on row one and creation date on row two.
- On a round or clipped display, put the header at the top of the **safe visible area**, inset enough to fit inside the circle. The battery occupies that row's right end, not an invisible rectangular corner.
- Use high-contrast text and a consistent left-to-right reading order. Scaling should suit the physical screen; a high-resolution large screen does not need microscopic ten-physical-pixel text.
- The app renderer receives a content rectangle excluding the visible header. Eyes, particles and touch targets must fit that rectangle. Rotating apps rotate/reflow the bar consistently with the face and keep its battery and text within bounds.
- When the header is hidden, the app may reclaim the space, but must update its rendering and touch transforms together. Hiding metadata must not change the app's behavior, personality or timing.

## Default and performance behavior

1. On a normal boot, the status bar is **on**. Every new app/port inherits that default.
2. A deliberate, documented user action enters performance mode and hides the complete status bar. An explicit app launcher/system menu can offer the same toggle.
3. A reliable, documented action restores the bar without rebooting. Hiding it must not hide the only way to restore it.
4. Each device chooses an appropriate gesture or button combination after checking its existing controls. Do not consume ordinary eye pokes, gamepad buttons or touch gestures already used by the app without updating the control design.
5. Performance mode is session state. It can remain active while switching apps within that session; a reboot restores the default visible bar. Do not silently save a permanent hidden setting.
6. No periodic debug labels, automatic metadata popups or test overlays should interrupt a performance. Necessary fault indications should be explicitly designed and documented.

## Metadata and implementation contract

Each target should consume one canonical version record containing at least:

- `app_id` and `display_name_short`;
- `firmware_version` and `version_created`;
- `hardware_target` and a source revision/build identifier for diagnostic output;
- the local battery source and its supported reading type.

The README, release record, status bar and status-query output must agree on version/date. A changed firmware behavior gets an appropriate version change and dated revision-log entry. Do not assign today's date to an old release merely because it was rebuilt. For legacy releases whose dates are not yet verified, report the date as unknown and track it as a migration gap rather than guessing.

Keep the header layout/state separate from each app's animation logic and from hardware battery reads. Use a common implementation where toolchains permit it, or equivalent adapters where they do not. Do not force unrelated boards to share pin definitions, drivers or firmware images. Battery reads and diagnostics must remain bounded and must not block animation if a peripheral is absent.

## Adoption and compatibility tracking

Existing and future apps are in scope. Historical tags and original binaries remain unchanged; adoption happens in maintained source branches and new versioned builds. Some projects live outside OpenToy's main branch or in their existing standalone repository; follow the [project directory](../projects/README.md) rather than relocating them.

| App/track | Starting status | Required next step |
| --- | --- | --- |
| Animatronic Eyes 1 and 2 — Feather | Existing name/battery strip; full version/date/performance standard not yet implemented | Add the shared metadata and reversible performance control; rebuild/test each maintained variant |
| Animatronic Eyes 1 — Nesso | Existing rotating status/battery presentation; full standard pending | Preserve smooth rotation and touch mapping during migration |
| Waveshare 1.85C BOX device track | Eyes 1.0.2 installed; canonical metadata, round safe two-row header and session controls pass host/live checks | [Verification](../projects/animatronic-eyes/docs/WAVESHARE-VERIFICATION.md): owner visual/physical-touch check pending; battery input zero displays `--`. [Catalog matrix](../projects/waveshare-1.85c/README.md) tracks remaining ports |
| CrowPanel P4 + XIAO device track | Native P4 Eyes 1 0.2.2 installed; canonical metadata and session header controls pass host/live checks; local battery `--` | [Verification](../projects/crowpanel/EYES1-VERIFICATION.md): owner reported the smooth eyes look great; battery remains unqualified. XIAO is unused by this port; [matrix](../projects/crowpanel/COMPATIBILITY.md) tracks the remaining apps |
| Gravity Water, Orange Face, Nearby Buddy, Battery Faces, Nesso Face | Adoption not audited in this change | Audit their maintained sources, existing controls and battery capabilities, then record versioned migration results |
| Archived XIAO and other historical apps | Historical releases retained | Record compatibility; apply the standard when making a new maintained port, without rewriting old release history |

Each device track keeps a per-project matrix: source location, required peripherals, port status, installed version, display/touch/audio/network checks, and remaining limitations. “Builds,” “runs on hardware,” and “owner-observed behavior” are separate states. Updating this design document does not mark a firmware port complete.

## Acceptance checks for a migrated app

- Verify app/version/date against the installed build record and diagnostic output.
- Check the entire header at every supported orientation, including circular corners, long version strings and missing battery readings.
- Confirm the header is visible after reboot; performance mode hides it, remains quiet, and can be exited without rebooting.
- Confirm switching apps updates the identity and preserves performance session state intentionally.
- Confirm hiding/showing does not break app bounds, touch coordinates, gamepad mappings or animation timing.
- Confirm battery failures do not freeze the app and do not display fabricated charge information.
- Record the actual device/build tested. Simulations and visual mockups are supporting evidence, not substitutes for physical observations.

## Change history

- v1.0 — 2026-11-17: app identity, installed version, version creation date, right-aligned battery, default-visible status bar, reversible session-only performance mode, adaptive layouts and backward adoption tracking.
