# Eyes common controls — 0.3.0 candidate

Created 2026-11-17. **Source and build candidate only; not deployed by this track.**
The last verified standalone Eyes checkpoint is 0.2.2, documented in
[Eyes verification](EYES1-VERIFICATION.md). Hardware remains released to Copy-Face;
this change opened no serial port and did not reset or flash the panel. Current
hardware state belongs to that project. Battery remains unknown.

## Controls

| Intent | Candidate behavior |
| --- | --- |
| Play | Touch either eye, or both, for the existing independent reactions |
| Open Menu | Tap the visible Menu button; release inside to activate |
| Return | Return to play; Back moves one menu level |
| Apps | Current Eyes 1 only; additional apps require installation |
| Personality | Curious, Dreamy, Cheeky; the current choice is labeled selected |
| Display | ECO toggles existing brightness/cadence settings; Performance hides all bars |
| Performance restore | Begin and hold one finger in the top-center zone for 0.8 seconds; information and Menu return |
| Controls / About | On-screen gesture help, version/date/board and honest battery information |

The normal header is 88 px high, with metadata, BAT--, current personality/ECO and
Menu. The former footer is removed. Performance uses the full screen with no
persistent controls or recurring reminders. Its entry page and Controls teach
the restore gesture first. Fixed-landscape restore bounds are x352–671, y0–63.
Leaving the zone cancels the hold until release. Restart defaults remain normal
information, Curious and ECO ON. No BOOT/RESET/EN app mappings.

The menu follows the [family standard](../../docs/CONTROLS.md). Sound is omitted
because audio is unavailable in this build. There is no app launcher, brightness
calibration, automatic orientation or battery percentage/charging claim. ECO
still maps to driver brightness 35/65 and requested 66/33 ms cadence; these are not
measured brightness or runtime. Performance is not a promised frame-rate boost.

## Input behavior and reusable source

[Menu.h](firmware/main/Menu.h) contains the portable page model, item rectangles
and gesture recognizer. The LVGL adapter draws those same rectangles. Copy-Face
can adapt the model and recognizer from this checkpoint; the Apps labels and
geometry are CrowPanel/Eyes-specific, not a universal UI library.

A menu action requires a single contact begun inside its control and released
there. Dragging out permanently cancels that gesture even after reentry. A play
contact cannot become a menu tap by dragging. Multi-contact menu/restore gestures
are canceled; normal scene interaction still supports independent two-eye input.
Failures and 100 ms staleness release actions and require an observed release before
resuming. A new idle contact is accepted normally. Restore uses fresh packets,
never cached coordinates, and handles timer wrap. Page/layout transitions release
physical and synthetic pokes; pending layouts do not accept new actions against
geometry that has not been shown.

Diagnostics are request-only: `?` status; `M` open normal Menu; `Q` normal play;
`H` toggle full performance (changed from 0.2.2's header-only diagnostic);
`E` ECO; `1`–`3` personality; `L/R/B` synthetic pokes; `!` input pause.
Status adds numeric `menu` page and `performance`. Diagnostics are not physical
gesture evidence. The old private 0.2.2 bench script needs adaptation before use
on 0.3.0; do not treat its header-only assumptions as current acceptance.

## Evidence and deployment checklist

- [x] Host navigation tests: every page has Back/Return; only the current app is offered.
- [x] Release-inside, drag-out/reentry, play-origin drag, multi-contact reordering,
  two-eye scene play, failed/stale input, idle-gap recovery and timer-wrap tests.
- [x] Restore at 800 ms but not 799 ms, strict origin/bounds, no retrigger while held.
- [x] Both viewport transforms and button bounds; full CrowPanel/reused Eyes suite.
- [x] ESP-IDF 5.4.2 build; image checksum/revision/capacity verification offline.
- [x] Fresh final review: no Critical/Important blockers; two minor follow-ups below.
- [ ] Explicit hardware handback from Copy-Face before any serial action.
- [ ] Identity/live-state verification, new app-only installation and independent readback.
- [ ] Physical Menu navigation, press feedback/text fit, simultaneous touch, lost input,
  restore hold, full-screen performance and restart defaults on the actual panel.

Candidate image: **614,096 bytes**, P4 DIO/80 MHz/16 MiB,
chip revision range 0.1–1.99, checksum and validation hash valid. Fits the preserved
11 MiB factory slot. Source digest:
`308c0d8a6f21a36da197762b4fb92a5a4cfee3780e0ff9e564ae760fc72fbad9`.
All compiler output and firmware binaries remain private. No hardware command was run.

### Deferred minor follow-ups

- Raw `contacts` diagnostics can briefly say 0 during a fresh held Menu/restore
  touch when no new packet arrives, because that path checks the eye-poke mask.
  The actual gesture state remains valid. Fix contact reporting before relying
  on that field for physical-menu acceptance.
- Deferred-layout tests cover stored shown/wanted geometry; they do not simulate
  repeated failed LVGL locks with contact movement through the adapter. Suppression
  is present and reviewed, but that integration regression test remains to add.

No 0.3.0 live timing, battery improvement or physical-control result is claimed.
Keep the original backups and verified 0.2.2 image; no whole-device upload command.
