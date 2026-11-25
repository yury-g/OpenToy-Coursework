# Blue's Adafruit seesaw gamepad

The v27 application adds local face controls to Blue using Adafruit's Mini I2C Gamepad QT, product5743. Only Blue is programmed; the v26 checkpoint remains intact. The shared I2C bus is reused; no address/EEPROM writes, seesaw reset, motor trigger, battery test or charger change is introduced by gamepad input.

| Control | Result |
|---|---|
| Joystick | Smoothly move the eyes and brows left/right/up/down; the mouth and battery information stay in place |
| A | Happy |
| B | Sad/sorry |
| X | Surprised |
| Y | Angry |
| Start | Neutral |
| Select | Automatic battery expression; recenter the stick before using it again |

Moving the stick or selecting an expression brings Blue's own face page forward. Center noise does not override battery mode or navigation. D0/D1 battery browsing, D2 breathing and Qwiic hold-to-buzz remain available. Manual expressions are local to Blue; the existing battery telemetry and other devices' automatic faces are unchanged.

The adapter verifies the seesaw hardware ID and product5743 at0x50–0x53 before configuring only the six volatile input/pullup bits. Buttons are active low and debounced25ms, with no synthetic press when reconnecting a held key. Failed/short reads and out-of-range ADC values immediately return to the battery face. Joystick drift has a center deadzone, and Select has priority over other controls. A short `gamepad sample` response observes cached input without interrupting a motor or consuming input events. `i2c scan` is a Blue-only, address-ACK diagnostic; it stops actuation first, like other long diagnostics.

## Verification

31 portable C++ modes cover the controller, gaze buffer bounds and existing battery/network features;43 Python tests include the real I2C adapter with a fake transport, address/product validation and unplug/short-read/invalid-ADC behavior. The normal renderer remains byte-identical when gaze is zero. Live hardware verification is recorded separately and must establish that the physical gamepad actually responds before claiming the joystick/buttons worked.

Sources: [Adafruit pinouts](https://learn.adafruit.com/gamepad-qt/pinouts), [Arduino example and product check](https://learn.adafruit.com/gamepad-qt/arduino), [axis direction](https://learn.adafruit.com/gamepad-qt/circuitpython-and-python), [official seesaw register implementation](https://github.com/adafruit/Adafruit_Seesaw). This checked adapter follows that protocol rather than resetting the existing shared Wire bus.

## Current physical result

The gamepad initially had no response while the gauge, motor driver and Qwiic button remained visible. After the owner's connection changes, it appeared at0x50 with hardwareID0x87 and product5743. The actual bus then contained the gauge0x36 and gamepad0x50 only. [Live identity/status and scan](../outputs/gamepad-v27-live.json) records771 successful samples, zero gamepad transport errors, joystick centered at raw500/510, and a manually selected surprised face. The disconnected haptic/button show unavailable reads as expected; these are not gamepad errors.

[Recorded physical input events](../outputs/gamepad-v27-identify-button.json) include all four joystick directions and A/X/Y/B expression changes. Start/Select, debounce and disconnect fallback have host coverage; they were not separately annotated in that live capture. No synthetic input is presented as a physical test. [Actual Blue screen](../assets/screenshots/blue-gamepad-v27.png).
