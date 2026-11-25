# Tab5 Find Me

Only Blue (`02:00:00:00:00:01`) and the replacement Green (`02:00:00:00:00:02`) accept Find Me control. The retired Green (`02:00:00:00:00:03`) is never a control destination or accepted source. Other face firmwares keep the existing 76-byte FACE v3 battery packet; the 92-byte monitor packet is unchanged.

On the Faces page, tap a live Blue/Green card showing **TAP FACE TO FIND**. Tap that card again, or the red **STOP BUZZ** control in the header on any page, to stop. An active find also ends when the Feather/Q button is pressed. Accessory status must be ready and under five seconds old before starting. Battery freshness alone does not establish accessory readiness.

Find uses the MED actuator command (64 of 127) in v26, 180 ms pulses every 1.5 seconds. Each target enforces a six-second lease, renewed every two seconds by Tab5, and an independent 120-second session limit. Renewal cannot restart an expired/stopped/found session. An actuator temporarily busy with a prior pulse/breathing transition can wait inside these same limits. Tab5 does not restart sessions automatically. Packet loss or Tab5 reboot stops renewal; target reboot starts inactive with a new random boot nonce.

## Transport and validation

- Tab5 uses the existing private Blue AP on channel 1 and static address `192.168.4.2`; AP credentials remain in ignored `FaceLinkConfig.h`.
- Tab5 sends distinct 32-byte binary accessory commands to Blue UDP port 4243. Blue accepts only the dedicated Tab5 IP. Blue handles itself locally or relays only to the replacement Green over ESP-NOW. Green accepts commands only from Blue's MAC.
- Distinct 48-byte accessory status packets return directly from Blue, or from Green via Blue, to Tab5 UDP port 4242. Status includes target boot nonce, monotonic status sequence, acknowledged command sequence, session/controller IDs, readiness, find state, stop reason, remaining session time, and relay source age.
- Commands require the currently advertised target boot nonce and a sequence newer than that target's accepted command floor. Tab5 seeds commands above the reported floor after reconnect/reboot. Duplicate/reordered commands do not reapply actions; duplicate/reordered statuses do not refresh source age.
- Target identity/IP/MAC checks are routing guards, **not cryptographic authentication**. ESP-NOW accessory messages are unencrypted, consistent with the existing radio setup.

`AccessoryProtocol.h`, `FindState.h`, and `FindController.h` contain the wire format and portable state machines. `FeatherFind.h` queues incoming ESP-NOW packets under the radio lock; actuator/I2C work only runs from the main loop. The Tab5 receives on its existing UDP socket and sends with a separate socket.

## Live accessory telemetry (v24)

The Blue and Green face cards show a strip beneath the face: `BREATH19 / Q:DOWN`, `HOLD127 / Q:PRESS`, `BUZZ20 / Q:UP`, or `IDLE0 / Q:UP`. The number is the current actuator command on its 0–127 scale, and the bar below uses that same scale. `BREATH0` includes the quiet rest between breaths; `HOLD` is a button-held steady vibration. `Q:DOWN` means the ready Qwiic button is pressed, `Q:PRESS` marks a recently reported press, and `Q:--` means button state is unavailable. The recent-press marker also expires locally after 1.5 seconds without a new source update.

Live IO is carried in the existing 48-byte status flags, preserving VERSION1 and both battery packet formats. Bits3–10 are button-ready, button-down, motor-active, breathing, held, breathing-enabled, recent-press and live-IO-present; bits16–22 carry command intensity0–127. Bits11–12 carry the source-reported outline-visible and outline-active state; other bits remain invalid. Legacy status flags0–7 remain accepted and display `ACCESSORY --`; they do not establish live IO state. Once source status is five seconds old the strip reads `ACCESSORY OFFLINE`, clears the intensity bar and hides old button/motor activity. Find availability still uses its separate readiness/lease rules.

`ACCESSORY` serial reports append `io_known`, `mode`, `drive`, `button_down`, `recent_press`, `outline` and `outline_active`. These are current source reports, subject to relay latency, rather than physical measurements of vibration. Qwiic LED brightness follows breathing command intensity (`min(drive*6,192)`) and is checked by register readback in the v24 creature verifier. No physical input is simulated by that verifier.

The v25 Tab5 face outline copies the Feather border at twice its logical scale: two rounded lines around the face, in the face color while active and gray during the source-reported afterglow. Outline visibility comes from the Feather, rather than being inferred from button or motor activity. Missing/legacy or stale telemetry never draws a border.

## Verification

Run `python3 tools/run_host_tests.py`. Find tests cover lease expiry, fixed session maximum, physical acknowledgement, rejected renew/replay/boot nonce, timestamp wrap, source age, controller retries/stop, UI page and card selection, source allowlists, battery-packet separation, and queue-only radio callbacks. These host tests do not prove over-the-air delivery or installed hardware behavior.

Bench checks after an explicitly authorized install: start/stop Blue and Green separately; stop via their physical buttons; switch to Race/Log/Demo and use global STOP; disconnect Tab5/radio and verify buzzing ends within six seconds; maintain renewal and verify the target ends by 120 seconds; reboot either end and verify no automatic restart. Check Faces/Race/Log/Demo and background recording still operate. Serial `ACCESSORY` lines expose readiness, source age, boot/status/ack sequence, session, stop reason and remaining time; `find stop`, `find blue`, and `find green` use the same controller and readiness limits as touch.

### Installed v22 bench result

[USB bench evidence](../outputs/find-me-v22-live.json) records both Blue and Green passing20seconds of renewed pulses, card-hit-test start, navigation through Race/Log/Demo, global STOP on Demo, and a separate unrenewed six-second lease. Final direct readback confirms zero/inactive output. The real comparison remained WAITING/session0. Touch events were injected through the same UI hit tests; physical touch and button feel await owner confirmation. The120-second maximum and reboot/replay logic were checked in host tests and review, not a full-duration hardware run.

`find once blue` / `find once green` are deliberate bench diagnostics: one START datagram with no controller renewal. They are not a repeated automatic test. The bounded verifier is `tools/verify_find_me.py --run`.
