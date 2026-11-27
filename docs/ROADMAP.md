# OpenToy family roadmap

## November 27 — full-screen apps

Gravity Water uses the full native canvas under overlaid status text on Waveshare 1.69, Feather and Nesso. The 1.69 Eyes app does too. All three target builds and host regressions passed; Rainbow 1.69 `0.2.2-dev` is installed and USB-checked. Other boards remain untouched. Next: physical screen/touch/tilt acceptance and the remaining apps in the [status overlay migration table](APP-STATUS-BAR.md#adoption-and-compatibility-tracking). No parked device work resumes automatically.


Updated 2026-11-17. These are planned capabilities, not claims of installed or tested features. Follow the [device-family strategy](DEVICE-FAMILY-ARCHITECTURE.md), [status-bar standard](APP-STATUS-BAR.md) and each device's dated verification record.

## Unfinished work at the November 17 stopping point

The UNO Q now displays Eyes and the Tab5 photographed it after installation. The UNO Q and round Waveshare are off the active bench list. Tab5 is also parked with 0.2.0 installed. The local 0.2.1 display changes are unfinished source. Restore a visible viewfinder from cold boot, then verify Copy-Face gaze and qualify real mouth expression on that screen. See the [seed and build index](FIRMWARE-BUILDS.md). [Checkpoint](CHECKPOINT-2026-11-17.md) distinguishes installed firmware from candidates and prototypes.

| Priority / track | Next concrete result | Remaining acceptance / blocker |
| --- | --- | --- |
| 1 — Tab5 camera, display & multi-mode (`0.2.0`) | **`0.2.0` installed & live-qualified**: `720x1280` `ST7123` MIPI-DSI + `0x55` touch, `1280x720` `SC202CS` `OTCAM1` (`age_ms=188`), 3 modes (`CAMEYE`, `COPYFACE`, `EYES1`), `HumanFaceDetect` `0.5.2` gaze tracking, `INA226` `BAT --`, `ES7210` opt-in volatile mic | A cable reconnect restored remote snapshots. The capture sequence later stalled. Verify stable capture and use mirror evidence to check the physical boot-time viewfinder, header, camera indicator and battery status. Physical touch, speaker and dense lip checks remain. |
| 2 — Vibe Coding Camera Eye | Implemented `core/EvidenceGrade.h`, `camera_eye_eval.py`, tests; verified on Tab5 `0.1.1` & `0.2.0` against UNO Q TFT and Round `1.1.0`/`1.1.1` | Add target OCR/version auto-extraction if desired; raw footage remains local only |
| Parked — UNO Q TFT Eyes | Color pattern and Eyes 0.1.0 installed on UNO Q with a write-only ILI9341-compatible driver. Byte-for-byte readback and a native Tab5 photo confirm the displayed app, eyes and header | Qualify touch separately. Document exact shield markings if they become available. |
| Parked — round Waveshare | Integrated, built, bounded-installed **`1.1.1`**, verified `?` telemetry and live Tab5 `0.1.1`/`0.2.0` Camera Eye captures | Owner physical fingertip tap feel & `TOUCH` release trace (`lifts`, `lift0`, `rel`) + audible loudness check |
| Copy-Face (`feature/copy-face` + Tab5) | Added `LipLandmarkAdapter.h` (`adaptFaceMesh468`, `adaptIbug68`, `applyFivePointOnly`) + `LIP-MODEL-FEASIBILITY.md`; live 5-pt gaze follow active on Tab5 `0.2.0` | CrowPanel lacks fitted camera; export Apache-2.0 MediaPipe FaceMesh / PFLD crop to `.espdl` via `esp-ppq` for dense lip copying |
| CrowPanel standalone Eyes | Deploy/verify shared Menu 0.3.0 when that app is selected again | Built only; physical menu/performance/multi-touch acceptance and adapter follow-ups remain |
| RLCD Eyes | Confirm physical KEY menu/performance behavior | Native smooth 0.2.0 installed and appearance accepted; no need to repeat the completed contour port |
| RLCD + CrowPanel battery UI | Replaced `LV_SYMBOL_BATTERY_EMPTY` / hollow battery rectangles with neutral `BAT --` / `BAT X.XXV` in source candidates (`feature/copy-face`, `RLCD 0.2.1`) | Parked hardware untouched; target builds, bounded installs, live ADC/charging checks and physical acceptance pending |
| Feather + Nesso (`1.4.0` candidates) | Implemented `shared/SoundWake.h`, `FeatherMenu.h` (`D1`/`D2`, `D0` BOOT excluded, `Select+Start` disambiguated), and `NessoMenu.h` (360°-rotated header + Menu); all 16 host suites + target ESP32-S3/C6 `arduino-cli` builds **PASS** | Bench hardware (`1.3.0` / `1.3.2`) untouched; bounded install, physical acceptance, and Feather measured battery runtime remain |
| All device apps | Per-app build/install/live matrix, revision records and labelled animated documentation | No universal launcher/all-project port established; other catalogue apps require separate capability assessment |
| Audio + power | Delivered portable volatile `SoundWake.h` (adaptive floor, debounce, persona timing, speaker echo holdoff) and live-verified `ES7210` volatile loudness on Tab5 `0.2.0` | Qualify remaining board mics/speakers and whole-device sleep/runtime current before making battery claims |
| Deployment tooling | Qualify each real device adapter independently | Runner/offline tests do not prove live flashing; preserve original images, bounded writes and explicit device ownership |

The shared status row, exposed-button/touch language, native smooth rendering and distinct personalities remain requirements for maintained ports. BOOT/RESET are recovery only. Unavailable hardware must remain explicit. Keep camera media and private recovery data out of public documentation.

## Sound wakes the character and shapes its expressions

Shared controls are also a current family migration: see the [button and touch standard](CONTROLS.md) and [current/target matrices](../README.md). Replace BOOT-based play gestures, unify Menu/Back/Performance roles and preserve each app's direct interactions through versioned, tested updates.

Owner request: all devices should have a path to waking on sound and making expressive eyes in response to microphone input, with speaker reactions where supported.

1. **Qualify audio per board.** Identify the actual microphone, codec, speaker/amplifier and input/output wiring. Record absent hardware and required add-ons. Test capture and playback separately before combining them.
2. **Wake from quiet idle.** Detect a meaningful rise above background noise, with adjustable sensitivity, debounce and a cooldown. The eyes become alert, then settle gradually when the room becomes quiet. Preserve the owner's always-visible, dim/less-active preference on current Eyes builds; sound wake initially means waking the character from quiet idle, not blanking the screen.
3. **Expressive listening.** Map measured loudness, onset and rhythm to smooth eye opening, blinks, pupil movement and expression intensity. Keep each personality's timing distinct. A sudden sound can produce surprise; sustained sound can hold an attentive expression. These are character responses, not claims about a speaker's identity or emotional state.
4. **Quiet optional replies.** Add short, volume-limited speaker reactions and a mute control. Prevent the toy's own speaker from repeatedly triggering its microphone reaction through suitable suppression/echo handling and cooldowns. Preserve responsive touch, camera tracking and animation.
5. **Low-power acoustic wake.** Separately investigate whether each board can capture/detect sound while the main processor sleeps. Only enable supported wake paths; measure whole-device current, wake latency, missed triggers and false wakes before making runtime claims. An active microphone/codec can offset sleep savings.

Process sound locally without recording or uploading it by default. Sound direction requires suitable microphone geometry and a tested algorithm; never invent a direction from loudness alone. Speech recognition and wake words are separate optional future work.

## Coverage and first checks

| Device/project | Roadmap obligation |
| --- | --- |
| CrowPanel P4 | Qualify onboard audio capture/playback and interaction with display/camera; integrate sound with Eyes and later Copy-Face. |
| Waveshare 4.2 RLCD | Qualify microphone/codec/speaker path and reflective-display power tradeoffs; add expressive listening without assuming backlight controls. |
| Waveshare 1.85C touch/speaker | Preserve existing quiet reaction-sound work; qualify microphone input and speaker feedback suppression separately. |
| Feather family, including Curious/Dreamy/Cheeky | Audit the actual assembly; document microphone/speaker add-ons where needed. Keep current controller behavior and distinct personalities. |
| Nesso N1 | Audit audio hardware and driver availability; implement only verified capabilities or document add-ons. |
| Copy-Face | Combine camera-derived gaze/mouth with sound-triggered alertness; define priorities so audio does not overwrite mouth copying or cause jitter. |
| Future devices | Include audio presence, capture/playback status and possible wake mechanisms in enrollment. |

Each device tracks **hardware available → driver tested → behavior built → installed/live checks → owner observation** independently. Audio absence must not break other app features. Device chats retain exclusive hardware ownership; roadmap work does not interrupt a firmware write or override a pending hardware handoff.

## Battery indication — current follow-up

The owner reports batteries newly connected to CrowPanel and RLCD on 2026-11-17. Their device owners must verify live battery sensing, not infer it from physical connection. Use gauge percentage if genuinely supported, measured voltage otherwise, or `--` if unavailable. Report charging only from a verified hardware signal. Distinguish board battery from any companion's battery. Keep the default-visible header and performance-mode rules.

Compare readings before/after a safe USB-power transition when practical, and check stale/failed readings without blocking animation. A physical battery is not evidence that a gauge or a usable ADC measurement circuit exists. Any missing sensing hardware is a documented limitation, not a fabricated percentage.

## Revision history

- 2026-11-17: added the full unfinished-work backlog, Tab5, UNO Q TFT and Camera Eye stopping priorities; separated installed features from pending qualification.

- 2026-11-17: added family-wide sound wake, expressive microphone response, optional quiet playback, measured low-power investigation, device coverage, and battery-indicator verification follow-up.
