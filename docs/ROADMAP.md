# OpenToy family roadmap

Updated 2026-11-17. These are planned capabilities, not claims of installed or tested features. Follow the [device-family strategy](DEVICE-FAMILY-ARCHITECTURE.md), [status-bar standard](APP-STATUS-BAR.md) and each device's dated verification record.

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

- 2026-11-17: added family-wide sound wake, expressive microphone response, optional quiet playback, measured low-power investigation, device coverage, and battery-indicator verification follow-up.
