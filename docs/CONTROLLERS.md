# Xbox controller path

Verified/researched October 1, 2026. Start with **controller → Mac → Bluetooth LE → toy**. This isolates controller recognition, input mapping, radio delivery and rendering. It also preserves the tested Mac build/update path. It does require the Mac to be present while controlling the toy.

## Current control map: whites, pupils and eyelids

| Input | Face behavior |
| --- | --- |
| Left stick up/down | Move both white eye shapes vertically |
| D-pad | Move black pupils inside the whites; diagonals work |
| Right stick left/right | Proportional wink of the corresponding eye |
| Right stick up | Close both eyes from the bottom |
| Right stick down | Close both eyes from the top |
| Release right stick | Open both eyelids |
| A | Happy expression |
| B | Angry expression |
| X | Surprised expression |
| Y | Neutral expression |
| Stop controller / controller loss | Center gaze, open eyelids, neutral expression |

The mapper applies a15% stick dead zone and clamps all values. White position, pupil offsets and lid amounts are independent. Right-stick diagonals combine a wink with closure of both lids; the stronger closure wins for each eye. Horizontal winks close from the bottom when the vertical stick is centered. Pupils are black cutouts clipped to the white shape. White travel is14pixels vertically; pupil travel is6pixels horizontally and7pixels vertically. The left stick's horizontal axis and triggers are unassigned in this mapping.

Expression buttons act on press edges and keep the selected expression after release. The mapper emits one atomic `eyes` command. The Mac keeps only the newest pending pose and at most one command in flight; a new stop request survives completion of an older command. It sends changes at no more than10Hz. It invalidates its acknowledged state on reconnect so unchanged held input is reapplied to a rebooted toy.

Enabling controller input now waits for the toy to acknowledge **Touch off**. Reconnect repeats that prerequisite. The app negotiates `eyes=1` before sending the new commands; an older firmware build pauses layered control. Stop controller input before manual screen controls. On toy BLE disconnect, firmware centers whites/pupils, opens lids and resets the expression; screen selection and touch configuration remain independent. Later motor control needs its own timeouts, range limits, power design and physical stop tests.

## Connect one controller to this Mac

A data-capable USB cable is the first inspection route. The agent can read USB product/vendor identifiers and ask Apple's GameController framework whether the controller is available. Do not mistake USB power or a USB device listing for successful stick/button input. Verify actual input reports before declaring a particular controller supported.

Apple currently lists Bluetooth Xbox model1708, Series S/X, Elite Series2 and Adaptive controllers. Older Xbox controllers and third-party “Xbox compatible” models are not interchangeable. If USB input is unavailable for the exact device, use its supported Bluetooth connection; pairing may require physically holding its pairing button. Controller firmware updates use Microsoft's supported updater and are not part of this prototype.

Sources: [Apple Xbox controller support](https://support.apple.com/en-us/111101), [Apple connection guidance](https://support.apple.com/en-us/111099), [Apple GameController framework](https://developer.apple.com/documentation/gamecontroller).

## Acceptance checklist for an actual controller

1. Record its exact model and USB identifiers; record firmware version only if actually exposed or read through the supported updater.
2. Confirm macOS lists exactly one extended gamepad and identifies it by vendor/product category.
3. Confirm neutral and full ranges on both sticks, all four expression buttons and D-pad. Physically operating controls is required to verify live range/events.
4. With the class toy connected and Touch off, compare each command's sequence/state to independent USB `FACE` and `PERIPH` lines.
5. Test sustained movement, release, controller unplug, toy reboot/reconnect, Stop during an in-flight command, and a complete firmware update followed by control.
6. Repeat on a second Mac/student kit before recommending a class-wide controller/model/firmware combination.

**Current status:** A physical Microsoft controller was connected over USB on October 1. macOS sees VID `045e`, PID `0b12`, and loads its own `XboxSeriesXGamepad` driver. GameController exposes exactly one extended gamepad, vendor `Controller`, category `Xbox One` (an API category, not proof of the physical model number). No third-party driver or controller firmware modification was needed. The printed model number and exact firmware version have not been read.

A short-lived framework query initially returned no controller. Running a Cocoa main run loop delivered discovery and actual inputs. A real stick sweep reached -1 and+1 on both axes; left/right D-pad events were captured. The original recorded sweep was replayed through BLE with41 USB-matched poses. The owner subsequently reported that the first live smoke test worked well. The independent-whites/pupils/lids extension passed15 exact pixel-reference checks,20 combined synthetic poses, invalid-input atomicity and a real BLE-disconnect reset. Its current physical-control qualification is recorded in TEST-RESULTS.md; gamepad-unplug and held-input update checks remain open.

## Later: direct controller → XIAO

ESP32-S3 supports BLE and does not support Bluetooth Classic. Bluepad32 documents Xbox model1914 and firmware5.15+ as BLE; model1708 uses Classic on older firmware and BLE on newer firmware. This establishes a candidate route, not a completed test on this hardware.

Direct input would require a BLE host stack and controller parser on the XIAO, coexistence with its current BLE peripheral/uploader, pairing persistence, reconnect tests and dependency/license review. Keep it as a separate branch/experiment until it works with the exact controller. Do not replace the working Mac bridge to chase an unverified controller connection.

Source: [Bluepad32's own supported-gamepad documentation](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/).

## Later: controller → iPad → XIAO

A native iPad app can use Apple's GameController and CoreBluetooth APIs. This is the planned laptop-free route. Safari Web Bluetooth must not be assumed available. Adafruit Bluefruit Connect is a researched native BLE UART client for manual live commands; it is not an implemented Xbox mapping app or an uploader for OpenToy's custom OTA protocol. iPad hardware/client tests remain deferred. AI-generated source would need a real build service or other compiler, delivery of the correct binary/manifest to the iPad, and a custom native uploader with key storage and recovery tests.
